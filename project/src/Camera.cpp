#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPitchLimit = 89.0f;
}

Camera::Camera(glm::vec3 position, float yawDegrees, float pitchDegrees)
    : m_position(position)
    , m_worldUp(0.0f, 1.0f, 0.0f)
    , m_yaw(yawDegrees)
    , m_pitch(pitchDegrees)
    , m_moveSpeed(18.0f)
    , m_mouseSensitivity(0.12f)
    , m_fovDeg(60.0f)
    , m_near(0.1f)
    , m_far(500.0f)
{
    updateVectors();
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix(float aspect) const
{
    return glm::perspective(glm::radians(m_fovDeg), aspect, m_near, m_far);
}

void Camera::processKeyboard(int direction, float deltaTime)
{
    float v = m_moveSpeed * deltaTime;
    if (direction == Forward)
        m_position += m_front * v;
    if (direction == Backward)
        m_position -= m_front * v;
    if (direction == Left)
        m_position -= m_right * v;
    if (direction == Right)
        m_position += m_right * v;
    if (direction == Up)
        m_position += m_worldUp * v;
    if (direction == Down)
        m_position -= m_worldUp * v;
}

void Camera::processMouseMovement(float xoffset, float yoffset)
{
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;
    m_pitch = std::clamp(m_pitch, -kPitchLimit, kPitchLimit);
    updateVectors();
}

void Camera::processScroll(float yoffset)
{
    m_fovDeg -= yoffset;
    m_fovDeg = std::clamp(m_fovDeg, 15.0f, 90.0f);
}

void Camera::setFovDegrees(float f)
{
    m_fovDeg = std::clamp(f, 15.0f, 90.0f);
}

void Camera::updateVectors()
{
    glm::vec3 dir;
    dir.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    dir.y = std::sin(glm::radians(m_pitch));
    dir.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(dir);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}
