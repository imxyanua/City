#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct AppOptions;
class Camera;
class Scene;
struct ImGuiIO;

/// Input + chế độ camera (free, follow, drive, CCTV, cinematic). Đăng ký qua GLFW user pointer = `this`.
class CameraController {
public:
    void bind(GLFWwindow* window, Camera* camera, Scene* scene, AppOptions* opts);

    void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    void handleMouse(GLFWwindow* window, double x, double y);
    void handleScroll(GLFWwindow* window, double xoff, double yoff);
    void handleFramebufferSize(GLFWwindow* window, int width, int height);

    void update(GLFWwindow* window, const ImGuiIO& io, float dt, double timeSec);

private:
    struct KeyState {
        bool w = false, a = false, s = false, d = false;
        bool space = false, shift = false;
        bool firstMouse = true;
        double lastX = 400.0, lastY = 300.0;
    };

    void onMenuVisibilityChanged(GLFWwindow* window, bool menuOpen);

    Camera* m_camera = nullptr;
    Scene* m_scene = nullptr;
    AppOptions* m_opts = nullptr;
    KeyState m_keys;
    bool m_menuWas = false;

    int m_prevCameraMode = -1;
    glm::vec3 m_cineDampedPos{0.0f};
    float m_cineSmYaw = 0.0f;
    float m_cineSmPitch = 0.0f;
};
