#include "PostProcessor.h"
#include <iostream>
#include <filesystem>

PostProcessor::PostProcessor() {
    glGenVertexArrays(1, &m_emptyVao);
}

PostProcessor::~PostProcessor() {
    destroyFbos();
    if (m_emptyVao != 0) {
        glDeleteVertexArrays(1, &m_emptyVao);
        m_emptyVao = 0;
    }
}

bool PostProcessor::init(const std::string& exeDir) {
    std::filesystem::path dir(exeDir);
    const auto postVertPath = dir / "shaders" / "post.vert";
    const auto postFragPath = dir / "shaders" / "post.frag";
    const auto bloomExtPath = dir / "shaders" / "bloom_extract.frag";
    const auto bloomBlurPath = dir / "shaders" / "bloom_blur.frag";

    if (!m_postShader.loadFromFiles(postVertPath.string(), postFragPath.string())) {
        std::cerr << "Post shader failed.\n";
        return false;
    }

    m_hasBloom = m_bloomExtractShader.loadFromFiles(postVertPath.string(), bloomExtPath.string())
              && m_bloomBlurShader.loadFromFiles(postVertPath.string(), bloomBlurPath.string());
              
    if (!m_hasBloom) {
        std::cerr << "Bloom shaders failed (non-critical, continuing without bloom).\n";
    }

    return true;
}

void PostProcessor::destroyBloomFbos() {
    for (int i = 0; i < 2; ++i) {
        if (m_bloomTex[i]) { glDeleteTextures(1, &m_bloomTex[i]); m_bloomTex[i] = 0; }
        if (m_bloomFbo[i]) { glDeleteFramebuffers(1, &m_bloomFbo[i]); m_bloomFbo[i] = 0; }
    }
    m_bloomW = m_bloomH = 0;
}

void PostProcessor::ensureBloomFbos(int w, int h) {
    int bw = w / 4, bh = h / 4;
    if (bw <= 0 || bh <= 0) return;
    if (bw == m_bloomW && bh == m_bloomH && m_bloomFbo[0] != 0) return;
    destroyBloomFbos();
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &m_bloomFbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[i]);
        glGenTextures(1, &m_bloomTex[i]);
        glBindTexture(GL_TEXTURE_2D, m_bloomTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bw, bh, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomTex[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_bloomW = bw;
    m_bloomH = bh;
}

void PostProcessor::destroyFbos() {
    if (m_offscreenDepth != 0) { glDeleteTextures(1, &m_offscreenDepth); m_offscreenDepth = 0; }
    if (m_offscreenColor != 0) { glDeleteTextures(1, &m_offscreenColor); m_offscreenColor = 0; }
    if (m_offscreenFbo != 0)   { glDeleteFramebuffers(1, &m_offscreenFbo); m_offscreenFbo = 0; }
    m_width = m_height = 0;
    destroyBloomFbos();
}

void PostProcessor::resize(int w, int h) {
    if (w <= 0 || h <= 0) return;
    if (w == m_width && h == m_height && m_offscreenFbo != 0) return;
    destroyFbos();
    glGenFramebuffers(1, &m_offscreenFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_offscreenFbo);
    // HDR color attachment
    glGenTextures(1, &m_offscreenColor);
    glBindTexture(GL_TEXTURE_2D, m_offscreenColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_offscreenColor, 0);
    // Depth attachment
    glGenTextures(1, &m_offscreenDepth);
    glBindTexture(GL_TEXTURE_2D, m_offscreenDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_offscreenDepth, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FBO] HDR framebuffer incomplete.\n";
        destroyFbos();
    } else {
        m_width = w;
        m_height = h;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ensureBloomFbos(w, h);
}

void PostProcessor::beginOffscreen(float clearCol[3]) {
    if (m_offscreenFbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_offscreenFbo);
        glViewport(0, 0, m_width, m_height);
        glClearColor(clearCol[0], clearCol[1], clearCol[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void PostProcessor::endOffscreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::renderBloom(float threshold) {
    if (m_hasBloom && m_bloomFbo[0] != 0) {
        glDisable(GL_DEPTH_TEST);
        // Extract bright areas
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[0]);
        glViewport(0, 0, m_bloomW, m_bloomH);
        m_bloomExtractShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_offscreenColor);
        m_bloomExtractShader.setInt("uScene", 0);
        m_bloomExtractShader.setFloat("uThreshold", threshold);
        glBindVertexArray(m_emptyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // Blur (Ping-pong 4 times)
        m_bloomBlurShader.use();
        m_bloomBlurShader.setInt("uInput", 0);
        for (int i = 0; i < 4; ++i) {
            int ping = i % 2;
            int pong = (i + 1) % 2;
            glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo[pong]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_bloomTex[ping]);
            m_bloomBlurShader.setVec2("uDirection", ping == 0 ? glm::vec2(1.0f / m_bloomW, 0.0f) : glm::vec2(0.0f, 1.0f / m_bloomH));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glEnable(GL_DEPTH_TEST);
    }
}

void PostProcessor::renderPost(const Camera& camera, const Scene& scene, const AppOptions& opts, float time) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glDisable(GL_DEPTH_TEST);
    m_postShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_offscreenColor);
    m_postShader.setInt("uScene", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_offscreenDepth);
    m_postShader.setInt("uDepth", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_bloomTex[0]);
    m_postShader.setInt("uBloom", 2);

    m_postShader.setVec2("uResolution", glm::vec2(static_cast<float>(m_width), static_cast<float>(m_height)));
    m_postShader.setFloat("uTime", time);
    m_postShader.setFloat("uRainIntensity", opts.rainIntensity);
    const float aspect = static_cast<float>(m_width) / static_cast<float>(std::max(1, m_height));
    const glm::mat4 proj = camera.projectionMatrix(aspect);
    const glm::mat4 view = camera.viewMatrix();
    m_postShader.setMat4("uInvProj", glm::inverse(proj));
    m_postShader.setMat4("uInvView", glm::inverse(view));
    m_postShader.setFloat("uNear", camera.nearPlane());
    m_postShader.setFloat("uFar", camera.farPlane());
    
    glm::vec3 sunDir = glm::normalize(-scene.lightDir());
    const float sunElev = sunDir.y;
    const float sunIntensity = glm::clamp((sunElev + 0.07f) / 0.24f, 0.f, 1.f);
    m_postShader.setVec3("uSunDir", sunDir);
    m_postShader.setFloat("uSunIntensity", sunIntensity);
    m_postShader.setVec3("uSkyColor", scene.skyColor());
    m_postShader.setVec3("uGroundColor", scene.groundColor());
    m_postShader.setFloat("uDayFactor", scene.dayFactor());

    m_postShader.setFloat("uBloomIntensity", opts.bloomIntensity);
    m_postShader.setFloat("uGodRayIntensity", opts.godRayIntensity);
    m_postShader.setFloat("uColorGradeIntensity", opts.colorGradeIntensity);
    m_postShader.setFloat("uVignetteIntensity", opts.vignetteIntensity);
    m_postShader.setFloat("uFxaaIntensity", opts.fxaaIntensity);
    m_postShader.setFloat("uChromaticAberration", opts.chromaticAberration);
    m_postShader.setVec2("uWindDir", opts.windDir);
    
    glm::vec4 sunClip = proj * view * glm::vec4(sunDir * 1000.0f, 1.0f);
    glm::vec2 sunNdc = glm::vec2(sunClip.x, sunClip.y) / sunClip.w;
    glm::vec2 sunScreen = (sunNdc * 0.5f + 0.5f);
    m_postShader.setVec2("uSunScreenPos", sunScreen);

    glBindVertexArray(m_emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
