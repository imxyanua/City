#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <array>
#include <algorithm>
#include <fstream>
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

    bool useGridLayout = false;
    int gridRows = 2;
    int gridCols = 6;
    float gridSpaceX = 95.0f;
    float gridSpaceZ = 110.0f;

    bool uiShowFavorites = true;
    bool uiCompact = false;
    bool uiShowHotkeys = true;
    std::array<bool, kUiFavoriteCount> uiFavorites = {true, true, true, false, false, true, false, false, false};
    int uiSection = 0;
    std::string uiSearch;
};

inline void saveConfig(const AppOptions& opts, const std::string& path) {
    nlohmann::json j;
    j["timeOfDayHour"] = opts.timeOfDayHour;
    j["rainIntensity"] = opts.rainIntensity;
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
    j["uiShowFavorites"] = opts.uiShowFavorites;
    j["uiCompact"] = opts.uiCompact;
    j["uiShowHotkeys"] = opts.uiShowHotkeys;
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
        if (j.contains("timeOfDayHour")) opts.timeOfDayHour = j["timeOfDayHour"];
        if (j.contains("rainIntensity")) opts.rainIntensity = j["rainIntensity"];
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
        if (j.contains("cameraMode")) opts.cameraMode = j["cameraMode"];
        if (j.contains("uiShowFavorites")) opts.uiShowFavorites = j["uiShowFavorites"];
        if (j.contains("uiCompact")) opts.uiCompact = j["uiCompact"];
        if (j.contains("uiShowHotkeys")) opts.uiShowHotkeys = j["uiShowHotkeys"];
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
    } catch (std::exception& e) {
        std::cerr << "Failed to parse config: " << e.what() << std::endl;
    }
}
