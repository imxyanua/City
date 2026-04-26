#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Camera.h"

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    float maxLife;
    float size;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void init();
    void update(float dt);
    void draw(Shader& shader, const Camera& camera, float aspect);
    void emit(const glm::vec3& pos, const glm::vec3& vel, float life, float size);

private:
    std::vector<Particle> m_particles;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
};
