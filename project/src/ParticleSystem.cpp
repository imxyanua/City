#include "ParticleSystem.h"
#include <glad/gl.h>
#include <algorithm>

struct ParticleVertex {
    glm::vec3 position;
    float size;
    float alpha;
    float type; // Passed as float to vertex shader
};

ParticleSystem::ParticleSystem() {
}

ParticleSystem::~ParticleSystem() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
}

void ParticleSystem::init() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)0);
    // Size
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, size));
    // Alpha
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, alpha));
    // Type
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)offsetof(ParticleVertex, type));

    glBindVertexArray(0);
}

void ParticleSystem::update(float dt) {
    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            it->position += it->velocity * dt;
            it->velocity.y -= it->gravity * dt; // Gravity (can be negative for smoke)
            // Wind effect for smoke
            if (it->type == 1) {
                it->position.x += sin(it->life * 2.0f) * 0.5f * dt;
                it->position.z += cos(it->life * 1.5f) * 0.5f * dt;
                it->size += 0.5f * dt; // Smoke expands over time
            }
            ++it;
        }
    }
}

void ParticleSystem::emit(const glm::vec3& pos, const glm::vec3& vel, float life, float size, float gravity, int type) {
    if (m_particles.size() > 8000) return; // Limit particles (increased for smoke + rain)
    Particle p;
    p.position = pos;
    p.velocity = vel;
    p.life = life;
    p.maxLife = life;
    p.size = size;
    p.gravity = gravity;
    p.type = type;
    m_particles.push_back(p);
}

void ParticleSystem::draw(Shader& shader, const Camera& camera, float aspect) {
    if (m_particles.empty()) return;

    std::vector<ParticleVertex> vertices;
    vertices.reserve(m_particles.size());
    for (const auto& p : m_particles) {
        float alpha = p.life / p.maxLife;
        if (p.type == 1) alpha *= 0.6f; // Smoke is naturally more transparent
        vertices.push_back({ p.position, p.size, alpha, static_cast<float>(p.type) });
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ParticleVertex), vertices.data(), GL_DYNAMIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write depth
    glEnable(GL_PROGRAM_POINT_SIZE); // Enable setting point size in shader

    shader.use();
    shader.setMat4("uProjection", camera.projectionMatrix(aspect));
    shader.setMat4("uView", camera.viewMatrix());

    glDrawArrays(GL_POINTS, 0, vertices.size());

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
