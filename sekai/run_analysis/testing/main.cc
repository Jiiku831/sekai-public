#include <array>
#include <filesystem>
#include <tuple>

#include <GLFW/glfw3.h>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "base/init.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "load_data.h"
#include "plot.h"
#include "sekai/ranges_util.h"

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

  void Update() {
    data_.runs = RangesTo<std::vector<Runs>>(
        data_.segments | std::views::transform([&](const auto& x) {
          return SegmentRuns(x, state_.smoothing_window, state_.breakpoint_shift,
                             state_.breakpoint_threshold_low / 1000,
                             state_.breakpoint_threshold_high);
        }));
    data_.histograms = ComputeHistograms(data_.segments, state_.smoothing_window, kInterval);
    data_.run_histograms = RangesTo<std::vector<Histograms>>(
        data_.runs | std::views::transform([&](const auto& x) {
          return ComputeHistograms(x.runs, state_.smoothing_window, kInterval);
        }));
    data_.combined_run_histograms = Histograms::Join(data_.run_histograms);
  }

  void Draw(const PlotOptions& options) {
    ConditionalPlot(&state_.enable_raw_graph, PointsLineGraph("Raw", data_.raw_sequence))
        .Draw(options);
    ConditionalPlot(&state_.enable_processed_graph,
                    PointsLineGraph("Processed", data_.processed_sequence))
        .Draw(options);
    ConditionalPlot(&state_.enable_segments_graph,
                    SegmentsPlot<PointsLineGraph>("Segment", data_.segments))
        .Draw(options);
    auto breakpoints = RangesTo<std::vector<Sequence>>(
        data_.runs | std::views::transform([](const auto& run) { return run.breakpoints; }));
    ConditionalPlot(&state_.enable_breakpoints_graph,
                    SegmentsPlot<MarkersPlot>("Breakpoints", breakpoints))
        .Draw(options);
    ImPlot::SetAxis(ImAxis_Y2);
    auto bp_scores = RangesTo<std::vector<Sequence>>(
        data_.runs | std::views::transform([](const auto& run) { return run.breakpoint_scores; }));
    ConditionalPlot(&state_.enable_breakpoints_graph,
                    SegmentsPlot<PointsLineGraph>("Breakpoint Scores", bp_scores))
        .Draw(options);
    ImPlot::DragLineY(0, &state_.breakpoint_threshold_low, ImVec4(1, 0, 0, 1), 1,
                      ImPlotDragToolFlags_NoFit);
    ImPlot::TagY(state_.breakpoint_threshold_low, ImVec4(1, 0, 0, 1), "Thresh");
    // ConditionalPlot(&state_.enable_step_graph,
    //                 MarkersPlot("Steps", data_.combined_run_histograms.step_seq))
    //     .Draw(options);
    ImPlot::SetAxis(ImAxis_Y3);
    ConditionalPlot(&state_.enable_step_graph,
                    MarkersPlot("Smoothed Speed", data_.combined_run_histograms.smooth_seq))
        .Draw(options);
  }

  void DrawHistograms(const PlotOptions& options) {
    ConditionalPlot(&state_.enable_step_hist, HistogramPlot("Steps", data_.histograms.steps))
        .Draw(options);
    ConditionalPlot(&state_.enable_speed_hist,
                    HistogramPlot("Hourly Speeds", data_.histograms.speeds))
        .Draw(options);
    ConditionalPlot(&state_.enable_smoothed_hist,
                    HistogramPlot("Smoothed Hourly Speeds", data_.histograms.smoothed_speeds))
        .Draw(options);
  }

  void DrawSegmentHistograms(const PlotOptions& options) {
    ConditionalPlot(&state_.enable_step_hist,
                    HistogramPlot("Steps (All Runs)", data_.combined_run_histograms.steps))
        .Draw(options);
    ConditionalPlot(&state_.enable_speed_hist,
                    HistogramPlot("Hourly Speeds (All Runs)", data_.combined_run_histograms.speeds))
        .Draw(options);
    ConditionalPlot(&state_.enable_smoothed_hist,
                    HistogramPlot("Smoothed Hourly Speeds (All Runs)",
                                  data_.combined_run_histograms.smoothed_speeds))
        .Draw(options);
    ConditionalPlot(&state_.enable_step_hist,
                    HistogramPlot("Steps", data_.run_histograms[state_.selected_segment].steps))
        .Draw(options);
    ConditionalPlot(
        &state_.enable_speed_hist,
        HistogramPlot("Hourly Speeds", data_.run_histograms[state_.selected_segment].speeds))
        .Draw(options);
    ConditionalPlot(&state_.enable_smoothed_hist,
                    HistogramPlot("Smoothed Hourly Speeds",
                                  data_.run_histograms[state_.selected_segment].smoothed_speeds))
        .Draw(options);
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
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderFloat("BP Shift", &state_.breakpoint_shift, 0, 2);
    // ImGui::SameLine();
    // ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    // ImGui::SliderFloat("BP Thresh Low", &state_.breakpoint_threshold_low, 1, 10);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderFloat("BP Thresh High", &state_.breakpoint_threshold_high, 0.01, 0.99);

    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderInt("Smoothing Window", &state_.smoothing_window, 1, 12);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(15 * ImGui::GetFontSize());
    ImGui::SliderInt("Select Segment", &state_.selected_segment, 0, data_.segments.size() - 1);
  }

 private:
  struct PlotState {
    bool enable_raw_graph = true;
    bool enable_processed_graph = true;
    bool enable_step_graph = true;
    bool enable_segments_graph = true;
    bool enable_breakpoints_graph = true;
    bool enable_step_hist = false;
    bool enable_speed_hist = true;
    bool enable_smoothed_hist = true;
    int smoothing_window = kWindow;
    int selected_segment = 0;
    float breakpoint_shift = 1;
    double breakpoint_threshold_low = kBreakpointThresholdLow * 1000;
    float breakpoint_threshold_high = kBreakpointThresholdHigh;
  } state_;
  LoadedData data_;
  std::vector<std::pair<std::string, bool*>> checkboxes_ = {
      {"Raw Sequence", &state_.enable_raw_graph},
      {"Processed Sequence", &state_.enable_processed_graph},
      {"Steps", &state_.enable_step_graph},
      {"Segments", &state_.enable_segments_graph},
      {"Breakpoints", &state_.enable_breakpoints_graph},
      {"Step Histogram", &state_.enable_step_hist},
      {"Speed Histogram", &state_.enable_speed_hist},
      {"Smoothed Speed Histogram", &state_.enable_smoothed_hist},
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

  constexpr int kPaddingBottom = 125;
  constexpr int kPaddingRight = 35;
  const int kHistogramWidth = (viewport->WorkSize.x - 2 * kPaddingRight) / 3;
  const int kHistogramHeight = 600;

  ImVec2 plotSize(viewport->WorkSize.x - kPaddingRight,
                  viewport->WorkSize.y - kHistogramHeight - kPaddingBottom);
  if (ImPlot::BeginPlot("Event Points", plotSize, ImPlotFlags_Crosshairs)) {
    ImPlot::SetupAxes(nullptr, nullptr);
    ImPlot::SetupAxis(ImAxis_Y2, nullptr, ImPlotAxisFlags_Opposite);
    ImPlot::SetupAxis(ImAxis_Y3, nullptr, ImPlotAxisFlags_Opposite);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    plots.Draw(options);
    ImPlot::EndPlot();
  }
  ImVec2 histPlotSize(kHistogramWidth, kHistogramHeight);
  if (ImPlot::BeginPlot("Speed Histograms##Histograms", histPlotSize)) {
    ImPlot::SetupAxes(nullptr, nullptr);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    plots.DrawHistograms(options);
    ImPlot::EndPlot();
  }
  ImGui::SameLine();
  if (ImPlot::BeginPlot("Segment Histograms##Histograms2", histPlotSize)) {
    ImPlot::SetupAxes(nullptr, nullptr);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    plots.DrawSegmentHistograms(options);
    ImPlot::EndPlot();
  }
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
    defs.Update();
    DrawFrame(defs);
    RenderFrame(window);
    SwapFrame(window);
  }

  CleanupImPlot();
  CleanupGlfw(window);
}
