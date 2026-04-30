#pragma once

#include <glad/gl.h>

#include "AppOptions.h"
#include "Camera.h"
#include "PostProcessor.h"
#include "RainSystem.h"
#include "Scene.h"
#include "Shader.h"

/// Shadow map + một frame render đầy đủ (scene → rain → particle → SSAO → bloom → post).
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline() = default;

    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    void initShadow(AppOptions& opts);
    void noteRainCapsMatchingOpts(const AppOptions& opts);

    void syncDynamicResources(AppOptions& opts, RainSystem& rainSystem);

    /// Gọi trước khi đóng context OpenGL / cửa sổ (tránh release sau khi context mất).
    void shutdownGpu();

    void renderFrame(Shader& shader,
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
        int fbH);

private:
    void destroyShadowResources();
    void recreateShadowMapInternal(int dim);

    GLuint m_depthMapFBO = 0;
    GLuint m_depthMap = 0;
    int m_appliedShadowDim = -1;
    int m_appliedRainCap = -1;
};
