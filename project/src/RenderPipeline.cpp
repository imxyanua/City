#include "RenderPipeline.h"
#include <glm/gtc/matrix_transform.hpp>

RenderPipeline::RenderPipeline() = default;

void RenderPipeline::shutdownGpu()
{
    destroyShadowResources();
}

void RenderPipeline::destroyShadowResources()
{
    if (m_depthMap != 0) {
        glDeleteTextures(1, &m_depthMap);
        m_depthMap = 0;
    }
    if (m_depthMapFBO != 0) {
        glDeleteFramebuffers(1, &m_depthMapFBO);
        m_depthMapFBO = 0;
    }
}

void RenderPipeline::recreateShadowMapInternal(int dim)
{
    destroyShadowResources();
    glGenFramebuffers(1, &m_depthMapFBO);
    glGenTextures(1, &m_depthMap);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, dim, dim, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPipeline::initShadow(AppOptions& opts)
{
    int d = sanitizeShadowResolution(opts.shadowMapResolution);
    opts.shadowMapResolution = d;
    recreateShadowMapInternal(d);
    m_appliedShadowDim = d;
}

void RenderPipeline::noteRainCapsMatchingOpts(const AppOptions& opts)
{
    m_appliedRainCap = sanitizeRainMaxDrops(opts.rainMaxDrops);
}

void RenderPipeline::syncDynamicResources(AppOptions& opts, RainSystem& rainSystem)
{
    const int shadowDim = sanitizeShadowResolution(opts.shadowMapResolution);
    opts.shadowMapResolution = shadowDim;
    if (shadowDim != m_appliedShadowDim) {
        recreateShadowMapInternal(shadowDim);
        m_appliedShadowDim = shadowDim;
    }

    const int rainCap = sanitizeRainMaxDrops(opts.rainMaxDrops);
    opts.rainMaxDrops = rainCap;
    if (rainCap != m_appliedRainCap) {
        rainSystem.init(rainCap);
        m_appliedRainCap = rainCap;
    }
}

void RenderPipeline::renderFrame(
    Shader& shader,
    Shader& rainShader,
    Shader& shadowShader,
    Shader& particleShader,
    PostProcessor& postProcessor,
    Scene& scene,
    Camera& camera,
    RainSystem& rainSystem,
    AppOptions& opts,
    float timeSec,
    int fbW,
    int fbH)
{
    constexpr float nearPlane = -50.0f;
    constexpr float farPlane = 200.0f;
    const glm::mat4 lightProjection = glm::ortho(-80.0f, 80.0f, -80.0f, 80.0f, nearPlane, farPlane);
    const glm::vec3 lightDir = scene.lightDir();
    const glm::mat4 lightView = glm::lookAt(lightDir * 50.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, opts.shadowMapResolution, opts.shadowMapResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    shadowShader.use();
    shadowShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
    scene.render(shadowShader, camera);
    glCullFace(GL_BACK);

    glViewport(0, 0, fbW, fbH);
    postProcessor.resize(fbW, fbH);
    postProcessor.beginOffscreen(opts.clearCol);

    shader.use();
    shader.setFloat("uTime", timeSec);
    shader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    shader.setInt("uShadowMap", 1);

    scene.render(shader, camera);

    GLenum singleBuf[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, singleBuf);

    rainSystem.render(rainShader, camera, opts.rainIntensity, opts.windDir, fbW, fbH);

    scene.renderParticles(particleShader, camera);

    GLenum dualBuf[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, dualBuf);

    postProcessor.renderSSAO(camera);

    postProcessor.renderBloom(opts.bloomThreshold);
    postProcessor.renderPost(camera, scene, opts, timeSec);
}
