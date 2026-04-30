#pragma once

#include "Scene.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/glm.hpp>

/// Cập nhật hướng sáng, màu trời/đất, day factor và màu clear theo giờ (0–24).
inline void applyTimeOfDayHour(Scene& scene, float hour, float clearCol[3])
{
    float h = std::fmod(hour, 24.0f);
    if (h < 0.0f) {
        h += 24.0f;
    }

    const float pi = glm::pi<float>();
    const float angle = (h / 24.0f) * 2.0f * pi - pi * 0.5f;
    const float el = std::sin(angle);
    const float az = (h / 24.0f) * 2.0f * pi;

    glm::vec3 sunDir(
        std::cos(el * 1.1f) * std::sin(az),
        std::max(el, -0.12f),
        std::cos(el * 1.1f) * std::cos(az));
    sunDir = glm::normalize(sunDir);
    const glm::vec3 lightTravel = -sunDir;
    scene.setLightDir(lightTravel);

    const float day = glm::clamp((el + 0.25f) / 1.15f, 0.0f, 1.0f);
    float twi = 0.0f;
    if (h >= 5.0f && h < 8.5f) {
        twi = glm::smoothstep(5.0f, 8.0f, h);
    } else if (h >= 17.0f && h < 20.5f) {
        twi = glm::smoothstep(20.5f, 17.0f, h);
    }

    const glm::vec3 skyNight(0.04f, 0.05f, 0.12f);
    const glm::vec3 skyDay(0.38f, 0.48f, 0.62f);
    const glm::vec3 skyWarm(0.72f, 0.42f, 0.30f);
    glm::vec3 sky = glm::mix(skyNight, skyDay, day);
    sky = glm::mix(sky, skyWarm, twi * (1.0f - day * 0.5f));

    const glm::vec3 gNight(0.03f, 0.03f, 0.05f);
    const glm::vec3 gDay(0.20f, 0.18f, 0.16f);
    const glm::vec3 ground = glm::mix(gNight, gDay, day);

    const glm::vec3 sunNight(0.35f, 0.38f, 0.55f);
    const glm::vec3 sunDay(1.05f, 0.97f, 0.88f);
    const glm::vec3 sunDawn(1.15f, 0.55f, 0.35f);
    glm::vec3 sunCol = glm::mix(sunNight, sunDay, day);
    sunCol = glm::mix(sunCol, sunDawn, twi * 0.85f);
    const float sunPow = glm::clamp(el * 1.3f + 0.15f, 0.0f, 1.0f);
    sunCol *= 0.35f + 0.65f * sunPow;

    scene.setSkyColor(sky);
    scene.setGroundColor(ground);
    scene.setSunColor(sunCol);
    scene.setDayFactor(day);

    if (day < 0.18f) {
        clearCol[0] = skyNight.x * 0.92f;
        clearCol[1] = skyNight.y * 0.92f;
        clearCol[2] = skyNight.z * 0.92f;
    } else {
        clearCol[0] = sky.x * 0.35f + 0.02f;
        clearCol[1] = sky.y * 0.35f + 0.03f;
        clearCol[2] = sky.z * 0.35f + 0.06f;
    }
}
