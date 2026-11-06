#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include "Huffman.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>

// Fixed window dimensions
constexpr int WINDOW_WIDTH  = 620;
constexpr int WINDOW_HEIGHT = 450;

// Application state
static CompressionResult g_LastResult;
static std::string g_LastInputName;
static std::string g_LastOutputName;
static std::string g_LastAction; // "compress", "decompress", or ""
static std::string g_StatusMessage = "Select an action below to begin.";

// Helper to format byte sizes
static std::string FormatBytes(uint64_t bytes) {
    std::ostringstream ss;
    if (bytes < 1024) {
        ss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else {
        ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
    }
    return ss.str();
}

// Windows native file dialog
static std::string BrowseFile(const char* filter) {
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(buf);
    }
    return "";
}

// Opens the generated file directly in Windows Explorer
static void OpenInFolder(const std::string& path) {
    if (path.empty() || !std::filesystem::exists(path)) return;
    std::string param = "/select,\"" + path + "\"";
    ShellExecuteA(NULL, "open", "explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
}

// 1 Click Compress
static void OnCompressClicked() {
    std::string input = BrowseFile("All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0");
    if (input.empty()) return;

    // Save as .huff in the same directory
    std::string output = input + ".huff";

    g_LastAction = "compress";
    g_LastInputName = std::filesystem::path(input).filename().string();
    g_LastOutputName = std::filesystem::path(output).filename().string();
    g_LastResult = CompressFile(input, output);

    if (g_LastResult.success) {
        std::ostringstream ss;
        ss << "Compressed successfully! Saved "
           << std::fixed << std::setprecision(1) << g_LastResult.spaceSaved << "% space.";
        g_StatusMessage = ss.str();
    } else {
        g_StatusMessage = "Error: " + g_LastResult.error;
    }
}

// 1 Click Decompress
static void OnDecompressClicked() {
    std::string input = BrowseFile("Huffman Files (*.huff)\0*.huff\0All Files (*.*)\0*.*\0");
    if (input.empty()) return;

    // Restore original file name in the same directory
    std::filesystem::path inPath(input);
    std::string output;
    if (inPath.extension() == ".huff") {
        output = inPath.replace_extension("").string();
    } else {
        output = input + ".extracted.txt";
    }

    g_LastAction = "decompress";
    g_LastInputName = std::filesystem::path(input).filename().string();
    g_LastOutputName = std::filesystem::path(output).filename().string();
    g_LastResult = DecompressFile(input, output);

    if (g_LastResult.success) {
        g_StatusMessage = "Decompressed successfully! Restored to: " + g_LastOutputName;
    } else {
        g_StatusMessage = "Error: " + g_LastResult.error;
    }
}

// Pure Black Theme
static void ApplyBlackTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.WindowPadding     = ImVec2(18.0f, 18.0f);
    style.ItemSpacing       = ImVec2(10.0f, 12.0f);
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;

    ImVec4* colors = style.Colors;

    // Jet Black Canvas (#000000 & #101010)
    colors[ImGuiCol_WindowBg]      = ImVec4(0.00f, 0.00f, 0.00f, 1.0f); // True Black
    colors[ImGuiCol_ChildBg]       = ImVec4(0.06f, 0.06f, 0.06f, 1.0f); // #101010
    colors[ImGuiCol_Border]        = ImVec4(0.18f, 0.18f, 0.18f, 1.0f); // #2E2E2E
    colors[ImGuiCol_Separator]     = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);

    // Text (#FFFFFF & #888888)
    colors[ImGuiCol_Text]          = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
    colors[ImGuiCol_TextDisabled]  = ImVec4(0.52f, 0.52f, 0.52f, 1.0f);

    // Sleek Dark Charcoal Buttons (#1A1A1A, #282828)
    colors[ImGuiCol_Button]        = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
}

// User Interface
static void RenderUI() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("MainPanel", nullptr, flags);

    // Header
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "HUFFMAN COMPRESSOR");
    ImGui::TextDisabled("Click a button to pick a file. Saves in the same folder.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Two Main Action Buttons
    if (ImGui::Button("Compress a File", ImVec2(-1, 50))) {
        OnCompressClicked();
    }

    if (ImGui::Button("Decompress a File (.huff)", ImVec2(-1, 50))) {
        OnDecompressClicked();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Results & Status Box
    ImGui::BeginChild("StatusCard", ImVec2(0, 135), true);

    if (g_LastAction.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 25);
        ImGui::TextDisabled("Ready. Click an action above to start.");
    } else {
        ImVec4 statusColor = g_LastResult.success
            ? ImVec4(0.30f, 0.85f, 0.50f, 1.0f)   // Soft green
            : ImVec4(0.95f, 0.35f, 0.35f, 1.0f);   // Soft red

        ImGui::TextColored(statusColor, "%s", g_StatusMessage.c_str());
        ImGui::Spacing();

        if (g_LastResult.success) {
            ImGui::Text("File: %s -> %s", g_LastInputName.c_str(), g_LastOutputName.c_str());

            if (g_LastAction == "compress") {
                ImGui::TextDisabled("Original: %s  |  Compressed: %s  |  Saved: %.1f%%",
                    FormatBytes(g_LastResult.originalSize).c_str(),
                    FormatBytes(g_LastResult.compressedSize).c_str(),
                    g_LastResult.spaceSaved);
            } else {
                ImGui::TextDisabled("Restored size: %s", FormatBytes(g_LastResult.originalSize).c_str());
            }

            ImGui::Spacing();
            if (ImGui::Button("Show in Windows Explorer", ImVec2(220, 26))) {
                OpenInFolder(g_LastResult.outputPath);
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Fix the window size: disable resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Huffman Compressor", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Lock window limits strictly
    glfwSetWindowSizeLimits(window, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ApplyBlackTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        // Pitch Black clear
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}