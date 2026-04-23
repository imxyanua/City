#include "Camera.h"
#include "CityBuilder.h"
#include "Scene.h"
#include "Shader.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <cstdlib>

namespace {

struct AppContext {
    Camera* camera = nullptr;
    Scene* scene = nullptr;
    bool* showMenu = nullptr;
};

struct KeyState {
    bool w = false, a = false, s = false, d = false;
    bool space = false, shift = false;
    bool firstMouse = true;
    double lastX = 0.0, lastY = 0.0;
};

KeyState g_keys;
float g_deltaTime = 0.0f;
double g_lastFrame = 0.0;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS && ctx && ctx->showMenu != nullptr) {
        *ctx->showMenu = !(*ctx->showMenu);
        glfwSetInputMode(window, GLFW_CURSOR, *ctx->showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        if (!*ctx->showMenu) {
            g_keys.firstMouse = true;
        }
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (ctx && ctx->showMenu && *ctx->showMenu) {
            *ctx->showMenu = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            g_keys.firstMouse = true;
            return;
        }
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    const bool down = (action == GLFW_PRESS || action == GLFW_REPEAT);
    const bool capture = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;
    if (capture) {
        return;
    }

    if (key == GLFW_KEY_W)
        g_keys.w = down;
    if (key == GLFW_KEY_S)
        g_keys.s = down;
    if (key == GLFW_KEY_A)
        g_keys.a = down;
    if (key == GLFW_KEY_D)
        g_keys.d = down;
    if (key == GLFW_KEY_SPACE)
        g_keys.space = down;
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
        g_keys.shift = down;
}

void mouseCallback(GLFWwindow* window, double x, double y)
{
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->camera) {
        return;
    }

    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (ctx->showMenu && *ctx->showMenu) {
        return;
    }

    if (g_keys.firstMouse) {
        g_keys.lastX = x;
        g_keys.lastY = y;
        g_keys.firstMouse = false;
    }
    const float xoff = static_cast<float>(x - g_keys.lastX);
    const float yoff = static_cast<float>(g_keys.lastY - y);
    g_keys.lastX = x;
    g_keys.lastY = y;

    ctx->camera->processMouseMovement(xoff, yoff);
}

void scrollCallback(GLFWwindow* window, double xoff, double yoff)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoff, yoff);

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->camera) {
        return;
    }
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    ctx->camera->processScroll(static_cast<float>(yoff));
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (height <= 0) {
        return;
    }
    glViewport(0, 0, width, height);
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx && ctx->scene) {
        ctx->scene->setAspect(static_cast<float>(width) / static_cast<float>(height));
    }
}

std::filesystem::path exeDirectory(const char* argv0)
{
    std::error_code ec;
    std::filesystem::path p(argv0);
    if (!p.empty() && std::filesystem::is_regular_file(p, ec)) {
        return p.parent_path();
    }
    return std::filesystem::current_path();
}

GLuint g_offscreenFbo = 0;
GLuint g_offscreenColor = 0;
GLuint g_offscreenDepth = 0;
int g_offscreenW = 0;
int g_offscreenH = 0;
GLuint g_emptyVao = 0;

// Bloom FBOs (quarter-res ping-pong)
GLuint g_bloomFbo[2]  = {0, 0};
GLuint g_bloomTex[2]  = {0, 0};
int    g_bloomW = 0, g_bloomH = 0;

void destroyBloomFbos()
{
    for (int i = 0; i < 2; ++i) {
        if (g_bloomTex[i]) { glDeleteTextures(1, &g_bloomTex[i]); g_bloomTex[i] = 0; }
        if (g_bloomFbo[i]) { glDeleteFramebuffers(1, &g_bloomFbo[i]); g_bloomFbo[i] = 0; }
    }
    g_bloomW = g_bloomH = 0;
}

void ensureBloomFbos(int w, int h)
{
    int bw = w / 4, bh = h / 4;
    if (bw <= 0 || bh <= 0) return;
    if (bw == g_bloomW && bh == g_bloomH && g_bloomFbo[0] != 0) return;
    destroyBloomFbos();
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &g_bloomFbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, g_bloomFbo[i]);
        glGenTextures(1, &g_bloomTex[i]);
        glBindTexture(GL_TEXTURE_2D, g_bloomTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bw, bh, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_bloomTex[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g_bloomW = bw;
    g_bloomH = bh;
}

void destroyOffscreenFbo()
{
    if (g_offscreenDepth != 0) { glDeleteTextures(1, &g_offscreenDepth); g_offscreenDepth = 0; }
    if (g_offscreenColor != 0) { glDeleteTextures(1, &g_offscreenColor); g_offscreenColor = 0; }
    if (g_offscreenFbo != 0)   { glDeleteFramebuffers(1, &g_offscreenFbo); g_offscreenFbo = 0; }
    g_offscreenW = g_offscreenH = 0;
    destroyBloomFbos();
}

void ensureOffscreenFbo(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (w == g_offscreenW && h == g_offscreenH && g_offscreenFbo != 0) return;
    destroyOffscreenFbo();
    glGenFramebuffers(1, &g_offscreenFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_offscreenFbo);
    // HDR color attachment
    glGenTextures(1, &g_offscreenColor);
    glBindTexture(GL_TEXTURE_2D, g_offscreenColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_offscreenColor, 0);
    // Depth attachment
    glGenTextures(1, &g_offscreenDepth);
    glBindTexture(GL_TEXTURE_2D, g_offscreenDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_offscreenDepth, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FBO] HDR framebuffer incomplete.\n";
        destroyOffscreenFbo();
    } else {
        g_offscreenW = w;
        g_offscreenH = h;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ensureBloomFbos(w, h);
}

void formatHourVi(float hour, char* buf, size_t bufSize)
{
    float h = std::fmod(hour, 24.0f);
    if (h < 0.0f) {
        h += 24.0f;
    }
    int hi = static_cast<int>(std::floor(h));
    int mi = static_cast<int>(std::floor((h - static_cast<float>(hi)) * 60.0f + 0.5f));
    int hd = hi;
    if (mi >= 60) {
        mi = 0;
        hd = (hd + 1) % 24;
    }
    std::snprintf(buf, bufSize, "%02d:%02d", hd, mi);
}

void applyTimeOfDayHour(Scene& scene, float hour, float clearCol[3])
{
    float h = std::fmod(hour, 24.0f);
    if (h < 0.0f) {
        h += 24.0f;
    }

    const float pi = glm::pi<float>();
    const float angle = (h / 24.0f) * 2.0f * pi - pi * 0.5f;
    const float el = std::sin(angle);
    const float az = (h / 24.0f) * 2.0f * pi;

    glm::vec3 sunDir(
        std::cos(el * 1.1f) * std::sin(az),
        std::max(el, -0.12f),
        std::cos(el * 1.1f) * std::cos(az));
    sunDir = glm::normalize(sunDir);
    const glm::vec3 lightTravel = -sunDir;
    scene.setLightDir(lightTravel);

    const float day = glm::clamp((el + 0.25f) / 1.15f, 0.0f, 1.0f);
    float twi = 0.0f;
    if (h >= 5.0f && h < 8.5f) {
        twi = glm::smoothstep(5.0f, 8.0f, h);
    } else if (h >= 17.0f && h < 20.5f) {
        twi = glm::smoothstep(20.5f, 17.0f, h);
    }

    const glm::vec3 skyNight(0.04f, 0.05f, 0.12f);
    const glm::vec3 skyDay(0.38f, 0.48f, 0.62f);
    const glm::vec3 skyWarm(0.72f, 0.42f, 0.30f);
    glm::vec3 sky = glm::mix(skyNight, skyDay, day);
    sky = glm::mix(sky, skyWarm, twi * (1.0f - day * 0.5f));

    const glm::vec3 gNight(0.03f, 0.03f, 0.05f);
    const glm::vec3 gDay(0.20f, 0.18f, 0.16f);
    const glm::vec3 ground = glm::mix(gNight, gDay, day);

    const glm::vec3 sunNight(0.35f, 0.38f, 0.55f);
    const glm::vec3 sunDay(1.05f, 0.97f, 0.88f);
    const glm::vec3 sunDawn(1.15f, 0.55f, 0.35f);
    glm::vec3 sunCol = glm::mix(sunNight, sunDay, day);
    sunCol = glm::mix(sunCol, sunDawn, twi * 0.85f);
    const float sunPow = glm::clamp(el * 1.3f + 0.15f, 0.0f, 1.0f);
    sunCol *= 0.35f + 0.65f * sunPow;

    scene.setSkyColor(sky);
    scene.setGroundColor(ground);
    scene.setSunColor(sunCol);
    scene.setDayFactor(day);

    if (day < 0.18f) {
        clearCol[0] = skyNight.x * 0.92f;
        clearCol[1] = skyNight.y * 0.92f;
        clearCol[2] = skyNight.z * 0.92f;
    } else {
        clearCol[0] = sky.x * 0.35f + 0.02f;
        clearCol[1] = sky.y * 0.35f + 0.03f;
        clearCol[2] = sky.z * 0.35f + 0.06f;
    }
}



} // namespace

int main(int argc, char* argv[])
{
    const std::filesystem::path exeDir = exeDirectory(argc > 0 ? argv[0] : "");
    const std::filesystem::path assetModel = exeDir / "assets" / "models" / "procedural_city_7.glb";
    const std::filesystem::path vertPath = exeDir / "shaders" / "vertex.glsl";
    const std::filesystem::path fragPath = exeDir / "shaders" / "fragment.glsl";
    const std::filesystem::path postVertPath = exeDir / "shaders" / "post.vert";
    const std::filesystem::path postFragPath = exeDir / "shaders" / "post.frag";


    if (!std::filesystem::exists(assetModel)) {
        std::cerr << "Missing GLB model. Expected:\n  " << assetModel.string()
                  << "\nCopy procedural_city_7.glb next to the executable under assets/models/\n";
        return 1;
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(1600, 900, u8"Phố thành phố — procedural_city_7.glb", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imguiIo = ImGui::GetIO();
    imguiIo.IniFilename = nullptr;
    ImGui::StyleColorsDark();

#ifdef _WIN32
    {
        ImFontConfig fc;
        fc.OversampleH = 2;
        if (ImFont* vf = imguiIo.Fonts->AddFontFromFileTTF(
                "C:/Windows/Fonts/segoeui.ttf", 17.0f, &fc, imguiIo.Fonts->GetGlyphRangesVietnamese())) {
            (void)vf;
        }
    }
#endif

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330");

    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);

    Shader shader;
    if (!shader.loadFromFiles(vertPath.string(), fragPath.string())) {
        std::cerr << "Shader build failed. Paths:\n  " << vertPath.string() << "\n  " << fragPath.string() << "\n";
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Shader postShader;
    if (!postShader.loadFromFiles(postVertPath.string(), postFragPath.string())) {
        std::cerr << "Post shader failed.\n";
        ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }

    // Bloom shaders (use post.vert for fullscreen triangle)
    const auto bloomExtPath = exeDir / "shaders" / "bloom_extract.frag";
    const auto bloomBlurPath = exeDir / "shaders" / "bloom_blur.frag";
    Shader bloomExtractShader, bloomBlurShader;
    bool hasBloom = bloomExtractShader.loadFromFiles(postVertPath.string(), bloomExtPath.string())
                 && bloomBlurShader.loadFromFiles(postVertPath.string(), bloomBlurPath.string());
    if (!hasBloom) {
        std::cerr << "Bloom shaders failed (non-critical, continuing without bloom).\n";
    }

    // Rain 3D Shader
    const auto rainVertPath = exeDir / "shaders" / "rain.vert";
    const auto rainFragPath = exeDir / "shaders" / "rain.frag";
    Shader rainShader;
    rainShader.loadFromFiles(rainVertPath.string(), rainFragPath.string());

    // --- Setup 3D Rain System ---
    const int numRainDrops = 50000;
    std::vector<glm::vec3> rainOffsets(numRainDrops);
    for (int i = 0; i < numRainDrops; ++i) {
        float x = (static_cast<float>(rand()) / RAND_MAX) * 150.0f - 75.0f;
        float y = (static_cast<float>(rand()) / RAND_MAX) * 120.0f - 20.0f;
        float z = (static_cast<float>(rand()) / RAND_MAX) * 150.0f - 75.0f;
        rainOffsets[i] = glm::vec3(x, y, z);
    }
    
    float rainVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    
    GLuint rainVAO, rainVBO, rainInstanceVBO;
    glGenVertexArrays(1, &rainVAO);
    glBindVertexArray(rainVAO);
    
    glGenBuffers(1, &rainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rainVertices), rainVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glGenBuffers(1, &rainInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rainInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, rainOffsets.size() * sizeof(glm::vec3), rainOffsets.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    
    glBindVertexArray(0);

    glGenVertexArrays(1, &g_emptyVao);
    Scene scene;
    scene.setAspect(1600.0f / 900.0f);
    if (!scene.loadCityModel(assetModel.string())) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::vector<std::string> carPaths = {
        (exeDir / "assets" / "models" / "2014_bmw_z4_sdrive35is.glb").string(),
        (exeDir / "assets" / "models" / "2021_lexus_bev_sport_concept.glb").string(),
        (exeDir / "assets" / "models" / "2022_nio_et7.glb").string()
    };
    scene.loadCarModels(carPaths);
    scene.initTrafficAndPedestrians();

    scene.setInstanceTransforms({glm::mat4(1.0f)});

    Camera camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);

    bool showMenu = false;
    bool wireframe = false;
    bool cullBack = true;
    bool vsync = true;
    float clearCol[3] = {0.07f, 0.08f, 0.12f};
    bool syncTimeOfDay = true;
    float timeOfDayHour = 12.0f;
    float rainIntensity = 0.0f;
    float bloomThreshold = 1.2f;
    float bloomIntensity = 0.4f;
    float godRayIntensity = 0.35f;
    float colorGradeIntensity = 0.85f;
    float vignetteIntensity = 0.55f;
    glm::vec2 windDir = glm::normalize(glm::vec2(-1.0f, -0.2f));
    applyTimeOfDayHour(scene, timeOfDayHour, clearCol);

    static bool useGridLayout = false;
    static int gridRows = 2;
    static int gridCols = 6;
    static float gridSpaceX = 95.0f;
    static float gridSpaceZ = 110.0f;

    AppContext ctx;
    ctx.camera = &camera;
    ctx.scene = &scene;
    ctx.showMenu = &showMenu;
    glfwSetWindowUserPointer(window, &ctx);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << u8"Tab: bật/tắt menu | WASD + chuột (khi menu tắt) | Esc: đóng menu / thoát\n";

    g_lastFrame = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        g_deltaTime = static_cast<float>(now - g_lastFrame);
        g_lastFrame = now;

        scene.update(g_deltaTime);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (showMenu) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.98f));
            ImGui::SetNextWindowBgAlpha(0.98f);
            ImGui::Begin(u8"Cài đặt (Tab để đóng/mở)", &showMenu, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text(u8"FPS: %.0f", static_cast<double>(imguiIo.Framerate));
            ImGui::Separator();

            if (ImGui::BeginTabBar("SettingsTabs")) {

                if (ImGui::BeginTabItem(u8"Môi trường")) {
                    ImGui::Text(u8"--- Thời gian ---");
                    ImGui::Checkbox(u8"Đồng bộ ánh sáng theo giờ", &syncTimeOfDay);
                    if (syncTimeOfDay) {
                        ImGui::SliderFloat(u8"Giờ (0–24)", &timeOfDayHour, 0.0f, 24.0f, "%.2f");
                        char tbuf[8];
                        formatHourVi(timeOfDayHour, tbuf, sizeof(tbuf));
                        ImGui::Text(u8"Đang chỉnh: %s", tbuf);
                    }
                    ImGui::Separator();
                    ImGui::Text(u8"--- Sương mù ---");
                    float fogD = scene.fogDensity();
                    if (ImGui::SliderFloat(u8"Mật độ (0 = tắt)", &fogD, 0.0f, 1.2f)) scene.setFogDensity(fogD);
                    float fBase = scene.fogBaseHeight();
                    if (ImGui::SliderFloat(u8"Độ cao nền", &fBase, -10.0f, 50.0f)) scene.setFogBaseHeight(fBase);
                    float fFalloff = scene.fogHeightFalloff();
                    if (ImGui::SliderFloat(u8"Giảm theo độ cao", &fFalloff, 0.0f, 0.2f)) scene.setFogHeightFalloff(fFalloff);
                    glm::vec3 fc = scene.fogColor();
                    float fc3[3] = {fc.x, fc.y, fc.z};
                    if (ImGui::ColorEdit3(u8"Màu sương", fc3)) scene.setFogColor(glm::vec3(fc3[0], fc3[1], fc3[2]));
                    ImGui::Separator();
                    ImGui::Text(u8"--- Mưa & Ướt ---");
                    if (ImGui::SliderFloat(u8"Cường độ mưa", &rainIntensity, 0.0f, 1.0f)) scene.setWetness(rainIntensity);
                    ImGui::SliderFloat2(u8"Hướng gió", &windDir.x, -1.0f, 1.0f);
                    windDir = glm::normalize(windDir + glm::vec2(1e-5f));
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(u8"Ánh sáng")) {
                    float exp = scene.exposure();
                    if (ImGui::SliderFloat(u8"Phơi sáng (exposure)", &exp, 0.2f, 3.5f)) scene.setExposure(exp);
                    
                    ImGui::BeginDisabled(syncTimeOfDay);
                    glm::vec3 sky = scene.skyColor();
                    glm::vec3 gnd = scene.groundColor();
                    glm::vec3 sun = scene.sunColor();
                    float sky3[3] = {sky.x, sky.y, sky.z};
                    float gnd3[3] = {gnd.x, gnd.y, gnd.z};
                    float sun3[3] = {sun.x, sun.y, sun.z};
                    if (ImGui::ColorEdit3(u8"Màu trời (ambient trên)", sky3)) scene.setSkyColor(glm::vec3(sky3[0], sky3[1], sky3[2]));
                    if (ImGui::ColorEdit3(u8"Màu đất (ambient dưới)", gnd3)) scene.setGroundColor(glm::vec3(gnd3[0], gnd3[1], gnd3[2]));
                    if (ImGui::ColorEdit3(u8"Màu nắng", sun3)) scene.setSunColor(glm::vec3(sun3[0], sun3[1], sun3[2]));
                    glm::vec3 ld = scene.lightDir();
                    float lx = ld.x, ly = ld.y, lz = ld.z;
                    if (ImGui::DragFloat3(u8"Hướng tia sáng", &lx, 0.01f)) scene.setLightDir(glm::vec3(lx, ly, lz));
                    ImGui::EndDisabled();

                    ImGui::Separator();
                    ImGui::Text(u8"--- Tòa nhà (Đêm) ---");
                    float em = scene.windowEmissive();
                    if (ImGui::SliderFloat(u8"Độ sáng cửa sổ", &em, 0.0f, 5.0f)) scene.setWindowEmissive(em);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(u8"Hậu kỳ")) {
                    ImGui::SliderFloat(u8"Bloom Threshold", &bloomThreshold, 0.5f, 5.0f);
                    ImGui::SliderFloat(u8"Bloom Intensity", &bloomIntensity, 0.0f, 2.0f);
                    ImGui::SliderFloat(u8"God Ray (Sun)", &godRayIntensity, 0.0f, 1.5f);
                    ImGui::SliderFloat(u8"Color Grade (Lạnh)", &colorGradeIntensity, 0.0f, 1.0f);
                    ImGui::SliderFloat(u8"Vignette (Viền tối)", &vignetteIntensity, 0.0f, 1.0f);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(u8"Hệ thống")) {
                    ImGui::Text(u8"--- Xe & Giao thông ---");
                    ImGui::Text(u8"Model xe: %d | Xe trên đường: %d", scene.carModelCount(), scene.carCount());
                    
                    ImGui::Checkbox(u8"Chỉ hiển thị xe (Debug View)", &scene.m_debugDrawCarOnly);
                    if (scene.m_debugDrawCarOnly && scene.carModelCount() > 0) {
                        ImGui::SliderInt(u8"Xem model số", &scene.m_debugCarIndex, 0, scene.carModelCount() - 1);
                    }

                    float cs = scene.carScale();
                    if (ImGui::SliderFloat(u8"Tỷ lệ xe (Scale)", &cs, 1.0f, 200.0f, "%.1f")) scene.setCarScale(cs);
                    float cy = scene.carYOffset();
                    if (ImGui::SliderFloat(u8"Độ cao xe (Y Offset)", &cy, -5.0f, 5.0f, "%.2f")) scene.setCarYOffset(cy);
                    ImGui::Separator();
                    ImGui::Text(u8"--- Camera ---");
                    float spd = camera.moveSpeed();
                    if (ImGui::SliderFloat(u8"Tốc độ di chuyển", &spd, 2.0f, 80.0f)) camera.setMoveSpeed(spd);
                    float sens = camera.mouseSensitivity();
                    if (ImGui::SliderFloat(u8"Độ nhạy chuột", &sens, 0.02f, 0.35f)) camera.setMouseSensitivity(sens);
                    float fov = camera.fovDegrees();
                    if (ImGui::SliderFloat(u8"Trường nhìn FOV", &fov, 20.0f, 90.0f)) camera.setFovDegrees(fov);
                    if (ImGui::Button(u8"Đặt lại vị trí camera")) camera = Camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);
                    
                    ImGui::Separator();
                    ImGui::Text(u8"--- Bố cục ---");
                    ImGui::Checkbox(u8"Lưới nhiều bản sao", &useGridLayout);
                    if (useGridLayout) {
                        ImGui::SliderInt(u8"Hàng", &gridRows, 1, 10);
                        ImGui::SliderInt(u8"Cột", &gridCols, 1, 10);
                        ImGui::SliderFloat(u8"Khoảng cách X", &gridSpaceX, 20.0f, 200.0f);
                        ImGui::SliderFloat(u8"Khoảng cách Z", &gridSpaceZ, 20.0f, 250.0f);
                    }
                    if (ImGui::Button(u8"Áp dụng bố cục")) {
                        if (useGridLayout) scene.setInstanceTransforms(CityBuilder::buildGrid(gridRows, gridCols, gridSpaceX, gridSpaceZ, 0.0f));
                        else scene.setInstanceTransforms({glm::mat4(1.0f)});
                    }

                    ImGui::Separator();
                    ImGui::Text(u8"--- Hiển thị ---");
                    if (ImGui::Checkbox(u8"Khung dây", &wireframe)) glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
                    if (ImGui::Checkbox(u8"Loại mặt sau (cull)", &cullBack)) {
                        if (cullBack) glEnable(GL_CULL_FACE);
                        else glDisable(GL_CULL_FACE);
                    }
                    if (ImGui::Checkbox(u8"VSync", &vsync)) glfwSwapInterval(vsync ? 1 : 0);
                    
                    ImGui::BeginDisabled(syncTimeOfDay);
                    ImGui::ColorEdit3(u8"Màu nền trời", clearCol);
                    ImGui::EndDisabled();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();
            ImGui::PopStyleColor();
        }

        ImGui::Render();

        if (syncTimeOfDay) {
            applyTimeOfDayHour(scene, timeOfDayHour, clearCol);
        }

        {
            static bool menuWas = false;
            if (menuWas && !showMenu) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                g_keys.firstMouse = true;
            }
            menuWas = showMenu;
        }

        if (!showMenu || !imguiIo.WantCaptureKeyboard) {
            if (g_keys.w) {
                camera.processKeyboard(Camera::Forward, g_deltaTime);
            }
            if (g_keys.s) {
                camera.processKeyboard(Camera::Backward, g_deltaTime);
            }
            if (g_keys.a) {
                camera.processKeyboard(Camera::Left, g_deltaTime);
            }
            if (g_keys.d) {
                camera.processKeyboard(Camera::Right, g_deltaTime);
            }
            if (g_keys.space) {
                camera.processKeyboard(Camera::Up, g_deltaTime);
            }
            if (g_keys.shift) {
                camera.processKeyboard(Camera::Down, g_deltaTime);
            }
        }

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        ensureOffscreenFbo(fbW, fbH);

        if (g_offscreenFbo != 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, g_offscreenFbo);
            glViewport(0, 0, fbW, fbH);
            glClearColor(clearCol[0], clearCol[1], clearCol[2], 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shader.use();
            shader.setFloat("uTime", static_cast<float>(now));
            scene.render(shader, camera);

            // 3D GPU Rain
            if (rainIntensity > 0.001f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending
                glDepthMask(GL_FALSE); // Read-only depth
                
                rainShader.use();
                rainShader.setMat4("uProjection", camera.projectionMatrix(static_cast<float>(fbW)/std::max(1, fbH)));
                rainShader.setMat4("uView", camera.viewMatrix());
                rainShader.setVec3("uCameraPos", camera.position());
                rainShader.setFloat("uTime", static_cast<float>(now));
                rainShader.setVec2("uWindDir", windDir);
                rainShader.setFloat("uFallSpeed", 35.0f);
                rainShader.setFloat("uRainLength", 1.5f + rainIntensity * 3.0f);
                rainShader.setFloat("uRainIntensity", rainIntensity);
                
                glBindVertexArray(rainVAO);
                int dropCount = static_cast<int>(rainIntensity * numRainDrops);
                glDrawArraysInstanced(GL_LINES, 0, 2, dropCount);
                glBindVertexArray(0);
                
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }

            // Bloom passes
            if (hasBloom && bloomIntensity > 0.001f && g_bloomFbo[0] != 0) {
                glDisable(GL_DEPTH_TEST);
                // Extract bright areas
                glBindFramebuffer(GL_FRAMEBUFFER, g_bloomFbo[0]);
                glViewport(0, 0, g_bloomW, g_bloomH);
                bloomExtractShader.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, g_offscreenColor);
                bloomExtractShader.setInt("uScene", 0);
                bloomExtractShader.setFloat("uThreshold", bloomThreshold);
                glBindVertexArray(g_emptyVao);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                
                // Blur (Ping-pong 4 times)
                bloomBlurShader.use();
                bloomBlurShader.setInt("uInput", 0);
                for (int i = 0; i < 4; ++i) {
                    int ping = i % 2;
                    int pong = (i + 1) % 2;
                    glBindFramebuffer(GL_FRAMEBUFFER, g_bloomFbo[pong]);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, g_bloomTex[ping]);
                    bloomBlurShader.setVec2("uDirection", ping == 0 ? glm::vec2(1.0f / g_bloomW, 0.0f) : glm::vec2(0.0f, 1.0f / g_bloomH));
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                }
                glEnable(GL_DEPTH_TEST);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbW, fbH);
            glDisable(GL_DEPTH_TEST);
            postShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_offscreenColor);
            postShader.setInt("uScene", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, g_offscreenDepth);
            postShader.setInt("uDepth", 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, g_bloomTex[0]);
            postShader.setInt("uBloom", 2);

            postShader.setVec2("uResolution", glm::vec2(static_cast<float>(fbW), static_cast<float>(fbH)));
            postShader.setFloat("uTime", static_cast<float>(now));
            postShader.setFloat("uRainIntensity", rainIntensity);
            const float aspect = static_cast<float>(fbW) / static_cast<float>(std::max(1, fbH));
            const glm::mat4 proj = camera.projectionMatrix(aspect);
            const glm::mat4 view = camera.viewMatrix();
            postShader.setMat4("uInvProj", glm::inverse(proj));
            postShader.setMat4("uInvView", glm::inverse(view));
            postShader.setFloat("uNear", camera.nearPlane());
            postShader.setFloat("uFar", camera.farPlane());
            
            glm::vec3 sunDir = glm::normalize(-scene.lightDir());
            const float sunElev = sunDir.y;
            const float sunIntensity = glm::clamp((sunElev + 0.07f) / 0.24f, 0.f, 1.f);
            postShader.setVec3("uSunDir", sunDir);
            postShader.setFloat("uSunIntensity", sunIntensity);

            // Set cinematic uniforms
            postShader.setFloat("uBloomIntensity", bloomIntensity);
            postShader.setFloat("uGodRayIntensity", godRayIntensity);
            postShader.setFloat("uColorGradeIntensity", colorGradeIntensity);
            postShader.setFloat("uVignetteIntensity", vignetteIntensity);
            postShader.setVec2("uWindDir", windDir);
            
            // Calculate screen space sun pos for god rays
            glm::vec4 sunClip = proj * view * glm::vec4(sunDir * 1000.0f, 1.0f);
            glm::vec2 sunNdc = glm::vec2(sunClip.x, sunClip.y) / sunClip.w;
            glm::vec2 sunScreen = (sunNdc * 0.5f + 0.5f);
            postShader.setVec2("uSunScreenPos", sunScreen);

            glBindVertexArray(g_emptyVao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            glEnable(GL_DEPTH_TEST);
        } else {
            glViewport(0, 0, fbW, fbH);
            glClearColor(clearCol[0], clearCol[1], clearCol[2], 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene.render(shader, camera);
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyOffscreenFbo();
    if (g_emptyVao != 0) {
        glDeleteVertexArrays(1, &g_emptyVao);
        g_emptyVao = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
