#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
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

    void renderSSAO(const Camera& camera);
    void renderBloom(float threshold);
    void renderPost(const Camera& camera, const Scene& scene, const AppOptions& opts, float time);

    GLuint offscreenFbo() const { return m_offscreenFbo; }
    GLuint normalTexture() const { return m_offscreenNormal; }
    GLuint ssaoTexture() const { return m_ssaoBlurTex; }

private:
    void destroyFbos();
    void destroyBloomFbos();
    void destroySSAOFbos();
    void ensureBloomFbos(int w, int h);
    void ensureSSAOFbos(int w, int h);
    void generateSSAOKernel();
    void generateNoiseTex();

    GLuint m_offscreenFbo = 0;
    GLuint m_offscreenColor = 0;
    GLuint m_offscreenNormal = 0;
    GLuint m_offscreenDepth = 0;
    int m_width = 0;
    int m_height = 0;

    GLuint m_bloomFbo[2] = {0, 0};
    GLuint m_bloomTex[2] = {0, 0};
    int m_bloomW = 0;
    int m_bloomH = 0;

    // SSAO
    GLuint m_ssaoFbo = 0;
    GLuint m_ssaoTex = 0;
    GLuint m_ssaoBlurFbo = 0;
    GLuint m_ssaoBlurTex = 0;
    GLuint m_noiseTex = 0;
    int m_ssaoW = 0;
    int m_ssaoH = 0;
    std::vector<glm::vec3> m_ssaoKernel;

    GLuint m_emptyVao = 0;

    Shader m_postShader;
    Shader m_bloomExtractShader;
    Shader m_bloomBlurShader;
    Shader m_ssaoShader;
    Shader m_ssaoBlurShader;

    bool m_hasBloom = false;
    bool m_hasSSAO = false;
};
