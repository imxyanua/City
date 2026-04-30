#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <array>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

struct AppOptions {
    static constexpr int kUiFavoriteCount = 9;

    bool showMenu = false;
    bool wireframe = false;
    bool cullBack = true;
    bool vsync = true;
    
    float clearCol[3] = {0.07f, 0.08f, 0.12f};
    bool syncTimeOfDay = true;
    float timeOfDayHour = 12.0f;
    
    float rainIntensity = 0.0f;
    glm::vec2 windDir = glm::normalize(glm::vec2(-1.0f, -0.2f));
    float cloudHeight = 90.0f;
    
    float bloomThreshold = 1.2f;
    float bloomIntensity = 0.4f;
    float godRayIntensity = 0.35f;
    float colorGradeIntensity = 0.85f;
    float vignetteIntensity = 0.55f;
    float fxaaIntensity = 1.0f;
    float chromaticAberration = 0.012f;
    float ssaoIntensity = 1.0f;
    float ssrIntensity = 0.6f;

    int cameraMode = 0; // 0: Free, 1: Follow Car, 2: CCTV, 3: Drive, 4: Cinematic

    /// Shadow map edge length (pixels). Allowed: 512, 1024, 2048 — lower improves FPS on weak GPUs.
    int shadowMapResolution = 1024;
    /// Max rain streaks allocated; active count scales with rain intensity × this cap.
    int rainMaxDrops = 48000;

    bool useGridLayout = false;
    int gridRows = 2;
    int gridCols = 6;
    float gridSpaceX = 95.0f;
    float gridSpaceZ = 110.0f;

    bool uiShowFavorites = true;
    bool uiCompact = false;
    bool uiShowHotkeys = true;
    /// FPS + ms/frame góc màn khi không mở menu (tiện chỉnh hiệu năng)
    bool showFpsOverlay = true;
    std::array<bool, kUiFavoriteCount> uiFavorites = {true, true, true, false, false, true, false, false, false};
    int uiSection = 0;
    std::string uiSearch;
};

inline int sanitizeShadowResolution(int v) {
    if (v <= 768) return 512;
    if (v <= 1536) return 1024;
    return 2048;
}

inline int sanitizeRainMaxDrops(int v) {
    constexpr int kMin = 4000;
    constexpr int kMax = 130000;
    return std::clamp(v, kMin, kMax);
}

inline void saveConfig(const AppOptions& opts, const std::string& path) {
    nlohmann::json j;
    j["wireframe"] = opts.wireframe;
    j["cullBack"] = opts.cullBack;
    j["vsync"] = opts.vsync;
    j["clearCol"] = {opts.clearCol[0], opts.clearCol[1], opts.clearCol[2]};
    j["syncTimeOfDay"] = opts.syncTimeOfDay;
    j["timeOfDayHour"] = opts.timeOfDayHour;
    j["rainIntensity"] = opts.rainIntensity;
    j["windDir"] = {opts.windDir.x, opts.windDir.y};
    j["cloudHeight"] = opts.cloudHeight;
    j["bloomThreshold"] = opts.bloomThreshold;
    j["bloomIntensity"] = opts.bloomIntensity;
    j["godRayIntensity"] = opts.godRayIntensity;
    j["colorGradeIntensity"] = opts.colorGradeIntensity;
    j["vignetteIntensity"] = opts.vignetteIntensity;
    j["fxaaIntensity"] = opts.fxaaIntensity;
    j["chromaticAberration"] = opts.chromaticAberration;
    j["ssaoIntensity"] = opts.ssaoIntensity;
    j["ssrIntensity"] = opts.ssrIntensity;
    j["cameraMode"] = opts.cameraMode;
    j["shadowMapResolution"] = opts.shadowMapResolution;
    j["rainMaxDrops"] = opts.rainMaxDrops;
    j["useGridLayout"] = opts.useGridLayout;
    j["gridRows"] = opts.gridRows;
    j["gridCols"] = opts.gridCols;
    j["gridSpaceX"] = opts.gridSpaceX;
    j["gridSpaceZ"] = opts.gridSpaceZ;
    j["uiShowFavorites"] = opts.uiShowFavorites;
    j["uiCompact"] = opts.uiCompact;
    j["uiShowHotkeys"] = opts.uiShowHotkeys;
    j["showFpsOverlay"] = opts.showFpsOverlay;
    j["uiFavorites"] = opts.uiFavorites;
    j["uiSection"] = opts.uiSection;
    j["uiSearch"] = opts.uiSearch;

    std::ofstream o(path);
    if (o.is_open()) {
        o << std::setw(4) << j << std::endl;
    }
}

inline void loadConfig(AppOptions& opts, const std::string& path) {
    std::ifstream i(path);
    if (!i.is_open()) return;
    nlohmann::json j;
    try {
        i >> j;
        if (j.contains("wireframe")) opts.wireframe = j["wireframe"];
        if (j.contains("cullBack")) opts.cullBack = j["cullBack"];
        if (j.contains("vsync")) opts.vsync = j["vsync"];
        if (j.contains("clearCol") && j["clearCol"].is_array() && j["clearCol"].size() >= 3) {
            opts.clearCol[0] = j["clearCol"][0].get<float>();
            opts.clearCol[1] = j["clearCol"][1].get<float>();
            opts.clearCol[2] = j["clearCol"][2].get<float>();
        }
        if (j.contains("syncTimeOfDay")) opts.syncTimeOfDay = j["syncTimeOfDay"];
        if (j.contains("timeOfDayHour")) opts.timeOfDayHour = j["timeOfDayHour"];
        if (j.contains("rainIntensity")) opts.rainIntensity = j["rainIntensity"];
        if (j.contains("windDir") && j["windDir"].is_array() && j["windDir"].size() >= 2) {
            glm::vec2 w(j["windDir"][0].get<float>(), j["windDir"][1].get<float>());
            const float len = glm::length(w);
            opts.windDir = (len > 1.0e-6f) ? (w / len) : glm::normalize(glm::vec2(-1.0f, -0.2f));
        }
        if (j.contains("cloudHeight")) opts.cloudHeight = j["cloudHeight"];
        if (j.contains("bloomThreshold")) opts.bloomThreshold = j["bloomThreshold"];
        if (j.contains("bloomIntensity")) opts.bloomIntensity = j["bloomIntensity"];
        if (j.contains("godRayIntensity")) opts.godRayIntensity = j["godRayIntensity"];
        if (j.contains("colorGradeIntensity")) opts.colorGradeIntensity = j["colorGradeIntensity"];
        if (j.contains("vignetteIntensity")) opts.vignetteIntensity = j["vignetteIntensity"];
        if (j.contains("fxaaIntensity")) opts.fxaaIntensity = j["fxaaIntensity"];
        if (j.contains("chromaticAberration")) opts.chromaticAberration = j["chromaticAberration"];
        if (j.contains("ssaoIntensity")) opts.ssaoIntensity = j["ssaoIntensity"];
        if (j.contains("ssrIntensity")) opts.ssrIntensity = j["ssrIntensity"];
        if (j.contains("cameraMode")) {
            int cm = j["cameraMode"].get<int>();
            if (cm < 0) cm = 0;
            if (cm > 4) cm = 4;
            opts.cameraMode = cm;
        }
        if (j.contains("shadowMapResolution")) opts.shadowMapResolution = j["shadowMapResolution"];
        if (j.contains("rainMaxDrops")) opts.rainMaxDrops = j["rainMaxDrops"];
        if (j.contains("useGridLayout")) opts.useGridLayout = j["useGridLayout"];
        if (j.contains("gridRows")) opts.gridRows = j["gridRows"];
        if (j.contains("gridCols")) opts.gridCols = j["gridCols"];
        if (j.contains("gridSpaceX")) opts.gridSpaceX = j["gridSpaceX"];
        if (j.contains("gridSpaceZ")) opts.gridSpaceZ = j["gridSpaceZ"];
        if (j.contains("gridRows") || j.contains("gridCols") || j.contains("useGridLayout")
            || j.contains("gridSpaceX") || j.contains("gridSpaceZ")) {
            opts.gridRows = std::clamp(opts.gridRows, 1, 10);
            opts.gridCols = std::clamp(opts.gridCols, 1, 10);
        }
        if (j.contains("uiShowFavorites")) opts.uiShowFavorites = j["uiShowFavorites"];
        if (j.contains("uiCompact")) opts.uiCompact = j["uiCompact"];
        if (j.contains("uiShowHotkeys")) opts.uiShowHotkeys = j["uiShowHotkeys"];
        if (j.contains("showFpsOverlay")) opts.showFpsOverlay = j["showFpsOverlay"];
        if (j.contains("uiFavorites") && j["uiFavorites"].is_array()) {
            const auto& arr = j["uiFavorites"];
            const int count = std::min(static_cast<int>(arr.size()), AppOptions::kUiFavoriteCount);
            for (int i = 0; i < count; ++i) {
                opts.uiFavorites[i] = arr[i].get<bool>();
            }
        }
        if (j.contains("uiSection")) {
            int s = j["uiSection"].get<int>();
            if (s < 0) s = 0;
            if (s > 3) s = 3;
            opts.uiSection = s;
        }
        if (j.contains("uiSearch")) opts.uiSearch = j["uiSearch"].get<std::string>();

        opts.shadowMapResolution = sanitizeShadowResolution(opts.shadowMapResolution);
        opts.rainMaxDrops = sanitizeRainMaxDrops(opts.rainMaxDrops);
    } catch (std::exception& e) {
        std::cerr << "Failed to parse config: " << e.what() << std::endl;
    }
}
