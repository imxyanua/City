#include "Scene.h"

#include <iostream>

bool Scene::loadCityModel(const std::string& glbPath)
{
    if (!m_cityModel.loadFromGLB(glbPath)) {
        std::cerr << "[Scene] Failed to load GLB: " << glbPath << "\n";
        return false;
    }
    return true;
}

void Scene::setInstanceTransforms(std::vector<glm::mat4> transforms)
{
    m_instanceTransforms = std::move(transforms);
}

void Scene::render(Shader& shader, const Camera& camera) const
{
    shader.use();
    shader.setMat4("uView", camera.viewMatrix());
    shader.setMat4("uProjection", camera.projectionMatrix(m_aspect));
    shader.setVec3("uLightDir", glm::normalize(m_lightDir));
    shader.setVec3("uSkyColor", m_skyColor);
    shader.setVec3("uGroundColor", m_groundColor);
    shader.setVec3("uSunColor", m_sunColor);
    shader.setFloat("uExposure", m_exposure);
    shader.setVec3("uViewPos", camera.position());
    shader.setVec3("uFogColor", m_fogColor);
    shader.setFloat("uFogDensity", m_fogDensity);

    for (const glm::mat4& world : m_instanceTransforms) {
        m_cityModel.draw(shader, world);
    }
}
