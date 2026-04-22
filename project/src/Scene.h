#pragma once

#include <algorithm>

#include "Camera.h"
#include "Model.h"
#include "Shader.h"

#include <glm/glm.hpp>

#include <vector>

// Owns the shared city model, lighting parameters, and per-instance transforms.
class Scene {
public:
    bool loadCityModel(const std::string& glbPath);
    void setInstanceTransforms(std::vector<glm::mat4> transforms);

    void setAspect(float aspect) { m_aspect = aspect; }

    void render(Shader& shader, const Camera& camera) const;

    Model& cityModel() { return m_cityModel; }
    const Model& cityModel() const { return m_cityModel; }

    float exposure() const { return m_exposure; }
    void setExposure(float v) { m_exposure = std::max(0.05f, v); }

    glm::vec3 skyColor() const { return m_skyColor; }
    void setSkyColor(const glm::vec3& c) { m_skyColor = c; }
    glm::vec3 groundColor() const { return m_groundColor; }
    void setGroundColor(const glm::vec3& c) { m_groundColor = c; }
    glm::vec3 sunColor() const { return m_sunColor; }
    void setSunColor(const glm::vec3& c) { m_sunColor = c; }
    glm::vec3 lightDir() const { return m_lightDir; }
    void setLightDir(const glm::vec3& d) { m_lightDir = d; }

    glm::vec3 fogColor() const { return m_fogColor; }
    void setFogColor(const glm::vec3& c) { m_fogColor = c; }
    float fogDensity() const { return m_fogDensity; }
    void setFogDensity(float d) { m_fogDensity = std::max(0.0f, d); }

private:
    Model m_cityModel;
    std::vector<glm::mat4> m_instanceTransforms;
    float m_aspect = 16.0f / 9.0f;

    // Hướng nắng (vector “ánh sáng tới cảnh” — shader dùng L = normalize(-uLightDir)).
    glm::vec3 m_lightDir{0.4f, -0.92f, 0.15f};
    // Ambient bán cầu: trời (lạnh/sáng) + phản chiếu mặt đất (ấm).
    glm::vec3 m_skyColor{0.38f, 0.48f, 0.62f};
    glm::vec3 m_groundColor{0.20f, 0.18f, 0.16f};
    glm::vec3 m_sunColor{1.05f, 0.97f, 0.88f};
    float m_exposure = 1.18f;

    glm::vec3 m_fogColor{0.45f, 0.52f, 0.62f};
    float m_fogDensity = 0.0f;
};
