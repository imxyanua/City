#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <random>
#include <vector>
#include "Camera.h"
#include "Shader.h"

class RainSystem {
public:
    RainSystem();
    ~RainSystem();

    RainSystem(const RainSystem&) = delete;
    RainSystem& operator=(const RainSystem&) = delete;

    void init(int maxDrops);
    void update(float dt, const Camera& camera, float intensity, const glm::vec2& windDir, float cloudHeight);
    void render(Shader& rainShader, const Camera& camera, float intensity, const glm::vec2& windDir, int fbW, int fbH);

private:
    struct RainDrop {
        glm::vec3 pos;
        float speed;
        float length;
        float width;
        glm::vec2 drift;
    };

    struct RainInstance {
        glm::vec3 pos;
        glm::vec2 params;
    };

    void respawnDrop(RainDrop& drop, const glm::vec3& center, float intensity, float cloudHeight);

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_instanceVbo = 0;
    int m_maxDrops = 0;
    int m_activeDrops = 0;
    bool m_hasCenter = false;
    glm::vec3 m_lastCenter = glm::vec3(0.0f);

    std::vector<RainDrop> m_drops;
    std::vector<RainInstance> m_instances;
    std::mt19937 m_rng;
};
