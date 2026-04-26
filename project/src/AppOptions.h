#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

struct AppOptions {
    bool showMenu = false;
    bool wireframe = false;
    bool cullBack = true;
    bool vsync = true;
    
    float clearCol[3] = {0.07f, 0.08f, 0.12f};
    bool syncTimeOfDay = true;
    float timeOfDayHour = 12.0f;
    
    float rainIntensity = 0.0f;
    glm::vec2 windDir = glm::normalize(glm::vec2(-1.0f, -0.2f));
    
    float bloomThreshold = 1.2f;
    float bloomIntensity = 0.4f;
    float godRayIntensity = 0.35f;
    float colorGradeIntensity = 0.85f;
    float vignetteIntensity = 0.55f;
    float fxaaIntensity = 1.0f;
    float chromaticAberration = 0.012f;

    bool useGridLayout = false;
    int gridRows = 2;
    int gridCols = 6;
    float gridSpaceX = 95.0f;
    float gridSpaceZ = 110.0f;
};

inline void saveConfig(const AppOptions& opts, const std::string& path) {
    nlohmann::json j;
    j["timeOfDayHour"] = opts.timeOfDayHour;
    j["rainIntensity"] = opts.rainIntensity;
    j["bloomThreshold"] = opts.bloomThreshold;
    j["bloomIntensity"] = opts.bloomIntensity;
    j["godRayIntensity"] = opts.godRayIntensity;
    j["colorGradeIntensity"] = opts.colorGradeIntensity;
    j["vignetteIntensity"] = opts.vignetteIntensity;
    j["fxaaIntensity"] = opts.fxaaIntensity;
    j["chromaticAberration"] = opts.chromaticAberration;

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
        if (j.contains("bloomThreshold")) opts.bloomThreshold = j["bloomThreshold"];
        if (j.contains("bloomIntensity")) opts.bloomIntensity = j["bloomIntensity"];
        if (j.contains("godRayIntensity")) opts.godRayIntensity = j["godRayIntensity"];
        if (j.contains("colorGradeIntensity")) opts.colorGradeIntensity = j["colorGradeIntensity"];
        if (j.contains("vignetteIntensity")) opts.vignetteIntensity = j["vignetteIntensity"];
        if (j.contains("fxaaIntensity")) opts.fxaaIntensity = j["fxaaIntensity"];
        if (j.contains("chromaticAberration")) opts.chromaticAberration = j["chromaticAberration"];
    } catch (std::exception& e) {
        std::cerr << "Failed to parse config: " << e.what() << std::endl;
    }
}
