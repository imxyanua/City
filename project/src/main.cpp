#include "Camera.h"
#include "CityBuilder.h"
#include "Scene.h"
#include "Shader.h"
#include "AppOptions.h"
#include "RainSystem.h"
#include "PostProcessor.h"
#include "UIManager.h"
#include "ParticleSystem.h"

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
#include <ctime>

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
    bool space = false, shift = false, f = false;
    bool firstMouse = true;
    double lastX = 400.0, lastY = 300.0;
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

    if (key == GLFW_KEY_F && action == GLFW_PRESS && ctx && ctx->opts && ctx->scene) {
        auto& cars = ctx->scene->getCarsRef();
        if (ctx->opts->cameraMode != 3) {
            ctx->opts->cameraMode = 3;
            if (!cars.empty()) cars[0].isPlayerDriven = true;
        } else {
            ctx->opts->cameraMode = 0;
            if (!cars.empty()) cars[0].isPlayerDriven = false;
        }
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

    if (ctx->opts && ctx->opts->cameraMode == 0) {
        ctx->camera->processMouseMovement(xoff, yoff);
    }
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

void destroyShadowMap(unsigned int& fbo, unsigned int& depthTex)
{
    if (depthTex != 0) {
        glDeleteTextures(1, &depthTex);
        depthTex = 0;
    }
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}

void recreateShadowMap(int dim, unsigned int& fbo, unsigned int& depthTex)
{
    destroyShadowMap(fbo, depthTex);
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, dim, dim, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    AppOptions opts;
    loadConfig(opts, (exeDir / "config.json").string());

    glfwSwapInterval(opts.vsync ? 1 : 0);
    glPolygonMode(GL_FRONT_AND_BACK, opts.wireframe ? GL_LINE : GL_FILL);
    if (opts.cullBack) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;
    recreateShadowMap(sanitizeShadowResolution(opts.shadowMapResolution), depthMapFBO, depthMap);
    opts.shadowMapResolution = sanitizeShadowResolution(opts.shadowMapResolution);

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

    if (opts.useGridLayout)
        scene.setInstanceTransforms(CityBuilder::buildGrid(opts.gridRows, opts.gridCols, opts.gridSpaceX, opts.gridSpaceZ, 0.0f));
    else
        scene.setInstanceTransforms({glm::mat4(1.0f)});
    scene.initTrafficAndPedestrians();

    Camera camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);
    camera.setMoveSpeed(25.0f);

    UIManager uiManager;
    RainSystem rainSystem;
    rainSystem.init(sanitizeRainMaxDrops(opts.rainMaxDrops));
    opts.rainMaxDrops = sanitizeRainMaxDrops(opts.rainMaxDrops);


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
        rainSystem.update(g_deltaTime, camera, opts.rainIntensity, opts.windDir, opts.cloudHeight);


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        uiManager.render(opts, scene, camera, window, imguiIo);
        ImGui::Render();

        const int shadowDim = sanitizeShadowResolution(opts.shadowMapResolution);
        opts.shadowMapResolution = shadowDim;
        static int appliedShadowDim = shadowDim;
        if (shadowDim != appliedShadowDim) {
            recreateShadowMap(shadowDim, depthMapFBO, depthMap);
            appliedShadowDim = shadowDim;
        }

        const int rainCap = sanitizeRainMaxDrops(opts.rainMaxDrops);
        opts.rainMaxDrops = rainCap;
        static int appliedRainCap = rainCap;
        if (rainCap != appliedRainCap) {
            rainSystem.init(rainCap);
            appliedRainCap = rainCap;
        }

        if (opts.syncTimeOfDay) {
            std::time_t t = std::time(nullptr);
            std::tm local_tm;
            localtime_s(&local_tm, &t);
            opts.timeOfDayHour = local_tm.tm_hour + local_tm.tm_min / 60.0f + local_tm.tm_sec / 3600.0f;
        }
        applyTimeOfDayHour(scene, opts.timeOfDayHour, opts.clearCol);

        // --- Lightning Storm ---
        float lightningIntensity = 0.0f;
        if (opts.rainIntensity > 0.5f) {
            float noise = sin(static_cast<float>(now) * 15.0f) * sin(static_cast<float>(now) * 31.0f) * cos(static_cast<float>(now) * 53.0f);
            if (noise > 0.85f && sin(static_cast<float>(now) * 2.0f) > 0.5f) {
                lightningIntensity = (noise - 0.85f) * 6.6f; // map 0.85-1.0 to 0-1
            }
        }
        scene.setLightning(lightningIntensity);

        {
            static bool menuWas = false;
            if (menuWas && !opts.showMenu) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                g_keys.firstMouse = true;
            }
            menuWas = opts.showMenu;
        }

        if (!opts.showMenu || !imguiIo.WantCaptureKeyboard) {
            if (opts.cameraMode == 0) {
                if (g_keys.w) camera.processKeyboard(Camera::Forward, g_deltaTime);
                if (g_keys.s) camera.processKeyboard(Camera::Backward, g_deltaTime);
                if (g_keys.a) camera.processKeyboard(Camera::Left, g_deltaTime);
                if (g_keys.d) camera.processKeyboard(Camera::Right, g_deltaTime);
                if (g_keys.space) camera.processKeyboard(Camera::Up, g_deltaTime);
                if (g_keys.shift) camera.processKeyboard(Camera::Down, g_deltaTime);
            }
        }
        
        // --- Auto Camera Modes ---
        if (opts.cameraMode == 1 || opts.cameraMode == 3) { // Follow Car or Drive
            auto& cars = scene.getCarsRef();
            if (!cars.empty()) {
                auto& car = cars[0]; // Follow the first car
                
                if (opts.cameraMode == 3) {
                    car.isPlayerDriven = true;
                    if (g_keys.w) car.targetSpeed = 30.0f;
                    else if (g_keys.s) car.targetSpeed = -15.0f;
                    else car.targetSpeed = 0.0f;

                    if (abs(car.speed) > 1.0f) {
                        float steer = 0.0f;
                        if (g_keys.a) steer = 1.0f;
                        if (g_keys.d) steer = -1.0f;
                        float steerMult = (car.speed > 0) ? 1.0f : -1.0f;
                        car.rotOffset += steer * steerMult * 1.5f * g_deltaTime;
                        car.dir = glm::vec3(sin(car.rotOffset), 0.0f, cos(car.rotOffset));
                    }
                }

                glm::vec3 targetPos = car.pos + glm::vec3(0.0f, 1.5f, 0.0f);
                glm::vec3 camPos = targetPos - car.dir * 6.0f + glm::vec3(0.0f, 2.5f, 0.0f);
                
                // Detect wrap-around teleportation to prevent camera "flying" across the map
                if (glm::distance(camera.position(), camPos) > 40.0f) {
                    camera.setPosition(camPos);
                } else {
                    camera.setPosition(glm::mix(camera.position(), camPos, g_deltaTime * 5.0f));
                }
                
                glm::vec3 dir = glm::normalize(targetPos - camera.position());
                float yaw = glm::degrees(atan2(dir.z, dir.x));
                float pitch = glm::degrees(asin(dir.y));
                camera.setYawPitch(yaw, pitch);
            }
        } else if (opts.cameraMode == 2) { // CCTV
            glm::vec3 cctvPos = glm::vec3(12.0f, 15.0f, 12.0f);
            camera.setPosition(glm::mix(camera.position(), cctvPos, g_deltaTime * 2.0f));
            
            glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 dir = glm::normalize(targetPos - camera.position());
            
            float yaw = glm::degrees(atan2(dir.z, dir.x));
            yaw += sin(static_cast<float>(now) * 0.2f) * 15.0f; // Panning
            float pitch = glm::degrees(asin(dir.y));
            
            camera.setYawPitch(yaw, pitch);
        } else if (opts.cameraMode == 4) { // Cinematic
            float t = static_cast<float>(now) * 0.15f;
            float radius = 60.0f + sin(t * 0.8f) * 30.0f;
            float height = 25.0f + cos(t * 1.3f) * 15.0f;
            
            glm::vec3 cinePos = glm::vec3(sin(t) * radius, height, cos(t) * radius);
            camera.setPosition(glm::mix(camera.position(), cinePos, g_deltaTime * 1.5f));
            
            glm::vec3 targetPos = glm::vec3(0.0f, 5.0f, 0.0f); // Look slightly above ground center
            glm::vec3 dir = glm::normalize(targetPos - camera.position());
            
            float yaw = glm::degrees(atan2(dir.z, dir.x));
            float pitch = glm::degrees(asin(dir.y));
            
            camera.setYawPitch(yaw, pitch);
        }

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        
        // --- 1. Render Shadow Map ---
        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = -50.0f, far_plane = 200.0f;
        lightProjection = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, near_plane, far_plane);
        glm::vec3 lightDir = scene.lightDir();
        lightView = glm::lookAt(lightDir * 50.0f, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, opts.shadowMapResolution, opts.shadowMapResolution);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT); // Avoid peter panning
        shadowShader.use();
        shadowShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
        scene.render(shadowShader, camera);
        glCullFace(GL_BACK);

        // --- 2. Render Main Scene ---
        glViewport(0, 0, fbW, fbH);
        postProcessor.resize(fbW, fbH);
        postProcessor.beginOffscreen(opts.clearCol);

        shader.use();
        shader.setFloat("uTime", static_cast<float>(now));
        shader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
        
        // Bind shadow map to texture unit 1 (0 is used by models)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shader.setInt("uShadowMap", 1);
        
        scene.render(shader, camera);

        // Switch to single draw buffer for transparent passes (rain, particles)
        // They don't output normals, so avoid writing garbage to normal buffer
        GLenum singleBuf[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, singleBuf);

        rainSystem.render(rainShader, camera, opts.rainIntensity, opts.windDir, fbW, fbH);
        
        scene.renderParticles(particleShader, camera);

        // Restore MRT for post-processing readback
        GLenum dualBuf[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, dualBuf);

        postProcessor.renderSSAO(camera);

        postProcessor.renderBloom(opts.bloomThreshold);
        postProcessor.renderPost(camera, scene, opts, static_cast<float>(now));

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    saveConfig(opts, (exeDir / "config.json").string());

    destroyShadowMap(depthMapFBO, depthMap);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
