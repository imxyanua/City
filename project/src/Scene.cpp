#include "Scene.h"
#include <iostream>
#include <string>
#include <utility>
#include <cmath>
#include <cstdlib>
#include <random>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

static constexpr float PI = 3.14159265f;
static constexpr float CITY_HALF = 150.0f;
static constexpr float STOP_DIST = 14.0f;
static constexpr float CAR_SCALE = 100.0f;  // The GLB car models are extremely tiny (0.05 units long)

bool Scene::loadCityModel(const std::string& glbPath) {
    if (!m_cityModel.loadFromGLB(glbPath)) {
        std::cerr << "[Scene] Failed to load GLB: " << glbPath << "\n";
        return false;
    }
    return true;
}

bool Scene::loadCarModels(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        Model m;
        if (!m.loadFromGLB(path)) {
            std::cerr << "[Scene] Failed to load car GLB: " << path << "\n";
            continue; // skip failed, don't abort all
        }
        std::cerr << "[Scene] Loaded car: " << path << " (" << m.meshes().size() << " meshes)\n";
        m_carModels.push_back(std::move(m));
    }
    std::cerr << "[Scene] Total car models loaded: " << m_carModels.size() << "\n";
    return !m_carModels.empty();
}

// ─── helpers for procedural geometry ─────────────────────────────
static void addBox(std::vector<float>& p, std::vector<float>& n, std::vector<float>& uv,
                   std::vector<unsigned int>& idx, glm::vec3 c, glm::vec3 e) {
    unsigned int base = (unsigned int)(p.size() / 3);
    glm::vec3 v[8] = {
        c+glm::vec3(-e.x,-e.y,-e.z), c+glm::vec3(e.x,-e.y,-e.z),
        c+glm::vec3(e.x,e.y,-e.z),   c+glm::vec3(-e.x,e.y,-e.z),
        c+glm::vec3(-e.x,-e.y,e.z),  c+glm::vec3(e.x,-e.y,e.z),
        c+glm::vec3(e.x,e.y,e.z),    c+glm::vec3(-e.x,e.y,e.z)
    };
    glm::vec3 nm[6]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
    int f[6][4]={{0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{4,5,1,0},{3,2,6,7}};
    for(int i=0;i<6;++i){
        for(int j=0;j<4;++j){
            p.push_back(v[f[i][j]].x); p.push_back(v[f[i][j]].y); p.push_back(v[f[i][j]].z);
            n.push_back(nm[i].x); n.push_back(nm[i].y); n.push_back(nm[i].z);
            uv.push_back(0); uv.push_back(0);
        }
        idx.push_back(base); idx.push_back(base+1); idx.push_back(base+2);
        idx.push_back(base); idx.push_back(base+2); idx.push_back(base+3);
        base+=4;
    }
}

// Cylinder along Y axis
static void addCylinder(std::vector<float>& p, std::vector<float>& n, std::vector<float>& uv,
                        std::vector<unsigned int>& idx, glm::vec3 base_center, float radius, float height, int segs=8) {
    unsigned int b = (unsigned int)(p.size()/3);
    float yBot = base_center.y;
    float yTop = base_center.y + height;
    float cx = base_center.x, cz = base_center.z;
    for(int i=0;i<segs;++i){
        float a0 = 2.0f*PI*i/segs, a1 = 2.0f*PI*(i+1)/segs;
        float c0=std::cos(a0),s0=std::sin(a0),c1=std::cos(a1),s1=std::sin(a1);
        glm::vec3 v0(cx+radius*c0, yBot, cz+radius*s0);
        glm::vec3 v1(cx+radius*c1, yBot, cz+radius*s1);
        glm::vec3 v2(cx+radius*c1, yTop, cz+radius*s1);
        glm::vec3 v3(cx+radius*c0, yTop, cz+radius*s0);
        glm::vec3 nm0(c0,0,s0), nm1(c1,0,s1);
        unsigned int bi = (unsigned int)(p.size()/3);
        auto pushV=[&](glm::vec3 vv, glm::vec3 nn){ p.push_back(vv.x);p.push_back(vv.y);p.push_back(vv.z);
            n.push_back(nn.x);n.push_back(nn.y);n.push_back(nn.z); uv.push_back(0);uv.push_back(0);};
        pushV(v0,nm0); pushV(v1,nm1); pushV(v2,nm1); pushV(v3,nm0);
        idx.push_back(bi); idx.push_back(bi+1); idx.push_back(bi+2);
        idx.push_back(bi); idx.push_back(bi+2); idx.push_back(bi+3);
    }
}

// Sphere (low-poly)
static void addSphere(std::vector<float>& p, std::vector<float>& n, std::vector<float>& uv,
                      std::vector<unsigned int>& idx, glm::vec3 center, float radius, int slices=6, int stacks=4) {
    for(int j=0;j<stacks;++j){
        float t0=PI*j/stacks, t1=PI*(j+1)/stacks;
        for(int i=0;i<slices;++i){
            float p0=2*PI*i/slices, p1=2*PI*(i+1)/slices;
            glm::vec3 v00=center+radius*glm::vec3(std::sin(t0)*std::cos(p0),std::cos(t0),std::sin(t0)*std::sin(p0));
            glm::vec3 v10=center+radius*glm::vec3(std::sin(t0)*std::cos(p1),std::cos(t0),std::sin(t0)*std::sin(p1));
            glm::vec3 v01=center+radius*glm::vec3(std::sin(t1)*std::cos(p0),std::cos(t1),std::sin(t1)*std::sin(p0));
            glm::vec3 v11=center+radius*glm::vec3(std::sin(t1)*std::cos(p1),std::cos(t1),std::sin(t1)*std::sin(p1));
            unsigned int bi=(unsigned int)(p.size()/3);
            auto pushV=[&](glm::vec3 vv){glm::vec3 nn=glm::normalize(vv-center);
                p.push_back(vv.x);p.push_back(vv.y);p.push_back(vv.z);
                n.push_back(nn.x);n.push_back(nn.y);n.push_back(nn.z);uv.push_back(0);uv.push_back(0);};
            pushV(v00);pushV(v10);pushV(v11);pushV(v01);
            idx.push_back(bi);idx.push_back(bi+1);idx.push_back(bi+2);
            idx.push_back(bi);idx.push_back(bi+2);idx.push_back(bi+3);
        }
    }
}

// ─── build meshes ────────────────────────────────────────────────
void Scene::buildPersonMesh() {
    std::vector<float> p,n,uv; std::vector<unsigned int> idx;
    // Head (sphere)
    addSphere(p,n,uv,idx, glm::vec3(0, 1.65f, 0), 0.12f, 8, 6);
    // Neck
    addCylinder(p,n,uv,idx, glm::vec3(0, 1.45f, 0), 0.045f, 0.08f, 6);
    // Torso (cylinder)
    addCylinder(p,n,uv,idx, glm::vec3(0, 0.9f, 0), 0.16f, 0.55f, 8);
    // Shoulders (horizontal box across top of torso)
    addBox(p,n,uv,idx, glm::vec3(0, 1.42f, 0), glm::vec3(0.24f, 0.04f, 0.08f));
    // Hips (slightly wider)
    addCylinder(p,n,uv,idx, glm::vec3(0, 0.85f, 0), 0.14f, 0.08f, 8);
    // Left leg
    addCylinder(p,n,uv,idx, glm::vec3(-0.08f, 0.08f, 0), 0.055f, 0.77f, 6);
    // Right leg
    addCylinder(p,n,uv,idx, glm::vec3(0.08f, 0.08f, 0), 0.055f, 0.77f, 6);
    // Left foot/shoe
    addBox(p,n,uv,idx, glm::vec3(-0.08f, 0.03f, 0.03f), glm::vec3(0.05f, 0.03f, 0.09f));
    // Right foot/shoe
    addBox(p,n,uv,idx, glm::vec3(0.08f, 0.03f, 0.03f), glm::vec3(0.05f, 0.03f, 0.09f));
    // Left arm
    addCylinder(p,n,uv,idx, glm::vec3(-0.22f, 0.95f, 0), 0.04f, 0.48f, 6);
    // Right arm
    addCylinder(p,n,uv,idx, glm::vec3(0.22f, 0.95f, 0), 0.04f, 0.48f, 6);
    // Left hand (small sphere)
    addSphere(p,n,uv,idx, glm::vec3(-0.22f, 0.93f, 0), 0.04f, 5, 3);
    // Right hand (small sphere)
    addSphere(p,n,uv,idx, glm::vec3(0.22f, 0.93f, 0), 0.04f, 5, 3);
    m_personMesh.upload(p,n,uv,idx);
}

void Scene::buildTrafficLightMesh() {
    // Pole: tall thin cylinder
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addCylinder(p,n,uv,idx, glm::vec3(0, 0, 0), 0.06f, 4.5f, 8);
        m_trafficLightPole.upload(p,n,uv,idx);
    }
    // Head: housing box that holds 3 lights (tall narrow box)
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        // Main housing
        addBox(p,n,uv,idx, glm::vec3(0, 5.35f, 0), glm::vec3(0.22f, 0.65f, 0.15f));
        // Visor/hood on top
        addBox(p,n,uv,idx, glm::vec3(0, 6.05f, -0.08f), glm::vec3(0.25f, 0.04f, 0.22f));
        // Horizontal arm from pole to head
        addBox(p,n,uv,idx, glm::vec3(0, 4.5f, -0.5f), glm::vec3(0.05f, 0.05f, 0.5f));
        m_trafficLightHead.upload(p,n,uv,idx);
    }
    // Bulb: small sphere for each light
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addSphere(p,n,uv,idx, glm::vec3(0, 0, 0), 0.1f, 8, 6);
        m_trafficLightBulb.upload(p,n,uv,idx);
    }
}

void Scene::buildStreetLampMesh() {
    // Pole: tall thin cylinder
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addCylinder(p,n,uv,idx, glm::vec3(0, 0, 0), 0.06f, 5.8f, 8);
        m_streetLampPole.upload(p,n,uv,idx);
    }
    // Head: lamp housing + curved arm
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        // Horizontal arm
        addBox(p,n,uv,idx, glm::vec3(0, 5.9f, 0.3f), glm::vec3(0.04f, 0.04f, 0.35f));
        // Lamp housing
        addBox(p,n,uv,idx, glm::vec3(0, 5.8f, 0.6f), glm::vec3(0.15f, 0.06f, 0.15f));
        // Bulb (small sphere under housing)
        addSphere(p,n,uv,idx, glm::vec3(0, 5.7f, 0.6f), 0.08f, 6, 4);
        m_streetLampHead.upload(p,n,uv,idx);
    }
}

// ─── init ────────────────────────────────────────────────────────
void Scene::initTrafficAndPedestrians() {
    if (m_carModels.empty()) return;

    // --- Traffic lights at intersections ---
    m_trafficLights.clear();
    buildTrafficLightMesh();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disTLGreen(8, 11);
    std::uniform_int_distribution<> disBinary(0, 1);
    std::uniform_int_distribution<> disWalkCycle(0, 99);
    std::uniform_real_distribution<float> disPedSpeed(0.8f, 1.7f);

    // Place traffic lights at the 4 corners of the main intersection (0,0)
    float offsets[] = { -8.0f, 8.0f };
    for (float ox : offsets) {
        for (float oz : offsets) {
            TrafficLight tl;
            tl.pos = glm::vec3(ox, 0.0f, oz);
            tl.phase = TrafficPhase::GreenNS;
            tl.timer = 0.0f;
            tl.greenDuration = static_cast<float>(disTLGreen(gen));
            tl.yellowDuration = 2.0f;
            m_trafficLights.push_back(tl);
        }
    }

    // --- Cars: 12 total (3 per lane), all 3 models guaranteed via round-robin ---
    m_cars.clear();
    const int numModels = static_cast<int>(m_carModels.size());
    const int carsPerLane = 3;
    // 4 lanes: NS x=-3 (dir +Z), NS x=3 (dir -Z), EW z=-3 (dir +X), EW z=3 (dir -X)
    struct LaneDef { float laneCoord; bool isNS; float dirSign; };
    LaneDef lanes[] = {
        { -3.0f, true,   1.0f },   // NS lane x=-3, going +Z
        {  3.0f, true,  -1.0f },   // NS lane x=3,  going -Z
        { -3.0f, false,  1.0f },   // EW lane z=-3, going +X
        {  3.0f, false, -1.0f },   // EW lane z=3,  going -X
    };
    std::uniform_real_distribution<float> disSpeed(7.0f, 13.0f);
    int modelCounter = 0;
    for (const auto& lane : lanes) {
        for (int j = 0; j < carsPerLane; ++j) {
            Car c;
            // Round-robin model assignment: guarantees all 3 models appear
            c.modelIdx = modelCounter % numModels;
            modelCounter++;
            c.speed = disSpeed(gen);
            c.scale = m_carScale;
            c.stopped = false;

            // Spread cars along the lane, far from intersection (±20..±140)
            float spread = 20.0f + static_cast<float>(j) * 45.0f;
            if (lane.isNS) {
                c.pos = glm::vec3(lane.laneCoord, 0.0f, -lane.dirSign * spread);
                c.dir = glm::vec3(0.0f, 0.0f, lane.dirSign);
                c.rotOffset = (lane.dirSign > 0) ? 0.0f : PI;
            } else {
                c.pos = glm::vec3(-lane.dirSign * spread, 0.0f, lane.laneCoord);
                c.dir = glm::vec3(lane.dirSign, 0.0f, 0.0f);
                c.rotOffset = (lane.dirSign > 0) ? PI * 0.5f : -PI * 0.5f;
            }
            m_cars.push_back(c);
        }
    }

    // --- Pedestrians ---
    buildPersonMesh();
    m_pedestrians.clear();
    std::uniform_real_distribution<float> disPedPos(-150.0f, 150.0f);
    for (int i = 0; i < 120; ++i) {
        Pedestrian pd;
        if (i % 2 == 0) {
            float sidewalk = (disBinary(gen) == 0) ? -7.0f : 7.0f;
            pd.pos = glm::vec3(sidewalk, 0.0f, disPedPos(gen));
            float zDir = (disBinary(gen) == 0) ? 1.0f : -1.0f;
            pd.dir = glm::vec3(0, 0, zDir);
        } else {
            float sidewalk = (disBinary(gen) == 0) ? -7.0f : 7.0f;
            pd.pos = glm::vec3(disPedPos(gen), 0.0f, sidewalk);
            float xDir = (disBinary(gen) == 0) ? 1.0f : -1.0f;
            pd.dir = glm::vec3(xDir, 0, 0);
        }
        pd.speed = disPedSpeed(gen);
        pd.walkCycle = static_cast<float>(disWalkCycle(gen));
        m_pedestrians.push_back(pd);
    }

    // --- Street lamps along sidewalks ---
    buildStreetLampMesh();
    m_streetLamps.clear();
    const float lampSpacing = 25.0f;
    for (float z = -125.0f; z <= 125.0f; z += lampSpacing) {
        m_streetLamps.push_back({{-9.0f, 0.0f, z}});
        m_streetLamps.push_back({{ 9.0f, 0.0f, z}});
    }
    for (float x = -125.0f; x <= 125.0f; x += lampSpacing) {
        // Skip lamps too close to the NS road lamps
        if (std::abs(x) < 2.0f) continue;
        m_streetLamps.push_back({{x, 0.0f, -9.0f}});
        m_streetLamps.push_back({{x, 0.0f,  9.0f}});
    }
}

// ─── update ──────────────────────────────────────────────────────
void Scene::update(float dt) {
    // Update traffic light state machines
    for (auto& tl : m_trafficLights) {
        tl.timer += dt;
        switch (tl.phase) {
        case TrafficPhase::GreenNS:
            if (tl.timer >= tl.greenDuration) { tl.phase = TrafficPhase::YellowNS; tl.timer = 0; }
            break;
        case TrafficPhase::YellowNS:
            if (tl.timer >= tl.yellowDuration) { tl.phase = TrafficPhase::GreenEW; tl.timer = 0; }
            break;
        case TrafficPhase::GreenEW:
            if (tl.timer >= tl.greenDuration) { tl.phase = TrafficPhase::YellowEW; tl.timer = 0; }
            break;
        case TrafficPhase::YellowEW:
            if (tl.timer >= tl.yellowDuration) { tl.phase = TrafficPhase::GreenNS; tl.timer = 0; }
            break;
        }
    }

    // All traffic lights share the same phase (single intersection)
    bool nsGreen = false;
    bool ewGreen = false;
    if (!m_trafficLights.empty()) {
        nsGreen = m_trafficLights[0].isGreenNS();
        ewGreen = m_trafficLights[0].isGreenEW();
    }

    // Stop line positions: cars must stop at ±STOP_LINE before entering intersection
    static constexpr float STOP_LINE = 6.0f;   // Distance from center (0,0) to stop line
    static constexpr float INTERSECTION_EXIT = 8.0f; // Past this, car has exited intersection

    for (auto& c : m_cars) {
        const bool isNS = (std::abs(c.dir.z) > 0.5f);
        c.stopped = false;

        // --- Traffic light stop logic ---
        // For NS cars: check their Z position relative to the stop line
        // For EW cars: check their X position relative to the stop line
        if (isNS && !nsGreen) {
            // NS car, red light: must stop at the stop line
            float carAxis = c.pos.z;
            float stopPos = (c.dir.z > 0) ? -STOP_LINE : STOP_LINE;
            bool approaching = (c.dir.z > 0) ? (carAxis < stopPos) : (carAxis > stopPos);
            float distToStop = std::abs(carAxis - stopPos);
            if (approaching && distToStop < 25.0f) {
                c.stopped = true;
            }
        } else if (!isNS && !ewGreen) {
            float carAxis = c.pos.x;
            float stopPos = (c.dir.x > 0) ? -STOP_LINE : STOP_LINE;
            bool approaching = (c.dir.x > 0) ? (carAxis < stopPos) : (carAxis > stopPos);
            float distToStop = std::abs(carAxis - stopPos);
            if (approaching && distToStop < 25.0f) {
                c.stopped = true;
            }
        }

        // --- Anti-collision: check cars ahead on same lane ---
        if (!c.stopped) {
            for (const auto& other : m_cars) {
                if (&c == &other) continue;
                if (glm::dot(c.dir, other.dir) < 0.9f) continue; // not same direction

                // Must be on the same lane (similar perpendicular coordinate)
                if (isNS) {
                    if (std::abs(c.pos.x - other.pos.x) > 1.5f) continue;
                } else {
                    if (std::abs(c.pos.z - other.pos.z) > 1.5f) continue;
                }

                glm::vec3 toOther = other.pos - c.pos;
                float ahead = glm::dot(toOther, c.dir);
                if (ahead > 0.0f && ahead < 8.0f) {
                    c.stopped = true;
                    break;
                }
            }
        }

        if (!c.stopped) {
            c.pos += c.dir * (c.speed * dt);
            if (c.pos.x > CITY_HALF) c.pos.x -= 2*CITY_HALF;
            if (c.pos.x < -CITY_HALF) c.pos.x += 2*CITY_HALF;
            if (c.pos.z > CITY_HALF) c.pos.z -= 2*CITY_HALF;
            if (c.pos.z < -CITY_HALF) c.pos.z += 2*CITY_HALF;
        }
    }

    // Update pedestrians (stop at red lights before crossing)
    for (auto& pd : m_pedestrians) {
        bool pedStopped = false;
        // Pedestrians walking along Z (on x=±7 sidewalks)
        bool walkingZ = (std::abs(pd.dir.z) > 0.5f);
        // Pedestrians walking along X (on z=±7 sidewalks)
        bool walkingX = (std::abs(pd.dir.x) > 0.5f);

        // Pedestrians on NS sidewalks crossing EW road: stop if EW has green (cars moving on EW)
        if (walkingZ && ewGreen) {
            float distToCrossing = std::abs(pd.pos.z);
            bool approaching = (pd.dir.z > 0 && pd.pos.z < -5.0f) || (pd.dir.z < 0 && pd.pos.z > 5.0f);
            if (approaching && distToCrossing < 12.0f) pedStopped = true;
        }
        // Pedestrians on EW sidewalks crossing NS road: stop if NS has green
        if (walkingX && nsGreen) {
            float distToCrossing = std::abs(pd.pos.x);
            bool approaching = (pd.dir.x > 0 && pd.pos.x < -5.0f) || (pd.dir.x < 0 && pd.pos.x > 5.0f);
            if (approaching && distToCrossing < 12.0f) pedStopped = true;
        }

        if (!pedStopped) {
            pd.pos += pd.dir * (pd.speed * dt);
            pd.walkCycle += dt * pd.speed * 2.0f;
        }
        if (pd.pos.x > CITY_HALF) pd.pos.x -= 2*CITY_HALF;
        if (pd.pos.x < -CITY_HALF) pd.pos.x += 2*CITY_HALF;
        if (pd.pos.z > CITY_HALF) pd.pos.z -= 2*CITY_HALF;
        if (pd.pos.z < -CITY_HALF) pd.pos.z += 2*CITY_HALF;
    }
}

// ─── draw ────────────────────────────────────────────────────────
void Scene::drawCars(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    for (const auto& c : m_cars) {
        if (glm::distance(camPos, c.pos) > 150.0f) continue;
        if (c.modelIdx >= (int)m_carModels.size()) continue;
        glm::mat4 model = world;
        model = glm::translate(model, c.pos + glm::vec3(0, m_carYOffset, 0));
        model = glm::rotate(model, c.rotOffset, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(c.scale));
        m_carModels[c.modelIdx].draw(shader, model);
    }
}

void Scene::drawPedestrians(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    for (const auto& pd : m_pedestrians) {
        if (glm::distance(camPos, pd.pos) > 120.0f) continue;
        glm::mat4 model = world;
        float bob = std::abs(std::sin(pd.walkCycle * PI)) * 0.08f;
        model = glm::translate(model, pd.pos + glm::vec3(0, bob, 0));
        float rot = std::atan2(pd.dir.x, pd.dir.z);
        model = glm::rotate(model, rot, glm::vec3(0, 1, 0));
        float sway = std::sin(pd.walkCycle * PI) * 0.04f;
        model = glm::rotate(model, sway, glm::vec3(0, 0, 1));
        shader.setMat4("uModel", model);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(model))));
        // Clothing color from position hash
        float rV = std::sin(pd.pos.x * 12.9898f) * 43758.5453f;
        float gV = std::sin(pd.pos.z * 78.233f) * 43758.5453f;
        float tR = std::abs(rV - std::floor(rV));
        float tG = std::abs(gV - std::floor(gV));
        shader.setVec4("uBaseColorFactor", glm::vec4(0.15f+tR*0.55f, 0.15f+tG*0.55f, 0.25f, 1.0f));
        m_personMesh.draw();
    }
}

void Scene::drawTrafficLights(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    for (const auto& tl : m_trafficLights) {
        if (glm::distance(camPos, tl.pos) > 160.0f) continue;
        glm::mat4 base = glm::translate(world, tl.pos);

        // Pole (dark metallic gray)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.25f, 0.25f, 0.27f, 1.0f));
        shader.setMat4("uModel", base);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(base))));
        m_trafficLightPole.draw();

        // Head housing (dark charcoal)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
        shader.setMat4("uModel", base);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(base))));
        m_trafficLightHead.draw();

        // 3 bulbs: Red (top y=5.75), Yellow (mid y=5.35), Green (bot y=4.95)
        float bulbZ = 0.16f; // slightly in front of the housing
        bool isGreen = (tl.phase == TrafficPhase::GreenNS || tl.phase == TrafficPhase::GreenEW);
        bool isYellow = (tl.phase == TrafficPhase::YellowNS || tl.phase == TrafficPhase::YellowEW);
        bool isRed = !isGreen && !isYellow;

        // Red bulb
        glm::vec3 redCol = isRed ? glm::vec3(3.0f, 0.05f, 0.05f) : glm::vec3(0.15f, 0.02f, 0.02f);
        shader.setVec4("uBaseColorFactor", glm::vec4(redCol, 1.0f));
        glm::mat4 rb = glm::translate(base, glm::vec3(0, 5.75f, bulbZ));
        shader.setMat4("uModel", rb);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(rb))));
        m_trafficLightBulb.draw();

        // Yellow bulb
        glm::vec3 ylwCol = isYellow ? glm::vec3(3.0f, 2.5f, 0.05f) : glm::vec3(0.12f, 0.10f, 0.02f);
        shader.setVec4("uBaseColorFactor", glm::vec4(ylwCol, 1.0f));
        glm::mat4 yb = glm::translate(base, glm::vec3(0, 5.35f, bulbZ));
        shader.setMat4("uModel", yb);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(yb))));
        m_trafficLightBulb.draw();

        // Green bulb
        glm::vec3 grnCol = isGreen ? glm::vec3(0.05f, 3.0f, 0.05f) : glm::vec3(0.02f, 0.12f, 0.02f);
        shader.setVec4("uBaseColorFactor", glm::vec4(grnCol, 1.0f));
        glm::mat4 gb = glm::translate(base, glm::vec3(0, 4.95f, bulbZ));
        shader.setMat4("uModel", gb);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(gb))));
        m_trafficLightBulb.draw();
    }
}

void Scene::setInstanceTransforms(std::vector<glm::mat4> transforms) {
    m_instanceTransforms = std::move(transforms);
}

void Scene::render(Shader& shader, const Camera& camera) const {
    shader.use();
    shader.setMat4("uView", camera.viewMatrix());
    shader.setMat4("uProjection", camera.projectionMatrix(m_aspect));
    shader.setVec3("uLightDir", glm::normalize(m_lightDir));
    shader.setVec3("uSkyColor", m_skyColor);
    shader.setVec3("uGroundColor", m_groundColor);
    shader.setVec3("uSunColor", m_sunColor);
    shader.setFloat("uExposure", m_exposure);
    shader.setVec3("uViewPos", camera.position());
    shader.setVec3("uFogColor", m_fogColor);
    shader.setFloat("uFogDensity", m_fogDensity);
    shader.setFloat("uFogBaseHeight", m_fogBaseHeight);
    shader.setFloat("uFogHeightFalloff", m_fogHeightFalloff);
    shader.setFloat("uWetness", m_wetness);
    shader.setFloat("uLightning", m_lightning);

    if (m_debugDrawCarOnly && !m_carModels.empty()) {
        int idx = std::clamp(m_debugCarIndex, 0, (int)m_carModels.size() - 1);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0, m_carYOffset, 0));
        model = glm::scale(model, glm::vec3(m_carScale));
        m_carModels[idx].draw(shader, model);
        return;
    }

    shader.setFloat("uWindowEmissive", m_windowEmissive);
    shader.setFloat("uDayFactor", m_dayFactor);

    // --- Headlights + Tail lights ---
    {
        const int maxHL = 24;
        float nightFactor = 1.0f - m_dayFactor;
        float hlIntensity = nightFactor > 0.15f ? nightFactor * 3.5f : 0.0f;
        shader.setFloat("uHeadlightIntensity", hlIntensity);
        shader.setVec3("uHeadlightColor", glm::vec3(1.0f, 0.95f, 0.8f));

        int hlCount = 0;
        if (hlIntensity > 0.01f) {
            for (const auto& c : m_cars) {
                if (hlCount >= maxHL) break;
                glm::vec3 perp = glm::normalize(glm::cross(c.dir, glm::vec3(0, 1, 0)));
                glm::vec3 headPos = c.pos + glm::vec3(0, 1.2f, 0) + c.dir * 2.0f;
                glm::vec3 hlDir = glm::normalize(c.dir + glm::vec3(0, -0.15f, 0));

                shader.setVec3(("uHeadlightPos[" + std::to_string(hlCount) + "]").c_str(), headPos - perp * 0.8f);
                shader.setVec3(("uHeadlightDir[" + std::to_string(hlCount) + "]").c_str(), hlDir);
                hlCount++;
                if (hlCount >= maxHL) break;

                shader.setVec3(("uHeadlightPos[" + std::to_string(hlCount) + "]").c_str(), headPos + perp * 0.8f);
                shader.setVec3(("uHeadlightDir[" + std::to_string(hlCount) + "]").c_str(), hlDir);
                hlCount++;
            }
        }
        shader.setInt("uHeadlightCount", hlCount);

        // --- Tail lights (red, brighter when stopped) ---
        const int maxTL = 24;
        int tlCount = 0;
        if (hlIntensity > 0.01f) {
            for (const auto& c : m_cars) {
                if (tlCount >= maxTL) break;
                glm::vec3 perp = glm::normalize(glm::cross(c.dir, glm::vec3(0, 1, 0)));
                glm::vec3 tailPos = c.pos + glm::vec3(0, 0.8f, 0) - c.dir * 2.0f;
                float brakeBoost = c.stopped ? 3.0f : 1.0f;

                shader.setVec3(("uTailLightPos[" + std::to_string(tlCount) + "]").c_str(), tailPos - perp * 0.7f);
                shader.setFloat(("uTailLightBrake[" + std::to_string(tlCount) + "]").c_str(), brakeBoost);
                tlCount++;
                if (tlCount >= maxTL) break;

                shader.setVec3(("uTailLightPos[" + std::to_string(tlCount) + "]").c_str(), tailPos + perp * 0.7f);
                shader.setFloat(("uTailLightBrake[" + std::to_string(tlCount) + "]").c_str(), brakeBoost);
                tlCount++;
            }
        }
        shader.setInt("uTailLightCount", tlCount);
        shader.setFloat("uTailLightIntensity", hlIntensity * 0.6f);

        // --- Street lamps ---
        const int maxSL = 32;
        int slCount = 0;
        if (nightFactor > 0.1f) {
            for (const auto& lamp : m_streetLamps) {
                if (slCount >= maxSL) break;
                shader.setVec3(("uStreetLampPos[" + std::to_string(slCount) + "]").c_str(), lamp.lightWorldPos());
                slCount++;
            }
        }
        shader.setInt("uStreetLampCount", slCount);
        shader.setFloat("uStreetLampIntensity", nightFactor > 0.1f ? nightFactor * 4.0f : 0.0f);
        shader.setVec3("uStreetLampColor", glm::vec3(1.0f, 0.82f, 0.45f)); // warm sodium orange
    }

    for (const glm::mat4& world : m_instanceTransforms) {
        m_cityModel.draw(shader, world);
        drawCars(shader, world, camera.position());
        drawPedestrians(shader, world, camera.position());
        drawTrafficLights(shader, world, camera.position());
        drawStreetLamps(shader, world, camera.position());
    }
}

void Scene::drawStreetLamps(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    for (const auto& lamp : m_streetLamps) {
        if (glm::distance(camPos, lamp.pos) > 180.0f) continue;
        glm::mat4 base = glm::translate(world, lamp.pos);

        // Pole (dark metallic)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.25f, 0.25f, 0.27f, 1.0f));
        shader.setMat4("uModel", base);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(base))));
        m_streetLampPole.draw();

        // Lamp head (emissive at night)
        float nightFactor = 1.0f - m_dayFactor;
        float emissive = nightFactor > 0.15f ? nightFactor * 5.0f : 0.0f;
        glm::vec3 lampCol = (emissive > 0.1f)
            ? glm::vec3(1.0f, 0.82f, 0.45f) * emissive
            : glm::vec3(0.2f, 0.2f, 0.22f);
        shader.setVec4("uBaseColorFactor", glm::vec4(lampCol, 1.0f));
        shader.setMat4("uModel", base);
        m_streetLampHead.draw();
    }
}
