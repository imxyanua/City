#include "ParticleSystem.h"
#include <glad/gl.h>
#include <algorithm>

struct ParticleVertex {
    glm::vec3 position;
    float size;
    float alpha;
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

    glBindVertexArray(0);
}

void ParticleSystem::update(float dt) {
    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            it->position += it->velocity * dt;
            it->velocity.y -= 9.8f * dt; // Gravity
            ++it;
        }
    }
}

void ParticleSystem::emit(const glm::vec3& pos, const glm::vec3& vel, float life, float size) {
    if (m_particles.size() > 5000) return; // Limit particles
    Particle p;
    p.position = pos;
    p.velocity = vel;
    p.life = life;
    p.maxLife = life;
    p.size = size;
    m_particles.push_back(p);
}

void ParticleSystem::draw(Shader& shader, const Camera& camera, float aspect) {
    if (m_particles.empty()) return;

    std::vector<ParticleVertex> vertices;
    vertices.reserve(m_particles.size());
    for (const auto& p : m_particles) {
        float alpha = p.life / p.maxLife;
        vertices.push_back({ p.position, p.size, alpha });
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
