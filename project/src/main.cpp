#include "Camera.h"
#include "CityBuilder.h"
#include "Scene.h"
#include "Shader.h"
#include "AppOptions.h"
#include "CameraController.h"
#include "RainSystem.h"
#include "PostProcessor.h"
#include "RenderPipeline.h"
#include "TimeOfDay.h"
#include "UIManager.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path exeDirectory(const char* argv0)
{
    std::error_code ec;
    std::filesystem::path p(argv0);
    if (!p.empty() && std::filesystem::is_regular_file(p, ec)) {
        return p.parent_path();
    }
    return std::filesystem::current_path();
}

} // namespace

int main(int argc, char* argv[])
{
    const std::filesystem::path exeDir = exeDirectory(argc > 0 ? argv[0] : "");
    AppOptions opts;
    loadConfig(opts, (exeDir / "config.json").string());
    const std::filesystem::path assetModel = exeDir / "assets" / "models" / opts.cityModelFile;
    const std::filesystem::path vertPath = exeDir / "shaders" / "vertex.glsl";
    const std::filesystem::path fragPath = exeDir / "shaders" / "fragment.glsl";
    const std::filesystem::path rainVertPath = exeDir / "shaders" / "rain.vert";
    const std::filesystem::path rainFragPath = exeDir / "shaders" / "rain.frag";

    if (!std::filesystem::exists(assetModel)) {
        std::cerr << "Missing city GLB. Place the file here (or set \"cityModelFile\" in config.json):\n  "
                  << assetModel.string() << "\n";
        return 1;
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string winTitle = reinterpret_cast<const char*>(u8"Phố thành phố | ");
    winTitle += opts.cityModelFile;
    GLFWwindow* window = glfwCreateWindow(1600, 900, winTitle.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

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

    const std::filesystem::path shadowVertPath = exeDir / "shaders" / "shadow.vert";
    const std::filesystem::path shadowFragPath = exeDir / "shaders" / "shadow.frag";
    Shader shadowShader;
    if (!shadowShader.loadFromFiles(shadowVertPath.string(), shadowFragPath.string())) {
        std::cerr << "Shadow shader failed.\n";
        return 1;
    }

    const std::filesystem::path particleVertPath = exeDir / "shaders" / "particle.vert";
    const std::filesystem::path particleFragPath = exeDir / "shaders" / "particle.frag";
    Shader particleShader;
    if (!particleShader.loadFromFiles(particleVertPath.string(), particleFragPath.string())) {
        std::cerr << "Particle shader failed.\n";
        return 1;
    }

    glfwSwapInterval(opts.vsync ? 1 : 0);
    glPolygonMode(GL_FRONT_AND_BACK, opts.wireframe ? GL_LINE : GL_FILL);
    if (opts.cullBack) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    RenderPipeline renderPipeline;
    renderPipeline.initShadow(opts);

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

    if (opts.useGridLayout) {
        scene.setInstanceTransforms(CityBuilder::buildGrid(opts.gridRows, opts.gridCols, opts.gridSpaceX, opts.gridSpaceZ, 0.0f));
    } else {
        scene.setInstanceTransforms({glm::mat4(1.0f)});
    }
    scene.initTrafficAndPedestrians();

    Camera camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);
    camera.setMoveSpeed(25.0f);

    CameraController cameraControl;
    cameraControl.bind(window, &camera, &scene, &opts);

    UIManager uiManager;
    RainSystem rainSystem;
    rainSystem.init(sanitizeRainMaxDrops(opts.rainMaxDrops));
    opts.rainMaxDrops = sanitizeRainMaxDrops(opts.rainMaxDrops);
    renderPipeline.noteRainCapsMatchingOpts(opts);

    applyTimeOfDayHour(scene, opts.timeOfDayHour, opts.clearCol);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << u8"Tab: bật/tắt menu | WASD + chuột (khi menu tắt) | Esc: đóng menu / thoát\n";

    double lastFrame = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        const float deltaTime = static_cast<float>(now - lastFrame);
        lastFrame = now;

        scene.update(deltaTime);
        rainSystem.update(deltaTime, camera, opts.rainIntensity, opts.windDir, opts.cloudHeight);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        uiManager.render(opts, scene, camera, window, imguiIo);
        ImGui::Render();

        renderPipeline.syncDynamicResources(opts, rainSystem);

        if (opts.syncTimeOfDay) {
            std::time_t t = std::time(nullptr);
            std::tm local_tm;
            localtime_s(&local_tm, &t);
            opts.timeOfDayHour = local_tm.tm_hour + local_tm.tm_min / 60.0f + local_tm.tm_sec / 3600.0f;
        }
        applyTimeOfDayHour(scene, opts.timeOfDayHour, opts.clearCol);

        float lightningIntensity = 0.0f;
        if (opts.rainIntensity > 0.5f) {
            float noise = std::sin(static_cast<float>(now) * 15.0f) * std::sin(static_cast<float>(now) * 31.0f)
                        * std::cos(static_cast<float>(now) * 53.0f);
            if (noise > 0.85f && std::sin(static_cast<float>(now) * 2.0f) > 0.5f) {
                lightningIntensity = (noise - 0.85f) * 6.6f;
            }
        }
        scene.setLightning(lightningIntensity);

        cameraControl.update(window, imguiIo, deltaTime, now);

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);

        renderPipeline.renderFrame(shader,
            rainShader,
            shadowShader,
            particleShader,
            postProcessor,
            scene,
            camera,
            rainSystem,
            opts,
            static_cast<float>(now),
            fbW,
            fbH);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    saveConfig(opts, (exeDir / "config.json").string());

    renderPipeline.shutdownGpu();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
