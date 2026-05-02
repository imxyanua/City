#include "CameraController.h"
#include "AppOptions.h"
#include "Camera.h"
#include "Scene.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {

void keyThunk(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* self = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleKey(window, key, scancode, action, mods);
    }
}

void mouseThunk(GLFWwindow* window, double x, double y)
{
    auto* self = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleMouse(window, x, y);
    }
}

void scrollThunk(GLFWwindow* window, double xoff, double yoff)
{
    auto* self = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleScroll(window, xoff, yoff);
    }
}

void fbThunk(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->handleFramebufferSize(window, width, height);
    }
}

} // namespace

void CameraController::bind(GLFWwindow* window, Camera* camera, Scene* scene, AppOptions* opts)
{
    m_camera = camera;
    m_scene = scene;
    m_opts = opts;
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyThunk);
    glfwSetCursorPosCallback(window, mouseThunk);
    glfwSetScrollCallback(window, scrollThunk);
    glfwSetFramebufferSizeCallback(window, fbThunk);
}

void CameraController::handleKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (!m_opts || !m_camera) {
        return;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        m_opts->showMenu = !m_opts->showMenu;
        glfwSetInputMode(window, GLFW_CURSOR, m_opts->showMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        if (!m_opts->showMenu) {
            m_keys.firstMouse = true;
        }
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_opts->showMenu) {
            m_opts->showMenu = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_keys.firstMouse = true;
            return;
        }
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS && m_scene) {
        auto& cars = m_scene->getCarsRef();
        if (m_opts->cameraMode != 3) {
            m_opts->cameraMode = 3;
            if (!cars.empty()) {
                cars[0].isPlayerDriven = true;
            }
        } else {
            m_opts->cameraMode = 0;
            if (!cars.empty()) {
                cars[0].isPlayerDriven = false;
            }
        }
    }

    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    const bool down = (action == GLFW_PRESS || action == GLFW_REPEAT);
    const bool capture = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;
    if (capture) {
        return;
    }

    if (key == GLFW_KEY_W) {
        m_keys.w = down;
    }
    if (key == GLFW_KEY_S) {
        m_keys.s = down;
    }
    if (key == GLFW_KEY_A) {
        m_keys.a = down;
    }
    if (key == GLFW_KEY_D) {
        m_keys.d = down;
    }
    if (key == GLFW_KEY_SPACE) {
        m_keys.space = down;
    }
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
        m_keys.shift = down;
    }
}

void CameraController::handleMouse(GLFWwindow* window, double x, double y)
{
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);

    if (!m_camera || !m_opts) {
        return;
    }
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    if (m_opts->showMenu) {
        return;
    }

    if (m_keys.firstMouse) {
        m_keys.lastX = x;
        m_keys.lastY = y;
        m_keys.firstMouse = false;
    }
    const float xoff = static_cast<float>(x - m_keys.lastX);
    const float yoff = static_cast<float>(m_keys.lastY - y);
    m_keys.lastX = x;
    m_keys.lastY = y;

    if (m_opts->cameraMode == 0) {
        m_camera->processMouseMovement(xoff, yoff);
    }
}

void CameraController::handleScroll(GLFWwindow* window, double xoff, double yoff)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoff, yoff);

    if (!m_camera) {
        return;
    }
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    m_camera->processScroll(static_cast<float>(yoff));
}

void CameraController::handleFramebufferSize(GLFWwindow* window, int width, int height)
{
    (void)window;
    if (height <= 0 || !m_scene) {
        return;
    }
    m_scene->setAspect(static_cast<float>(width) / static_cast<float>(height));
}

void CameraController::onMenuVisibilityChanged(GLFWwindow* window, bool menuOpen)
{
    if (m_menuWas && !menuOpen) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_keys.firstMouse = true;
    }
    m_menuWas = menuOpen;
}

void CameraController::update(GLFWwindow* window, const ImGuiIO& io, float dt, double timeSec)
{
    if (!m_camera || !m_scene || !m_opts || !window) {
        return;
    }

    onMenuVisibilityChanged(window, m_opts->showMenu);

    if (!m_opts->showMenu || !io.WantCaptureKeyboard) {
        if (m_opts->cameraMode == 0) {
            if (m_keys.w) {
                m_camera->processKeyboard(Camera::Forward, dt);
            }
            if (m_keys.s) {
                m_camera->processKeyboard(Camera::Backward, dt);
            }
            if (m_keys.a) {
                m_camera->processKeyboard(Camera::Left, dt);
            }
            if (m_keys.d) {
                m_camera->processKeyboard(Camera::Right, dt);
            }
            if (m_keys.space) {
                m_camera->processKeyboard(Camera::Up, dt);
            }
            if (m_keys.shift) {
                m_camera->processKeyboard(Camera::Down, dt);
            }
        }
    }

    if (m_opts->cameraMode == 1 || m_opts->cameraMode == 3) {
        auto& cars = m_scene->getCarsRef();
        if (!cars.empty()) {
            auto& car = cars[0];

            if (m_opts->cameraMode == 3) {
                car.isPlayerDriven = true;
                if (m_keys.w) {
                    car.targetSpeed = 30.0f;
                } else if (m_keys.s) {
                    car.targetSpeed = -15.0f;
                } else {
                    car.targetSpeed = 0.0f;
                }

                if (std::fabs(car.speed) > 1.0f) {
                    float steer = 0.0f;
                    if (m_keys.a) {
                        steer = 1.0f;
                    }
                    if (m_keys.d) {
                        steer = -1.0f;
                    }
                    float steerMult = (car.speed > 0) ? 1.0f : -1.0f;
                    car.rotOffset += steer * steerMult * 1.5f * dt;
                    car.dir = glm::vec3(std::sin(car.rotOffset), 0.0f, std::cos(car.rotOffset));
                }
            }

            glm::vec3 targetPos = car.pos + glm::vec3(0.0f, 1.5f, 0.0f);
            glm::vec3 camPos = targetPos - car.dir * 6.0f + glm::vec3(0.0f, 2.5f, 0.0f);

            if (glm::distance(m_camera->position(), camPos) > 40.0f) {
                m_camera->setPosition(camPos);
            } else {
                m_camera->setPosition(glm::mix(m_camera->position(), camPos, dt * 5.0f));
            }

            glm::vec3 dir = glm::normalize(targetPos - m_camera->position());
            float yaw = glm::degrees(std::atan2(dir.z, dir.x));
            float pitch = glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f)));
            m_camera->setYawPitch(yaw, pitch);
        }
    } else if (m_opts->cameraMode == 2) {
        glm::vec3 cctvPos = glm::vec3(12.0f, 15.0f, 12.0f);
        m_camera->setPosition(glm::mix(m_camera->position(), cctvPos, dt * 2.0f));

        glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 dir = glm::normalize(targetPos - m_camera->position());

        float yaw = glm::degrees(std::atan2(dir.z, dir.x));
        yaw += std::sin(static_cast<float>(timeSec) * 0.2f) * 15.0f;
        float pitch = glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f)));

        m_camera->setYawPitch(yaw, pitch);
    } else if (m_opts->cameraMode == 4) {
        if (m_prevCameraMode != 4) {
            m_cineDampedPos = m_camera->position();
            m_cineSmYaw = m_camera->yawDegrees();
            m_cineSmPitch = m_camera->pitchDegrees();
        }

        // Đường bay orbital nhẹ nhàng + low-pass exponential (ấm, “ảo” chứ không giật)
        const float t = static_cast<float>(timeSec) * 0.104f;
        const float radius = 68.0f + std::sin(t * 0.62f) * 26.0f;
        const float height = 22.0f + std::cos(t * 0.94f) * 12.0f;
        const glm::vec3 idealPos(std::sin(t) * radius, height, std::cos(t) * radius);

        constexpr float tauPos = 1.65f;
        constexpr float tauRot = 0.62f;
        const float alphaP = 1.0f - std::exp(-dt / tauPos);
        const float alphaR = 1.0f - std::exp(-dt / tauRot);

        m_cineDampedPos = glm::mix(m_cineDampedPos, idealPos, std::clamp(alphaP, 0.0f, 1.0f));
        m_camera->setPosition(m_cineDampedPos);

        const glm::vec3 targetPos(0.0f, 6.5f, 0.0f);
        glm::vec3 dir = targetPos - m_cineDampedPos;
        const float lenDir = glm::length(dir);
        if (lenDir > 1.0e-4f) {
            dir /= lenDir;
        } else {
            dir = glm::vec3(0.0f, 0.0f, -1.0f);
        }

        const float yawIdeal = glm::degrees(std::atan2(dir.z, dir.x));
        const float pitchIdeal = glm::degrees(std::asin(std::clamp(dir.y, -1.0f, 1.0f)));

        float yawDiff = yawIdeal - m_cineSmYaw;
        yawDiff -= 360.0f * std::floor((yawDiff + 180.0f) / 360.0f);
        m_cineSmYaw += yawDiff * std::clamp(alphaR, 0.0f, 1.0f);

        m_cineSmPitch += (pitchIdeal - m_cineSmPitch) * std::clamp(alphaR, 0.0f, 1.0f);

        m_camera->setYawPitch(m_cineSmYaw, m_cineSmPitch);
    }

    m_prevCameraMode = m_opts->cameraMode;
}
