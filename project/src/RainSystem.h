#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "Camera.h"
#include "Shader.h"

class RainSystem {
public:
    RainSystem();
    ~RainSystem();

    RainSystem(const RainSystem&) = delete;
    RainSystem& operator=(const RainSystem&) = delete;

    void init(int numDrops);
    void render(Shader& rainShader, const Camera& camera, float intensity, const glm::vec2& windDir, float time, int fbW, int fbH);

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_instanceVbo = 0;
    int m_numDrops = 0;
};
