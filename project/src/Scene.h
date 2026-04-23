#pragma once

#include <algorithm>

#include "Camera.h"
#include "Model.h"
#include "Shader.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Car {
    glm::vec3 pos{0.0f};
    glm::vec3 dir{0.0f, 0.0f, 1.0f};
    float speed = 10.0f;
    int modelIdx = 0;
    float scale = 1.0f;
    float rotOffset = 0.0f;
    bool stopped = false;   // stopped at red light?
};

struct Pedestrian {
    glm::vec3 pos{0.0f};
    glm::vec3 dir{0.0f, 0.0f, 1.0f};
    float speed = 1.2f;
    float walkCycle = 0.0f;
};

enum class TrafficPhase { GreenNS, YellowNS, GreenEW, YellowEW };

struct TrafficLight {
    glm::vec3 pos{0.0f};       // world position of the intersection center
    TrafficPhase phase = TrafficPhase::GreenNS;
    float timer = 0.0f;
    float greenDuration  = 10.0f;
    float yellowDuration =  2.0f;

    // Returns true if North-South traffic can go
    bool isGreenNS() const { return phase == TrafficPhase::GreenNS; }
    // Returns true if East-West traffic can go
    bool isGreenEW() const { return phase == TrafficPhase::GreenEW; }
};

class Scene {
public:
    bool loadCityModel(const std::string& glbPath);
    bool loadCarModels(const std::vector<std::string>& paths);
    void setInstanceTransforms(std::vector<glm::mat4> transforms);
    
    void initTrafficAndPedestrians();
    void update(float dt);
    void setAspect(float aspect) { m_aspect = aspect; }
    void render(Shader& shader, const Camera& camera) const;

    float carScale() const { return m_carScale; }
    void setCarScale(float s) { m_carScale = s; for(auto& c : m_cars) c.scale = s; }
    float carYOffset() const { return m_carYOffset; }
    void setCarYOffset(float y) { m_carYOffset = y; }
    int carCount() const { return (int)m_cars.size(); }
    int carModelCount() const { return (int)m_carModels.size(); }

    Model&       cityModel()       { return m_cityModel; }
    const Model& cityModel() const { return m_cityModel; }

    bool m_debugDrawCarOnly = false;
    int m_debugCarIndex = 0;

    // Exposure
    float exposure() const            { return m_exposure; }
    void  setExposure(float v)        { m_exposure = std::max(0.05f, v); }

    // Sky / Ground / Sun
    glm::vec3 skyColor() const        { return m_skyColor; }
    void setSkyColor(const glm::vec3& c)   { m_skyColor = c; }
    glm::vec3 groundColor() const     { return m_groundColor; }
    void setGroundColor(const glm::vec3& c){ m_groundColor = c; }
    glm::vec3 sunColor() const        { return m_sunColor; }
    void setSunColor(const glm::vec3& c)   { m_sunColor = c; }
    glm::vec3 lightDir() const        { return m_lightDir; }
    void setLightDir(const glm::vec3& d)   { m_lightDir = d; }

    // Fog
    glm::vec3 fogColor() const        { return m_fogColor; }
    void setFogColor(const glm::vec3& c)   { m_fogColor = c; }
    float fogDensity() const          { return m_fogDensity; }
    void  setFogDensity(float d)      { m_fogDensity = std::max(0.0f, d); }
    float fogBaseHeight() const       { return m_fogBaseHeight; }
    void  setFogBaseHeight(float v)   { m_fogBaseHeight = v; }
    float fogHeightFalloff() const    { return m_fogHeightFalloff; }
    void  setFogHeightFalloff(float v){ m_fogHeightFalloff = std::max(0.0f, v); }

    // Wetness
    float wetness() const             { return m_wetness; }
    void  setWetness(float v)         { m_wetness = std::clamp(v, 0.0f, 1.0f); }

    // Emissive windows
    float windowEmissive() const      { return m_windowEmissive; }
    void  setWindowEmissive(float v)  { m_windowEmissive = std::max(0.0f, v); }
    float dayFactor() const           { return m_dayFactor; }
    void  setDayFactor(float v)       { m_dayFactor = std::clamp(v, 0.0f, 1.0f); }

private:
    void drawCars(Shader& shader, const glm::mat4& world) const;
    void drawPedestrians(Shader& shader, const glm::mat4& world) const;
    void drawTrafficLights(Shader& shader, const glm::mat4& world) const;
    void buildPersonMesh();
    void buildTrafficLightMesh();

    Model m_cityModel;
    std::vector<glm::mat4> m_instanceTransforms;

    std::vector<Model> m_carModels;
    std::vector<Car> m_cars;

    Mesh m_personMesh;
    std::vector<Pedestrian> m_pedestrians;

    // Traffic lights
    Mesh m_trafficLightPole;
    Mesh m_trafficLightHead;
    Mesh m_trafficLightBulb;
    std::vector<TrafficLight> m_trafficLights;

    float m_aspect = 16.0f / 9.0f;
    float m_carScale = 100.0f;
    float m_carYOffset = 0.0f;

    glm::vec3 m_lightDir{0.4f, -0.92f, 0.15f};
    glm::vec3 m_skyColor{0.38f, 0.48f, 0.62f};
    glm::vec3 m_groundColor{0.20f, 0.18f, 0.16f};
    glm::vec3 m_sunColor{1.05f, 0.97f, 0.88f};
    float m_exposure = 1.18f;

    glm::vec3 m_fogColor{0.45f, 0.52f, 0.62f};
    float m_fogDensity       = 0.0f;
    float m_fogBaseHeight    = 0.0f;
    float m_fogHeightFalloff = 0.04f;

    float m_wetness        = 0.0f;
    float m_windowEmissive = 1.5f;
    float m_dayFactor      = 1.0f;
};
