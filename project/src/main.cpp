#include "Camera.h"
#include "CityBuilder.h"
#include "Scene.h"
#include "Shader.h"
#include "AppOptions.h"
#include "RainSystem.h"
#include "PostProcessor.h"
#include "UIManager.h"

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
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

struct AppContext {
    Camera* camera = nullptr;
    Scene* scene = nullptr;
    AppOptions* opts = nullptr;
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

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS && ctx && ctx->opts != nullptr) {
        ctx->opts->showMenu = !(ctx->opts->showMenu);
        glfwSetInputMode(window, GLFW_CURSOR, ctx->opts->showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        if (!ctx->opts->showMenu) {
            g_keys.firstMouse = true;
        }
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (ctx && ctx->opts && ctx->opts->showMenu) {
            ctx->opts->showMenu = false;
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

    if (key == GLFW_KEY_W) g_keys.w = down;
    if (key == GLFW_KEY_S) g_keys.s = down;
    if (key == GLFW_KEY_A) g_keys.a = down;
    if (key == GLFW_KEY_D) g_keys.d = down;
    if (key == GLFW_KEY_SPACE) g_keys.space = down;
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) g_keys.shift = down;
}

void mouseCallback(GLFWwindow* window, double x, double y)
{
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->camera) return;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    if (ctx->opts && ctx->opts->showMenu) return;

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
    if (!ctx || !ctx->camera) return;
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
    
    ctx->camera->processScroll(static_cast<float>(yoff));
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (height <= 0) return;
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

void applyTimeOfDayHour(Scene& scene, float hour, float clearCol[3])
{
    float h = std::fmod(hour, 24.0f);
    if (h < 0.0f) h += 24.0f;

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
    if (h >= 5.0f && h < 8.5f) twi = glm::smoothstep(5.0f, 8.0f, h);
    else if (h >= 17.0f && h < 20.5f) twi = glm::smoothstep(20.5f, 17.0f, h);

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
    const std::filesystem::path rainVertPath = exeDir / "shaders" / "rain.vert";
    const std::filesystem::path rainFragPath = exeDir / "shaders" / "rain.frag";

    if (!std::filesystem::exists(assetModel)) {
        std::cerr << "Missing GLB model. Expected:\n  " << assetModel.string() << "\n";
        return 1;
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1600, 900, u8"Phố thành phố — procedural_city_7.glb", nullptr, nullptr);
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
        std::cerr << "Main shader failed.\n";
        return 1;
    }
    
    Shader rainShader;
    if (!rainShader.loadFromFiles(rainVertPath.string(), rainFragPath.string())) {
        std::cerr << "Rain shader failed.\n";
        return 1;
    }

    PostProcessor postProcessor;
    if (!postProcessor.init(exeDir.string())) {
        std::cerr << "PostProcessor init failed.\n";
        return 1;
    }

    Scene scene;
    if (!scene.loadCityModel(assetModel.string())) {
        std::cerr << "Failed to load city.\n";
        return 1;
    }
    
    const std::filesystem::path assetsDir = exeDir / "assets" / "models";
    std::vector<std::string> carPaths = {
        (assetsDir / "2014_bmw_z4_sdrive35is.glb").string(),
        (assetsDir / "2021_lexus_bev_sport_concept.glb").string(),
        (assetsDir / "2022_nio_et7.glb").string()
    };
    scene.loadCarModels(carPaths);

    scene.setInstanceTransforms({glm::mat4(1.0f)});
    scene.initTrafficAndPedestrians();

    Camera camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);
    camera.setMoveSpeed(25.0f);

    AppOptions opts;
    UIManager uiManager;
    RainSystem rainSystem;
    rainSystem.init(50000);

    applyTimeOfDayHour(scene, opts.timeOfDayHour, opts.clearCol);

    AppContext ctx;
    ctx.camera = &camera;
    ctx.scene = &scene;
    ctx.opts = &opts;
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

        uiManager.render(opts, scene, camera, window, imguiIo);
        ImGui::Render();

        if (opts.syncTimeOfDay) {
            applyTimeOfDayHour(scene, opts.timeOfDayHour, opts.clearCol);
        }

        {
            static bool menuWas = false;
            if (menuWas && !opts.showMenu) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                g_keys.firstMouse = true;
            }
            menuWas = opts.showMenu;
        }

        if (!opts.showMenu || !imguiIo.WantCaptureKeyboard) {
            if (g_keys.w) camera.processKeyboard(Camera::Forward, g_deltaTime);
            if (g_keys.s) camera.processKeyboard(Camera::Backward, g_deltaTime);
            if (g_keys.a) camera.processKeyboard(Camera::Left, g_deltaTime);
            if (g_keys.d) camera.processKeyboard(Camera::Right, g_deltaTime);
            if (g_keys.space) camera.processKeyboard(Camera::Up, g_deltaTime);
            if (g_keys.shift) camera.processKeyboard(Camera::Down, g_deltaTime);
        }

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        
        postProcessor.resize(fbW, fbH);
        postProcessor.beginOffscreen(opts.clearCol);

        shader.use();
        shader.setFloat("uTime", static_cast<float>(now));
        scene.render(shader, camera);

        rainSystem.render(rainShader, camera, opts.rainIntensity, opts.windDir, static_cast<float>(now), fbW, fbH);

        postProcessor.renderBloom(opts.bloomThreshold);
        postProcessor.renderPost(camera, scene, opts, static_cast<float>(now));

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
