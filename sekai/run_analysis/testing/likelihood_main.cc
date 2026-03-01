#include <array>
#include <random>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "absl/base/nullability.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "base/init.h"
#include "base/util.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "sekai/config.h"
#include "sekai/db/master_db.h"
#include "sekai/db/proto/music_meta.pb.h"
#include "sekai/estimator.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/testing/plot.h"

using namespace ::sekai;
using namespace ::sekai::run_analysis;

ABSL_FLAG(int, power, 358'449, "team power");
ABSL_FLAG(float, event_bonus, 435, "event bonus %");
ABSL_FLAG(float, skill_min, 182, "min skill %");
ABSL_FLAG(float, skill_max, 218, "max skill %");
ABSL_FLAG(float, observed_gph, 28.1, "observed games/hr");
ABSL_FLAG(float, observed_ppg, 62'900, "observed pt/game");

constexpr const char* kGlslVersion = "#version 130";
constexpr float kScaleFactor = 2.0;
constexpr int kFontSize = 12 * kScaleFactor;
constexpr int kWindowWidth = 1600 * kScaleFactor;
constexpr int kWindowHeight = 900 * kScaleFactor;
constexpr const char* kWindowTitle = "Run Analysis Debugger";
constexpr std::array kClearColor = {0.45f, 0.55f, 0.60f, 1.00f};
constexpr int kPaddingBottom = 125;
constexpr int kPaddingRight = 35;
constexpr int kHistogramHeight = 600;
// Variation in play. Not sure how we can model this.
// Probably can use player point variance?
constexpr float kPlayVariance = 0.3;
constexpr float kSkillOffset = 0.015;
constexpr int kLikelihoodHeatmapResolution = 200;

void GlfwError(int code, const char* msg) { LOG(ERROR) << "GLFW Error " << code << ": " << msg; }

GLFWwindow* InitGlfwWindow() {
  glfwSetErrorCallback(GlfwError);
  if (!glfwInit()) return nullptr;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  GLFWwindow* window =
      glfwCreateWindow(kWindowWidth, kWindowHeight, kWindowTitle, nullptr, nullptr);
  if (window == nullptr) return nullptr;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  return window;
}

void CleanupGlfw(GLFWwindow* window) {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void InitImPlot(GLFWwindow* window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(kGlslVersion);

  ImGuiIO& io = ImGui::GetIO();
  if (io.Fonts->AddFontFromFileTTF("/mnt/c/Windows/Fonts/consola.ttf", kFontSize) == nullptr) {
    LOG(WARNING) << "Failed to load font.";
  }
  io.Fonts->AddFontDefault();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(kScaleFactor);
}

void CleanupImPlot() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
};

bool PollEvents(GLFWwindow* window) {
  glfwPollEvents();
  if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
    ImGui_ImplGlfw_Sleep(10);
    return false;
  }
  return true;
}

void NewFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

class PlayStrategy {
 public:
  PlayStrategy(absl::string_view name, Estimator::Mode mode,
               std::vector<const db::MusicMeta * absl_nonnull> metas)
      : name_(name), estimator_(mode, metas) {}

  const char* name() const { return name_.c_str(); }

  const Estimator& estimator() const { return estimator_; }

 private:
  std::string name_;
  Estimator estimator_;
};

class PlotDefs {
 public:
  PlotDefs() = default;

  absl::Status Update() {
    if (!should_update()) {
      return absl::OkStatus();
    }
    RETURN_IF_ERROR(SimulatePlay());
    RETURN_IF_ERROR(ComputeLikelihood());
    last_state_ = state_;
    return absl::OkStatus();
  }

  absl::Status SimulatePlay() {
    constexpr int kIterations = 100'000;
    std::normal_distribution dist(state_.filler_avg_skill, state_.filler_skill_stdev);
    data_.min_event_points.clear();
    data_.max_event_points.clear();
    data_.min_event_points.reserve(kIterations);
    data_.max_event_points.reserve(kIterations);
    float multiplier = kBoostMultipliers[state_.boost_multiplier_index];
    const Estimator& estimator = strategies_[state_.play_strategy_index].estimator();
    for (int i = 0; i < kIterations; ++i) {
      float skill_value = std::clamp(dist(gen_), static_cast<float>(kMinSkillValue),
                                     static_cast<float>(kMaxSkillValue));
      data_.min_event_points.push_back(
          multiplier * estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus,
                                                       state_.skill_min, skill_value, skill_value));
      data_.max_event_points.push_back(
          multiplier * estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus,
                                                       state_.skill_max, skill_value, skill_value));
    }
    return absl::OkStatus();
  }

  absl::Status ComputeLikelihood() {
    const double kStepX =
        static_cast<double>(kMaxSkillValue - kMinSkillValue) / (kLikelihoodHeatmapResolution - 1);
    const double kStepY = static_cast<double>(state_.skill_max - state_.skill_min) /
                          (kLikelihoodHeatmapResolution - 1);
    float multiplier = kBoostMultipliers[state_.boost_multiplier_index];
    const Estimator& estimator = strategies_[state_.play_strategy_index].estimator();

    for (int i = 0; i < kLikelihoodHeatmapResolution; ++i) {
      for (int j = 0; j < kLikelihoodHeatmapResolution; ++j) {
        double filler_skill = kMinSkillValue + i * kStepX;
        double player_skill = state_.skill_min + j * kStepY;

        double mu =
            multiplier * estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus,
                                                         player_skill, filler_skill, filler_skill);
        double filler_skill_var = state_.filler_skill_stdev * state_.filler_skill_stdev;
        double sigma = multiplier *
                       std::sqrt(estimator.Variance(state_.power, state_.event_bonus, player_skill,
                                                    filler_skill_var, filler_skill_var)) *
                       (1 + kPlayVariance);
        double min = multiplier *
                     estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus, player_skill,
                                                     kMinSkillValue, kMinSkillValue) *
                     (1 - kSkillOffset);
        double max = multiplier *
                     estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus, player_skill,
                                                     kMaxSkillValue, kMaxSkillValue) *
                     (1 + kSkillOffset);
        if (state_.observed_ppg < min || state_.observed_ppg > max) {
          data_.likelihood_heatmap[kLikelihoodHeatmapResolution - j - 1][i] = 0;
        } else {
          constexpr double kDistFactor = std::numbers::inv_sqrtpi / std::numbers::sqrt2;
          double norm_pt = (state_.observed_ppg - mu) / sigma;
          data_.likelihood_heatmap[kLikelihoodHeatmapResolution - j - 1][i] =
              kDistFactor / sigma * std::exp(-0.5 * norm_pt * norm_pt);
        }
      }
    }
    return absl::OkStatus();
  }

  void Draw(const ImGuiViewport* viewport) const {
    const int kHistogramWidth = (viewport->WorkSize.x - 2 * kPaddingRight);
    constexpr int kBaseSize = 4500;
    float multiplier = kBoostMultipliers[state_.boost_multiplier_index];
    ImVec2 histPlotSize(kHistogramWidth, kHistogramHeight);
    if (!ImPlot::BeginPlot("Point Distribution##Distributions", histPlotSize)) {
      return;
    }
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, kBaseSize * multiplier);
    ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxis(
        ImAxis_Y2, nullptr,
        ImPlotAxisFlags_Opposite | ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    HistogramPlot("Min Event Points", data_.min_event_points).Draw(options_);
    HistogramPlot("Max Event Points", data_.max_event_points).Draw(options_);

    ImPlot::SetAxis(ImAxis_Y2);
    const Estimator& estimator = strategies_[state_.play_strategy_index].estimator();
    auto color = ImVec4(1, 0, 0, 1);
    double ppg = state_.observed_ppg;
    ImPlot::DragLineX(0, &ppg, color, /*thickness=*/1, ImPlotDragToolFlags_NoInputs);
    ImPlot::TagX(ppg, color, "Observed");

    color = ImVec4(0, 1, 0, 1);
    double min_pts = multiplier * estimator.ExpectedEpFixedEncore(
                                      state_.power, state_.event_bonus, state_.skill_min,
                                      state_.filler_avg_skill, state_.filler_avg_skill);
    double filler_skill_var = state_.filler_skill_stdev * state_.filler_skill_stdev;
    double min_stdev =
        multiplier *
        std::sqrt(estimator.Variance(state_.power, state_.event_bonus, state_.skill_min,
                                     filler_skill_var, filler_skill_var)) *
        (1 + kPlayVariance);
    // ImPlot::DragLineX(0, &min_pts, color, /*thickness=*/1, ImPlotDragToolFlags_NoInputs);
    // ImPlot::TagX(min_pts, color, "Expect Min");
    double abs_min =
        multiplier *
        estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus, state_.skill_min,
                                        kMinSkillValue, kMinSkillValue) *
        (1 - kSkillOffset);
    double abs_max =
        multiplier *
        estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus, state_.skill_min,
                                        kMaxSkillValue, kMaxSkillValue) *
        (1 + kSkillOffset);
    NormalDistributionPdf("Min Dist", min_pts, min_stdev, color,
                          {.clamp_min = abs_min, .clamp_max = abs_max, .draw_mu = true})
        .Draw(options_);

    color = ImVec4(0, 0, 1, 1);
    double max_pts = multiplier * estimator.ExpectedEpFixedEncore(
                                      state_.power, state_.event_bonus, state_.skill_max,
                                      state_.filler_avg_skill, state_.filler_avg_skill);
    double max_stdev =
        multiplier *
        std::sqrt(estimator.Variance(state_.power, state_.event_bonus, state_.skill_max,
                                     filler_skill_var, filler_skill_var)) *
        (1 + kPlayVariance);
    // ImPlot::DragLineX(0, &max_pts, color, /*thickness=*/1, ImPlotDragToolFlags_NoInputs);
    // ImPlot::TagX(max_pts, color, "Expected Max");
    abs_min = multiplier *
              estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus, state_.skill_max,
                                              kMinSkillValue, kMinSkillValue) *
              (1 - kSkillOffset);
    abs_max = multiplier *
              estimator.ExpectedEpFixedEncore(state_.power, state_.event_bonus, state_.skill_max,
                                              kMaxSkillValue, kMaxSkillValue) *
              (1 + kSkillOffset);
    NormalDistributionPdf("Max Dist", max_pts, max_stdev, color,
                          {.clamp_min = abs_min, .clamp_max = abs_max, .draw_mu = true})
        .Draw(options_);
    ImPlot::EndPlot();
  }

  void DrawHeatmap(const ImGuiViewport* viewport) const {
    ImVec2 histPlotSize(kHistogramHeight * 1.5, kHistogramHeight * 1.5);

    static ImPlotColormap map = ImPlotColormap_Viridis;
    ImPlot::PushColormap(map);

    if (!ImPlot::BeginPlot("Likelihood##Heatmap", histPlotSize)) {
      return;
    }
    ImPlot::SetupAxis(ImAxis_X1, "Filler Skill", ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxis(ImAxis_Y1, "Player Skill", ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxesLimits(kMinSkillValue, kMaxSkillValue, state_.skill_min, state_.skill_max);

    ImPlot::PlotHeatmap("Likelihood", data_.likelihood_heatmap[0], kLikelihoodHeatmapResolution,
                        kLikelihoodHeatmapResolution, 0, 0, nullptr,
                        ImPlotPoint(kMinSkillValue, state_.skill_min),
                        ImPlotPoint(kMaxSkillValue, state_.skill_max));

    const Estimator& estimator = strategies_[state_.play_strategy_index].estimator();
    float multiplier = kBoostMultipliers[state_.boost_multiplier_index];
    double filler_skill_var = state_.filler_skill_stdev * state_.filler_skill_stdev;
    double max_stdev = std::sqrt(estimator.VarianceInvSkill(
        state_.power, state_.event_bonus, state_.skill_min, filler_skill_var, filler_skill_var));
    double min_stdev = std::sqrt(estimator.VarianceInvSkill(
        state_.power, state_.event_bonus, state_.skill_max, filler_skill_var, filler_skill_var));
    constexpr float kAlpha = 1;
    double max =
        estimator.ExpectedEpFixedEncoreInvSkill(state_.power, state_.event_bonus, state_.skill_min,
                                                state_.observed_ppg / multiplier) +
        kAlpha * max_stdev;
    double min =
        estimator.ExpectedEpFixedEncoreInvSkill(state_.power, state_.event_bonus, state_.skill_max,
                                                state_.observed_ppg / multiplier) -
        kAlpha * min_stdev;
    auto color = ImVec4(1, 0, 0, 1);
    if (kMinSkillValue <= min && min <= kMaxSkillValue) {
      ImPlot::DragLineX(0, &min, color, /*thickness=*/1, ImPlotDragToolFlags_NoInputs);
      ImPlot::TagX(min, color, "Min");
    }
    color = ImVec4(0, 1, 0, 1);
    if (kMinSkillValue <= max && max <= kMaxSkillValue) {
      ImPlot::DragLineX(0, &max, color, /*thickness=*/1, ImPlotDragToolFlags_NoInputs);
      ImPlot::TagX(max, color, "Max");
    }

    ImPlot::EndPlot();
    ImPlot::PopColormap();
  }

  void DrawDebug(const ImGuiViewport* viewport) const {
    const int kWidth = (viewport->WorkSize.x - 2 * kPaddingRight) / 3 + 3;
    ImVec2 childSize(kWidth, 600 * kScaleFactor);
    if (!ImGui::BeginChild("Debug Info", childSize)) {
      return;
    }
    if (ImGui::CollapsingHeader("Runner Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BulletText("Power:         %d", state_.power);
      ImGui::BulletText("Event Bonus:   %.2f%%", state_.event_bonus);
      ImGui::BulletText("Skill:         [%.2f%%, %.2f%%]", state_.skill_min, state_.skill_max);
      ImGui::BulletText("Observed G/hr: %.2f", state_.observed_gph);
      ImGui::BulletText("Observed pt/G: %.2f", state_.observed_ppg);
    }
    if (ImGui::CollapsingHeader("Filler Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BulletText("Avg. Skill:    %.2f%%", state_.filler_avg_skill);
      ImGui::BulletText("Skill Stdev:   %.2f%%", state_.filler_skill_stdev);
    }
    if (ImGui::CollapsingHeader("Play Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BulletText("Boost Usage:   %d (%dx)", state_.boost_multiplier_index,
                        kBoostMultipliers[state_.boost_multiplier_index]);
    }
    ImGui::EndChild();
  }

  void DrawInputs() {
    // Runner parameters
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderInt("Power", &state_.power, 0, 500'000);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("Event Bonus", &state_.event_bonus, 0, 1000);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("Skill Min", &state_.skill_min, kMinSkillValue,
                       std::min(state_.skill_max, static_cast<float>(kMaxSkillValue)));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("Skill Max", &state_.skill_max,
                       std::max(state_.skill_min, static_cast<float>(kMinSkillValue)),
                       kMaxSkillValue);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("Observed G/hr", &state_.observed_gph, 0, 40);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderFloat("Observed pt/G", &state_.observed_ppg, 0, 150'000);

    // Filler parameters
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("Filler avg skill", &state_.filler_avg_skill, kMinSkillValue,
                       kMaxSkillValue);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("Filler skill stdev", &state_.filler_skill_stdev, 0, 50);

    // Play parameters
    std::string boost_multiplier =
        absl::StrFormat("%dx", kBoostMultipliers[state_.boost_multiplier_index]);
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderInt("Boost", &state_.boost_multiplier_index, 0, kBoostMultipliers.size() - 1,
                     boost_multiplier.c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(30 * ImGui::GetFontSize());
    ImGui::Combo("Play Strategy", &state_.play_strategy_index, strategy_names_.data(),
                 strategy_names_.size());
  }

 private:
  struct PlotState {
    // Runner parameters
    int power = absl::GetFlag(FLAGS_power);
    float event_bonus = absl::GetFlag(FLAGS_event_bonus);
    float skill_min = absl::GetFlag(FLAGS_skill_min);
    float skill_max = absl::GetFlag(FLAGS_skill_max);
    float observed_gph = absl::GetFlag(FLAGS_observed_gph);
    float observed_ppg = absl::GetFlag(FLAGS_observed_ppg);

    // Filler parameters
    float filler_avg_skill = 230;
    float filler_skill_stdev = 10;

    // Play parameters
    int boost_multiplier_index = 10;
    int play_strategy_index = 0;

    bool operator==(const PlotState& other) const {
      constexpr float kTol = 0.01;
      return std::abs(power - other.power) < kTol &&
             std::abs(event_bonus - other.event_bonus) < kTol &&
             std::abs(skill_min - other.skill_min) < kTol &&
             std::abs(skill_max - other.skill_max) < kTol &&
             std::abs(observed_gph - other.observed_gph) < kTol &&
             std::abs(observed_ppg - other.observed_ppg) < kTol &&
             std::abs(filler_avg_skill - other.filler_avg_skill) < kTol &&
             std::abs(filler_skill_stdev - other.filler_skill_stdev) < kTol &&
             boost_multiplier_index == other.boost_multiplier_index &&
             play_strategy_index == other.play_strategy_index;
    }
  };
  PlotState state_;
  std::optional<PlotState> last_state_;

  bool should_update() const { return !last_state_.has_value() || *last_state_ != state_; }

  struct Data {
    std::vector<int> min_event_points;
    std::vector<int> max_event_points;
    double likelihood_heatmap[kLikelihoodHeatmapResolution][kLikelihoodHeatmapResolution];
  } data_;

  std::vector<PlayStrategy> strategies_ = {
      PlayStrategy("Ebi Ex (Multi)", Estimator::Mode::kMulti,
                   db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                     return meta.music_id() == 74 && meta.difficulty() == db::DIFF_EXPERT;
                   })),
      PlayStrategy("Ebi Mas (Multi)", Estimator::Mode::kMulti,
                   db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                     return meta.music_id() == 74 && meta.difficulty() == db::DIFF_MASTER;
                   })),
      PlayStrategy("Random Ex (Multi)", Estimator::Mode::kMulti,
                   db::MasterDb::GetIf<db::MusicMeta>([](const db::MusicMeta& meta) {
                     return meta.difficulty() == db::DIFF_EXPERT;
                   })),
  };
  std::vector<const char*> strategy_names_ = RangesTo<std::vector>(
      strategies_ | std::views::transform([&](const auto& strat) { return strat.name(); }));

  std::random_device rd_;
  std::mt19937 gen_{rd_()};
  PlotOptions options_;
};

void DrawFrame(PlotDefs& plots) {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::Begin("Run Analysis Debugger", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
  plots.DrawInputs();
  plots.Draw(viewport);
  plots.DrawHeatmap(viewport);
  ImGui::SameLine();
  plots.DrawDebug(viewport);
  ImGui::End();
  ImGui::ShowDemoWindow();
  // ImPlot::ShowDemoWindow();
}

void RenderFrame(GLFWwindow* window) {
  ImGui::Render();
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  glClearColor(kClearColor[0] * kClearColor[3], kClearColor[1] * kClearColor[3],
               kClearColor[2] * kClearColor[3], kClearColor[3]);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SwapFrame(GLFWwindow* window) { glfwSwapBuffers(window); }

int main(int argc, char** argv) {
  Init(argc, argv);

  GLFWwindow* window = InitGlfwWindow();
  if (window == nullptr) {
    LOG(ERROR) << "Failed to initialize GLFW window.";
    return 1;
  }
  InitImPlot(window);

  PlotDefs defs;
  while (!glfwWindowShouldClose(window)) {
    if (!PollEvents(window)) continue;
    NewFrame();
    absl::Status status = defs.Update();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to update state: " << status;
      break;
    }
    DrawFrame(defs);
    RenderFrame(window);
    SwapFrame(window);
  }

  CleanupImPlot();
  CleanupGlfw(window);
}
