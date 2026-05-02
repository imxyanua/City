#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <vector>

// Fly camera: WASD movement, mouse look, scroll zoom (FOV).
class Camera {
public:
    Camera(glm::vec3 position, float yawDegrees, float pitchDegrees);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

    void processKeyboard(int direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
    void processScroll(float yoffset);

    glm::vec3 position() const { return m_position; }
    float fovDegrees() const { return m_fovDeg; }
    float moveSpeed() const { return m_moveSpeed; }
    float mouseSensitivity() const { return m_mouseSensitivity; }

    void setMoveSpeed(float s) { m_moveSpeed = std::max(0.5f, s); }
    void setMouseSensitivity(float s) { m_mouseSensitivity = std::max(0.01f, s); }
    void setFovDegrees(float f);

    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setYawPitch(float yaw, float pitch);

    float yawDegrees() const { return m_yaw; }
    float pitchDegrees() const { return m_pitch; }

    float nearPlane() const { return m_near; }
    float farPlane() const { return m_far; }

    enum Direction { Forward, Backward, Left, Right, Up, Down };

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_moveSpeed;
    float m_mouseSensitivity;
    float m_fovDeg;
    float m_near;
    float m_far;
};
