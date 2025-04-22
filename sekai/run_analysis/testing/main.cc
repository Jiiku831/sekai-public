#include <algorithm>
#include <array>
#include <filesystem>
#include <ranges>
#include <tuple>

#include <GLFW/glfw3.h>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "base/init.h"
#include "base/util.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "sekai/ranges_util.h"
#include "sekai/run_analysis/segment_analysis.h"
#include "sekai/run_analysis/testing/imgui_util.h"
#include "sekai/run_analysis/testing/load_data.h"
#include "sekai/run_analysis/testing/plot.h"

ABSL_FLAG(std::string, run_data, "", "Run data to load");

using namespace ::sekai;
using namespace ::sekai::run_analysis;

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

class PlotDefs {
 public:
  PlotDefs(LoadedData data) : data_(std::move(data)) {}

  absl::Status Update() {
    ASSIGN_OR_RETURN(data_.segments,
                     SegmentRuns(data_.processed_sequence,
                                 {
                                     .window = state_.smoothing_window,
                                     .shift_detection_decay = state_.breakpoint_shift_decay,
                                     .shift_detection_factor = state_.breakpoint_shift,
                                     .mean_shift_threshold =
                                         static_cast<float>(state_.breakpoint_threshold_low / 1000),
                                     .major_shift_threshold = state_.breakpoint_threshold_high,
                                 }));
    state_.selected_segment = std::clamp(state_.selected_segment, 0,
                                         static_cast<int>(data_.segments.active_segments().size()));
    data_.run_histograms = RangesTo<std::vector<Histograms>>(
        data_.segments.active_segments() | std::views::transform([&](const Sequence& seq) {
          return ComputeHistograms(seq, state_.smoothing_window, kInterval);
        }));
    data_.histograms = Histograms::Join(data_.run_histograms);
    ASSIGN_OR_RETURN(state_.segment_analysis,
                     AnalyzeSegment(data_.segments.active_segments()[state_.selected_segment],
                                    /*debug=*/true));
    return absl::OkStatus();
  }

  void Draw(const PlotOptions& options, const ImGuiViewport* viewport) {
    ImVec2 plotSize(viewport->WorkSize.x - kPaddingRight,
                    viewport->WorkSize.y - kHistogramHeight - kPaddingBottom);
    if (!ImPlot::BeginPlot("Event Points", plotSize, ImPlotFlags_Crosshairs)) {
      return;
    }
    ImPlot::SetupAxes(nullptr, nullptr);
    ImPlot::SetupAxis(ImAxis_Y2, nullptr, ImPlotAxisFlags_Opposite);
    ImPlot::SetupAxis(ImAxis_Y3, nullptr, ImPlotAxisFlags_Opposite);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL);
    ConditionalPlot(&state_.enable_raw_graph, PointsLineGraph("Raw", data_.raw_sequence))
        .Draw(options);
    ConditionalPlot(&state_.enable_processed_graph,
                    PointsLineGraph("Processed", data_.processed_sequence))
        .Draw(options);
    ConditionalPlot(&state_.enable_breakpoints_graph,
                    MarkersPlot("Breakpoints", data_.segments.breakpoints()))
        .Draw(options);
    ImPlot::SetAxis(ImAxis_Y2);
    ConditionalPlot(&state_.enable_breakpoints_graph,
                    PointsLineGraph("Breakpoint Scores", data_.segments.breakpoint_scores()))
        .Draw(options);
    if (state_.enable_breakpoints_graph) {
      ImPlot::DragLineY(0, &state_.breakpoint_threshold_low, ImVec4(1, 0, 0, 1), 1,
                        ImPlotDragToolFlags_NoFit);
      ImPlot::TagY(state_.breakpoint_threshold_low, ImVec4(1, 0, 0, 1), "Thresh");
    }
    ImPlot::SetAxis(ImAxis_Y3);
    auto cluster_assignments = RangesTo<std::vector<Sequence>>(
        data_.segments.debug().cluster_assignments | std::views::transform([](const Sequence& seq) {
          return Sequence{.points = RangesTo<std::vector<Snapshot>>(
                              seq.points | std::views::transform([](const Snapshot& pt) {
                                return Snapshot{pt.time, pt.diff};
                              }))};
        }));
    std::array colors = {
        ImVec4(0, 1, 1, 1),
        ImVec4(0, 1, 0, 1),
        ImVec4(0, 0, 1, 1),
        ImVec4(1, 0, 0, 1),
    };
    SegmentsPlot<MarkersPlot>("Cluster Assignment", cluster_assignments, colors).Draw(options);
    SegmentsPlot<PointsLineGraph>("Cluster Mean", data_.segments.debug().cluster_means, colors)
        .Draw(options);
    std::array bound_colors = {
        ImVec4(0, 1, 1, 0.5),
        ImVec4(0, 1, 0, 0.5),
        ImVec4(0, 0, 1, 0.5),
        ImVec4(1, 0, 0, 0.5),
    };
    SegmentsPlot<PointsLineGraph>("Cluster LB", data_.segments.debug().cluster_lbs, bound_colors)
        .Draw(options);
    SegmentsPlot<PointsLineGraph>("Cluster UB", data_.segments.debug().cluster_ubs, bound_colors)
        .Draw(options);
    // ConditionalPlot(&state_.enable_step_graph, MarkersPlot("Raw Diffs",
    // data_.histograms.step_seq))
    //     .Draw(options);
    // ConditionalPlot(&state_.enable_step_graph,
    //                 MarkersPlot("Smoothed Diffs", data_.segments.smoothed_diffs()))
    //     .Draw(options);
    // ConditionalPlot(
    //     &state_.enable_speed_graph,
    //     SegmentsPlot<PointsLineGraph>("Segment Speeds", data_.segments.segment_speeds()))
    //     .Draw(options);
    ImPlot::SetAxis(ImAxis_Y1);
    ConditionalPlot(&state_.enable_segments_graph,
                    SegmentsPlot<PointsLineGraph>("Segment", data_.segments.active_segments()))
        .Draw(options);
    ImPlot::EndPlot();
  }

  void DrawHistograms(const PlotOptions& options, const ImGuiViewport* viewport) {
    const int kHistogramWidth = (viewport->WorkSize.x - 2 * kPaddingRight) / 3;
    ImVec2 histPlotSize(kHistogramWidth, kHistogramHeight);
    if (!ImPlot::BeginPlot("Speed Histograms##Histograms", histPlotSize)) {
      return;
    }
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit,
                      ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxis(ImAxis_Y2, nullptr, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxis(ImAxis_Y3, nullptr, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    ConditionalPlot(&state_.enable_step_hist, HistogramPlot("Steps", data_.histograms.steps))
        .Draw(options);
    ConditionalPlot(&state_.enable_speed_hist,
                    HistogramPlot("Hourly Speeds", data_.histograms.speeds))
        .Draw(options);
    ConditionalPlot(&state_.enable_smoothed_hist,
                    HistogramPlot("Smoothed Hourly Speeds", data_.histograms.smoothed_speeds))
        .Draw(options);
    ImPlot::SetAxis(ImAxis_Y2);
    ConditionalPlot(&state_.enable_step_hist,
                    HistogramPlot("Steps", data_.run_histograms[state_.selected_segment].steps))
        .Draw(options);
    ConditionalPlot(&state_.enable_speed_hist,
                    HistogramPlot("Segment Hourly Speeds",
                                  data_.run_histograms[state_.selected_segment].speeds))
        .Draw(options);
    ConditionalPlot(&state_.enable_smoothed_hist,
                    HistogramPlot("Segment Smoothed Hourly Speeds",
                                  data_.run_histograms[state_.selected_segment].smoothed_speeds))
        .Draw(options);
    ImPlot::SetAxis(ImAxis_Y3);
    ConditionalPlot(&state_.enable_segment_hist,
                    HistogramPlot("Segment Avg Speeds", data_.histograms.segment_speeds,
                                  {
                                      .bins = 100,
                                  }))
        .Draw(options);
    ImPlot::EndPlot();
  }

  void DrawSteps(const PlotOptions& options, const ImGuiViewport* viewport) {
    const int kHistogramWidth = (viewport->WorkSize.x - 2 * kPaddingRight) / 3;
    ImVec2 histPlotSize(kHistogramWidth, kHistogramHeight);
    if (!ImPlot::BeginPlot("Sequence", histPlotSize)) {
      return;
    }
    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxis(ImAxis_Y1, nullptr);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 300'000);
    ImPlot::SetupAxis(ImAxis_Y2, nullptr, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_AutoFit);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL);
    auto seqs = RangesTo<std::vector<Sequence>>(
        state_.segment_analysis.clusters | std::views::transform([](const Cluster& cluster) {
          return Sequence{.points = RangesTo<std::vector<Snapshot>>(
                              cluster.vals | std::views::transform([](const Snapshot& pt) {
                                return Snapshot{pt.time, pt.diff};
                              }))};
        }));
    std::array colors = {
        ImVec4(1, 0, 0, 1),
        ImVec4(0, 1, 0, 1),
        ImVec4(0, 0, 1, 1),
    };
    std::size_t num_clusters = state_.segment_analysis.clusters.size() - 1;
    SegmentsPlot<MarkersPlot>("Cluster", seqs | std::views::take(num_clusters)).Draw(options);
    SegmentsPlot<MarkersPlot>("Outliers", seqs | std::views::drop(num_clusters)).Draw(options);
    for (std::size_t i = 0; i < std::min(colors.size(), num_clusters); ++i) {
      double lv = state_.segment_analysis.clusters[i].mean;
      ImPlot::TagY(lv, colors[i], "%.0f", lv);
      ImPlot::DragLineY(i, &lv, colors[i], /*thickness=*/1, ImPlotDragToolFlags_NoInputs);
      // float bound = 3 * state_.segment_analysis.clusters[i].stdev;
      // ImPlot::TagY(lv - bound, colors[i], "LB %.0f", lv - bound);
      // ImPlot::TagY(lv + bound, colors[i], "UB %.0f", lv + bound);
    }
    ImPlot::SetAxis(ImAxis_Y2);
    std::string title = absl::StrCat("Segment ", state_.selected_segment);
    PointsLineGraph(title.c_str(), data_.segments.active_segments()[state_.selected_segment])
        .Draw(options);
    if (state_.segment_analysis.game_count_analysis.ok()) {
      ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross,
                                 /*size=*/ImGui::GetFontSize() / 3, /*fill=*/IMPLOT_AUTO_COL,
                                 /*weight=*/ImGui::GetFontSize() / 8,
                                 /*outline=*/ImVec4(1, 0, 0, 1));
      MarkersPlot("Rejected Samples", state_.segment_analysis.game_count_analysis->rejected_samples)
          .Draw(options);
    }
    ImPlot::EndPlot();
  }

  void DrawDebug(const PlotOptions& options, const ImGuiViewport* viewport) {
    const int kWidth = (viewport->WorkSize.x - 2 * kPaddingRight) / 3 + 3;
    ImVec2 childSize(kWidth, kHistogramHeight);
    if (!ImGui::BeginChild("Debug Info", childSize)) {
      return;
    }
    if (ImGui::CollapsingHeader("Team Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
      const auto& analysis = data_.team_analysis;
      PrintMessage("Analyze Request", data_.team_analysis_request);
      if (analysis.ok()) {
        ImGui::BulletText("Event Bonus: %.2f%%", analysis->event_bonus());
        ImGui::BulletText("Skill Value: [%.0f%%, %.0f%%]",
                          analysis->skill_details().skill_value_lower_bound(),
                          analysis->skill_details().skill_value_upper_bound());
      } else {
        ImGui::BulletText("%s", analysis.status().ToString().c_str());
      }
    }
    if (ImGui::CollapsingHeader("Run Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::BulletText("Total Duration: %s",
                        absl::FormatDuration(data_.segments.total_duration()).c_str());
      ImGui::BulletText("Total Uptime: %s",
                        absl::FormatDuration(data_.segments.total_uptime()).c_str());
      ImGui::Indent();
      ImGui::BulletText("Auto Time: %s",
                        absl::FormatDuration(data_.segments.total_auto_time()).c_str());
      ImGui::Unindent();
      ImGui::BulletText("Total Downtime: %s",
                        absl::FormatDuration(data_.segments.total_downtime()).c_str());
    }
    if (ImGui::CollapsingHeader("Segment Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
      const auto& analysis = state_.segment_analysis;
      ImGui::BulletText("Is Confident: %s", analysis.is_confident ? "true" : "false");
      ImGui::BulletText("Is Auto: %s", analysis.is_auto ? "true" : "false");
      ImGui::BulletText("Segment length: %s",
                        absl::FormatDuration(analysis.segment_length).c_str());
      ImGui::SeparatorText("Snapshot Analysis");
      for (std::size_t i = 0; i < analysis.clusters.size(); ++i) {
        const auto& cluster = analysis.clusters[i];
        if (cluster.mean < 0) {
          continue;
        }
        ImGui::BulletText("Cluster %ld: mean=%.0f stdev=%.0f", i, cluster.mean, cluster.stdev);
      }
      ImGui::BulletText("Cluster mean ratio: %.2f", analysis.cluster_mean_ratio);
      if (!analysis.cluster_debug.split_debug.empty() && ImGui::TreeNode("Cluster Split Debug")) {
        for (std::size_t i = 0; i < analysis.cluster_debug.split_debug.size(); ++i) {
          const auto& debug = analysis.cluster_debug.split_debug[i];
          ImGui::BulletText("%lu-cluster: rss=%.0f, rss_ratio=%.2f, min_size=%.2f", i + 1,
                            debug.rss, debug.rss_ratio, debug.min_size);
          ImGui::BulletText("%lu-cluster init means: %s", i + 1,
                            absl::StrJoin(debug.initial_means, ", ").c_str());
          ImGui::BulletText(
              "%lu-cluster means: %s", i + 1,
              absl::StrJoin(
                  RangesTo<std::vector<float>>(
                      debug.clusters | std::views::transform([](const auto& c) { return c.mean; })),
                  ", ")
                  .c_str());
        }
        ImGui::TreePop();
      }
      if (analysis.cluster_debug.split_debug.empty()) {
        ImGui::BulletText("No cluster split debug available.");
      }
      ImGui::SeparatorText("Game Count Analysis");
      if (analysis.game_count_analysis.ok()) {
        ImGui::BulletText("Est. EP/g: %.0f (sigma=%.0f)",
                          analysis.game_count_analysis->ep_per_game.mean(),
                          analysis.game_count_analysis->ep_per_game.stdev());
        ImGui::BulletText("Game Count: %d", analysis.game_count_analysis->game_count);
        ImGui::BulletText("Est. GPH: %.1f [%.1f, %.1f]",
                          analysis.game_count_analysis->estimated_gph.value(),
                          analysis.game_count_analysis->estimated_gph.lower_bound(),
                          analysis.game_count_analysis->estimated_gph.upper_bound());
      } else {
        ImGui::BulletText("%s", analysis.game_count_analysis.status().ToString().c_str());
      }
      PrintMessage("Segment Proto", SegmentAnalysisResultToApiSegment(analysis));
    }
    ImGui::EndChild();
  }

  void DrawCheckboxes() {
    int counter = 0;
    for (const auto& [title, toggle] : checkboxes_) {
      if (counter > 0) ImGui::SameLine();
      ImGui::PushID(counter++);
      ImGui::Checkbox(title.c_str(), toggle);
      ImGui::PopID();
    }
  }

  void DrawInputs() {
    DrawCheckboxes();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderFloat("BP Shift", &state_.breakpoint_shift, 0, 10);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderFloat("BP Shift Decay", &state_.breakpoint_shift_decay, 0, 1);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderFloat("BP Thresh High", &state_.breakpoint_threshold_high, 0.01, 0.99);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderInt("Smoothing Window", &state_.smoothing_window, 1, 12);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderInt("Select Segment", &state_.selected_segment, 0,
                     data_.segments.active_segments().size() - 1);
  }

 private:
  struct PlotState {
    bool enable_raw_graph = true;
    bool enable_processed_graph = true;
    bool enable_step_graph = false;
    bool enable_segments_graph = true;
    bool enable_breakpoints_graph = true;
    bool enable_speed_graph = true;
    bool enable_step_hist = false;
    bool enable_speed_hist = true;
    bool enable_smoothed_hist = true;
    bool enable_segment_hist = true;
    int smoothing_window = kWindow;
    int selected_segment = 0;
    float breakpoint_shift = kBreakpointShift;
    float breakpoint_shift_decay = kBreakpointDecay;
    double breakpoint_threshold_low = kBreakpointThresholdLow * 1000;
    float breakpoint_threshold_high = kBreakpointThresholdHigh;
    SegmentAnalysisResult segment_analysis;
  } state_;
  LoadedData data_;
  std::vector<std::pair<std::string, bool*>> checkboxes_ = {
      {"Raw Sequence", &state_.enable_raw_graph},
      {"Processed Sequence", &state_.enable_processed_graph},
      {"Steps", &state_.enable_step_graph},
      {"Segments", &state_.enable_segments_graph},
      {"Breakpoints", &state_.enable_breakpoints_graph},
      {"Segment Speed", &state_.enable_speed_graph},
      {"Step Histogram", &state_.enable_step_hist},
      {"Speed Histogram", &state_.enable_speed_hist},
      {"Smoothed Speed Histogram", &state_.enable_smoothed_hist},
      {"Segment Speed Histogram", &state_.enable_segment_hist},
  };
};

void DrawFrame(PlotDefs& plots) {
  PlotOptions options;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::Begin("Run Analysis Debugger", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
  plots.DrawInputs();
  plots.Draw(options, viewport);
  plots.DrawHistograms(options, viewport);
  ImGui::SameLine();
  plots.DrawSteps(options, viewport);
  ImGui::SameLine();
  plots.DrawDebug(options, viewport);
  ImGui::End();
  // ImGui::ShowDemoWindow();
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

  std::filesystem::path data_path = absl::GetFlag(FLAGS_run_data);
  if (data_path.empty()) {
    LOG(ERROR) << "No data specified to read.";
    return 1;
  }
  absl::StatusOr<LoadedData> data = LoadData(data_path);
  if (!data.ok()) {
    LOG(ERROR) << "Failed to load data:\n" << data.status();
    return 1;
  }

  GLFWwindow* window = InitGlfwWindow();
  if (window == nullptr) {
    LOG(ERROR) << "Failed to initialize GLFW window.";
    return 1;
  }
  InitImPlot(window);

  PlotDefs defs(*std::move(data));
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
