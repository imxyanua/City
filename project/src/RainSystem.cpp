#include "RainSystem.h"
#include <algorithm>
#include <cstddef>

namespace {
    const float kSpawnRadius = 180.0f;
    const float kSpawnBottom = -15.0f;
    const float kCloudSpawnMinOffset = -3.0f;
    const float kCloudSpawnMaxOffset = 7.0f;
    const float kMinSpeed = 22.0f;
    const float kMaxSpeed = 36.0f;
    const float kMinLength = 1.2f;
    const float kMaxLength = 2.8f;
    const float kMinWidth = 0.012f;
    const float kMaxWidth = 0.028f;
    const float kWindStrength = 0.35f;
}

RainSystem::RainSystem() {
    std::random_device rd;
    m_rng.seed(rd());
}

RainSystem::~RainSystem() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void RainSystem::init(int maxDrops) {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_instanceVbo != 0) {
        glDeleteBuffers(1, &m_instanceVbo);
        m_instanceVbo = 0;
    }

    m_maxDrops = std::max(0, maxDrops);
    m_activeDrops = 0;
    m_hasCenter = false;
    m_drops.assign(m_maxDrops, RainDrop{});
    m_instances.resize(m_maxDrops);

    float quadVertices[] = {
        -0.5f, 0.0f,
         0.5f, 0.0f,
         0.5f, 1.0f,
        -0.5f, 0.0f,
         0.5f, 1.0f,
        -0.5f, 1.0f,
    };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &m_instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, m_maxDrops * sizeof(RainInstance), nullptr, GL_STREAM_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RainInstance), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RainInstance), (void*)offsetof(RainInstance, params));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
}

void RainSystem::respawnDrop(RainDrop& drop, const glm::vec3& center, float intensity, float cloudHeight) {
    std::uniform_real_distribution<float> disX(center.x - kSpawnRadius, center.x + kSpawnRadius);
    std::uniform_real_distribution<float> disZ(center.z - kSpawnRadius, center.z + kSpawnRadius);
    const float yMin = cloudHeight + kCloudSpawnMinOffset;
    const float yMax = cloudHeight + kCloudSpawnMaxOffset;
    std::uniform_real_distribution<float> disY(yMin, yMax);

    std::uniform_real_distribution<float> disSpeed(kMinSpeed, kMaxSpeed);
    std::uniform_real_distribution<float> disLength(kMinLength, kMaxLength);
    std::uniform_real_distribution<float> disWidth(kMinWidth, kMaxWidth);
    std::uniform_real_distribution<float> disDrift(-0.25f, 0.25f);

    drop.pos = glm::vec3(disX(m_rng), disY(m_rng), disZ(m_rng));
    drop.speed = disSpeed(m_rng);
    drop.length = disLength(m_rng) * (0.7f + intensity * 0.6f);
    drop.width = disWidth(m_rng);
    drop.drift = glm::vec2(disDrift(m_rng), disDrift(m_rng));
}

void RainSystem::update(float dt, const Camera& camera, float intensity, const glm::vec2& windDir, float cloudHeight) {
    if (m_maxDrops == 0) return;

    float clampedIntensity = std::clamp(intensity, 0.0f, 1.0f);
    if (clampedIntensity < 0.001f) {
        m_activeDrops = 0;
        return;
    }

    int targetDrops = static_cast<int>(clampedIntensity * static_cast<float>(m_maxDrops));
    targetDrops = std::clamp(targetDrops, 0, m_maxDrops);
    m_activeDrops = targetDrops;

    glm::vec3 center = camera.position();
    if (!m_hasCenter) {
        m_hasCenter = true;
        m_lastCenter = center;
        for (auto& d : m_drops) {
            respawnDrop(d, center, clampedIntensity, cloudHeight);
        }
    } else {
        glm::vec3 delta = center - m_lastCenter;
        if (glm::length(delta) > 0.0001f) {
            for (auto& d : m_drops) {
                d.pos += delta;
            }
        }
        m_lastCenter = center;
    }

    glm::vec3 boundsMin(center.x - kSpawnRadius, kSpawnBottom, center.z - kSpawnRadius);
    glm::vec3 boundsMax(center.x + kSpawnRadius, cloudHeight + kCloudSpawnMaxOffset, center.z + kSpawnRadius);

    for (int i = 0; i < m_activeDrops; ++i) {
        RainDrop& d = m_drops[i];
        glm::vec2 wind = windDir + d.drift;
        glm::vec3 vel(wind.x * d.speed * kWindStrength, -d.speed, wind.y * d.speed * kWindStrength);
        d.pos += vel * dt;

        bool out = d.pos.y < boundsMin.y
            || d.pos.x < boundsMin.x || d.pos.x > boundsMax.x
            || d.pos.z < boundsMin.z || d.pos.z > boundsMax.z;
        if (out) {
            respawnDrop(d, center, clampedIntensity, cloudHeight);
        }

        m_instances[i] = { d.pos, glm::vec2(d.length, d.width) };
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, m_activeDrops * sizeof(RainInstance), m_instances.data(), GL_STREAM_DRAW);
}

void RainSystem::render(Shader& rainShader, const Camera& camera, float intensity, const glm::vec2& windDir, int fbW, int fbH) {
    if (intensity < 0.001f || m_activeDrops == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    rainShader.use();
    rainShader.setMat4("uProjection", camera.projectionMatrix(static_cast<float>(fbW)/std::max(1, fbH)));
    rainShader.setMat4("uView", camera.viewMatrix());
    rainShader.setVec3("uCameraPos", camera.position());
    rainShader.setVec2("uWindDir", windDir);
    rainShader.setFloat("uRainIntensity", std::clamp(intensity, 0.0f, 1.0f));

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_activeDrops);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
