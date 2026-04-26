#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Camera.h"
#include "Scene.h"
#include "AppOptions.h"

class PostProcessor {
public:
    PostProcessor();
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    bool init(const std::string& exeDir);

    void resize(int w, int h);
    void beginOffscreen(float clearCol[3]);
    void endOffscreen();

    void renderBloom(float threshold);
    void renderPost(const Camera& camera, const Scene& scene, const AppOptions& opts, float time);

    GLuint offscreenFbo() const { return m_offscreenFbo; }

private:
    void destroyFbos();
    void destroyBloomFbos();
    void ensureBloomFbos(int w, int h);

    GLuint m_offscreenFbo = 0;
    GLuint m_offscreenColor = 0;
    GLuint m_offscreenDepth = 0;
    int m_width = 0;
    int m_height = 0;

    GLuint m_bloomFbo[2] = {0, 0};
    GLuint m_bloomTex[2] = {0, 0};
    int m_bloomW = 0;
    int m_bloomH = 0;

    GLuint m_emptyVao = 0;

    Shader m_postShader;
    Shader m_bloomExtractShader;
    Shader m_bloomBlurShader;

    bool m_hasBloom = false;
};
