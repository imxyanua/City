#pragma once

#include <glm/glm.hpp>

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
