#include "RainSystem.h"

#include <vector>
#include <random>
#include <algorithm>

RainSystem::RainSystem() {}

RainSystem::~RainSystem() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void RainSystem::init(int numDrops) {
    m_numDrops = numDrops;

    std::vector<glm::vec3> rainOffsets(numDrops);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> disX(-75.0f, 75.0f);
    std::uniform_real_distribution<float> disY(-20.0f, 100.0f);
    std::uniform_real_distribution<float> disZ(-75.0f, 75.0f);

    for (int i = 0; i < numDrops; ++i) {
        rainOffsets[i] = glm::vec3(disX(gen), disY(gen), disZ(gen));
    }
    
    float rainVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rainVertices), rainVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glGenBuffers(1, &m_instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, rainOffsets.size() * sizeof(glm::vec3), rainOffsets.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    
    glBindVertexArray(0);
}

void RainSystem::render(Shader& rainShader, const Camera& camera, float intensity, const glm::vec2& windDir, float time, int fbW, int fbH) {
    if (intensity < 0.001f || m_numDrops == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending
    glDepthMask(GL_FALSE); // Read-only depth
    
    rainShader.use();
    rainShader.setMat4("uProjection", camera.projectionMatrix(static_cast<float>(fbW)/std::max(1, fbH)));
    rainShader.setMat4("uView", camera.viewMatrix());
    rainShader.setVec3("uCameraPos", camera.position());
    rainShader.setFloat("uTime", time);
    rainShader.setVec2("uWindDir", windDir);
    rainShader.setFloat("uFallSpeed", 35.0f);
    rainShader.setFloat("uRainLength", 1.5f + intensity * 3.0f);
    rainShader.setFloat("uRainIntensity", intensity);
    
    glBindVertexArray(m_vao);
    int dropCount = static_cast<int>(intensity * m_numDrops);
    glDrawArraysInstanced(GL_LINES, 0, 2, dropCount);
    glBindVertexArray(0);
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
