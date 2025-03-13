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

ABSL_FLAG(std::string, run_data, "", "Run data to load");

using ::sekai::run_analysis::ConditionalPlot;
using ::sekai::run_analysis::LoadData;
using ::sekai::run_analysis::LoadedData;
using ::sekai::run_analysis::PlotOptions;
using ::sekai::run_analysis::PointsLineGraph;
using ::sekai::run_analysis::SegmentsLineGraph;

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
  PlotDefs(const LoadedData& data) : data_(data) {}
  void Draw(PlotOptions& options) {
    ConditionalPlot(&state_.enable_raw_graph, PointsLineGraph("Raw", data_.raw_sequence))
        .Draw(options);
    ConditionalPlot(&state_.enable_processed_graph,
                    PointsLineGraph("Processed", data_.processed_sequence))
        .Draw(options);
    ConditionalPlot(&state_.enable_segments_graph, SegmentsLineGraph(data_.segments)).Draw(options);
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

 private:
  struct PlotState {
    bool enable_raw_graph = true;
    bool enable_processed_graph = true;
    bool enable_segments_graph = true;
  } state_;
  const LoadedData& data_;
  std::vector<std::pair<std::string, bool*>> checkboxes_ = {
      {"Raw Sequence", &state_.enable_raw_graph},
      {"Processed Sequence", &state_.enable_processed_graph},
      {"Segments", &state_.enable_segments_graph},
  };
};

void DrawFrame(PlotDefs& plots) {
  PlotOptions options;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::Begin("Run Analysis Debugger", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
  plots.DrawCheckboxes();

  constexpr int kPaddingBottom = 80;
  constexpr int kPaddingRight = 35;
  ImVec2 plotSize(viewport->WorkSize.x - kPaddingRight, viewport->WorkSize.y - kPaddingBottom);
  if (ImPlot::BeginPlot("Event Points", plotSize)) {
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
    plots.Draw(options);
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

  PlotDefs defs(*data);
  while (!glfwWindowShouldClose(window)) {
    if (!PollEvents(window)) continue;
    NewFrame();
    DrawFrame(defs);
    RenderFrame(window);
    SwapFrame(window);
  }

  CleanupImPlot();
  CleanupGlfw(window);
}
