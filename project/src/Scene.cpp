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

static void addCone(std::vector<float>& p, std::vector<float>& n, std::vector<float>& uv,
                   std::vector<unsigned int>& idx, glm::vec3 base_center, float radius, float height, int segs=12) {
    unsigned int b = (unsigned int)(p.size()/3);
    glm::vec3 tip = base_center + glm::vec3(0, height, 0);
    for(int i=0; i<segs; ++i) {
        float a0 = 2.0f*PI*i/segs, a1 = 2.0f*PI*(i+1)/segs;
        float c0=std::cos(a0), s0=std::sin(a0), c1=std::cos(a1), s1=std::sin(a1);
        glm::vec3 v0(base_center.x + radius*c0, base_center.y, base_center.z + radius*s0);
        glm::vec3 v1(base_center.x + radius*c1, base_center.y, base_center.z + radius*s1);
        
        glm::vec3 nm = glm::normalize(glm::cross(v1 - v0, tip - v0));
        unsigned int bi = (unsigned int)(p.size()/3);
        auto pushV=[&](glm::vec3 vv, glm::vec3 nn){ p.push_back(vv.x);p.push_back(vv.y);p.push_back(vv.z);
            n.push_back(nn.x);n.push_back(nn.y);n.push_back(nn.z); uv.push_back(0);uv.push_back(0);};
        pushV(v0, nm); pushV(v1, nm); pushV(tip, nm);
        idx.push_back(bi); idx.push_back(bi+1); idx.push_back(bi+2);
    }
}

// ─── build meshes ────────────────────────────────────────────────
void Scene::buildPedestrianMeshes() {
    // Body (Torso)
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addCylinder(p,n,uv,idx, glm::vec3(0, 0.9f, 0), 0.18f, 0.55f, 8); // Torso
        addBox(p,n,uv,idx, glm::vec3(0, 1.42f, 0), glm::vec3(0.24f, 0.04f, 0.08f)); // Shoulders
        m_pedBodyMesh.upload(p,n,uv,idx);
    }
    // Head
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addSphere(p,n,uv,idx, glm::vec3(0, 1.65f, 0), 0.12f, 8, 6); // Head
        addCylinder(p,n,uv,idx, glm::vec3(0, 1.45f, 0), 0.045f, 0.08f, 6); // Neck
        m_pedHeadMesh.upload(p,n,uv,idx);
    }
    // Limb (Leg/Arm segment)
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addCylinder(p,n,uv,idx, glm::vec3(0, 0, 0), 0.05f, 1.0f, 6); // Generic 1-unit limb
        m_pedLimbMesh.upload(p,n,uv,idx);
    }
    // Eye
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addSphere(p,n,uv,idx, glm::vec3(0, 0, 0), 0.025f, 4, 3);
        m_pedEyeMesh.upload(p,n,uv,idx);
    }
    // Hair (Basic mop)
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addSphere(p,n,uv,idx, glm::vec3(0, 0, 0), 0.13f, 8, 6);
        addBox(p,n,uv,idx, glm::vec3(0, -0.05f, -0.05f), glm::vec3(0.12f, 0.08f, 0.1f));
        m_pedHairMesh.upload(p,n,uv,idx);
    }
    // Backpack
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addBox(p,n,uv,idx, glm::vec3(0, 0, 0), glm::vec3(0.12f, 0.18f, 0.06f));
        m_pedPackMesh.upload(p,n,uv,idx);
    }
    // Umbrella
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addCone(p,n,uv,idx, glm::vec3(0, 0, 0), 1.0f, 0.4f, 16); // Top
        addCylinder(p,n,uv,idx, glm::vec3(0, -1.2f, 0), 0.02f, 1.2f, 6); // Handle
        m_umbrellaMesh.upload(p,n,uv,idx);
    }
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

    // Overhead pole: Vertical part + Horizontal part
    {
        std::vector<float> p,n,uv; std::vector<unsigned int> idx;
        addBox(p,n,uv,idx, glm::vec3(6.5f, 3.5f, -6.5f), glm::vec3(0.15f, 3.5f, 0.15f)); // Vertical
        addBox(p,n,uv,idx, glm::vec3(0.0f, 7.0f, -6.5f), glm::vec3(6.5f, 0.1f, 0.1f));  // Horizontal extension
        m_trafficPoleMesh.upload(p,n,uv,idx);
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

void Scene::buildPigeonMesh() {
    // Simple V-shape for pigeon body/wings
    std::vector<float> p,n,uv; std::vector<unsigned int> idx;
    
    auto pushV=[&](glm::vec3 vv, glm::vec3 nn, glm::vec2 u){ 
        p.push_back(vv.x);p.push_back(vv.y);p.push_back(vv.z);
        n.push_back(nn.x);n.push_back(nn.y);n.push_back(nn.z); 
        uv.push_back(u.x);uv.push_back(u.y);
    };

    // Left wing
    pushV(glm::vec3(0.0f, 0.0f, -0.1f), glm::vec3(0, 1, 0), glm::vec2(0,0));
    pushV(glm::vec3(-0.2f, 0.05f, -0.0f), glm::vec3(0, 1, 0), glm::vec2(0,1));
    pushV(glm::vec3(0.0f, 0.0f, 0.1f), glm::vec3(0, 1, 0), glm::vec2(1,1));
    // Right wing
    pushV(glm::vec3(0.0f, 0.0f, -0.1f), glm::vec3(0, 1, 0), glm::vec2(0,0));
    pushV(glm::vec3(0.0f, 0.0f, 0.1f), glm::vec3(0, 1, 0), glm::vec2(1,1));
    pushV(glm::vec3(0.2f, 0.05f, 0.0f), glm::vec3(0, 1, 0), glm::vec2(1,0));
    
    idx = {0, 1, 2, 3, 4, 5};
    m_pigeonMesh.upload(p,n,uv,idx);
}

void Scene::buildNeonSignMesh() {
    // Simple vertical/horizontal box for neon sign
    std::vector<float> p,n,uv; std::vector<unsigned int> idx;
    addBox(p,n,uv,idx, glm::vec3(0,0,0), glm::vec3(0.5f, 2.0f, 0.1f));
    m_neonSignMesh.upload(p,n,uv,idx);
}

// ─── init ────────────────────────────────────────────────────────
void Scene::initTrafficAndPedestrians() {
    if (m_carModels.empty()) return;

    // --- Traffic lights at intersections ---
    m_trafficLights.clear();
    buildPedestrianMeshes();
    buildTrafficLightMesh();
    buildStreetLampMesh();
    m_splashSystem.init();

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
            c.isPlayerDriven = false;

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
    buildPedestrianMeshes();
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

    // --- Pigeons ---
    buildPigeonMesh();
    m_pigeons.clear();
    std::uniform_real_distribution<float> disPigeonX(-30.0f, 30.0f);
    std::uniform_real_distribution<float> disPigeonZ(-30.0f, 30.0f);
    std::uniform_real_distribution<float> disTimer(0.0f, 2.0f);
    for (int i = 0; i < 30; ++i) {
        Pigeon pg;
        pg.pos = glm::vec3(disPigeonX(gen), 0.05f, disPigeonZ(gen));
        pg.vel = glm::vec3(0);
        pg.state = 0; // pecking
        pg.timer = disTimer(gen);
        pg.flapPhase = 0;
        m_pigeons.push_back(pg);
    }

    // --- Neon Signs ---
    buildNeonSignMesh();
    m_neonSigns.clear();
    // Add neon signs on some building walls
    m_neonSigns.push_back({{ -10.0f, 3.0f, -15.0f }, glm::vec3(1.0f), glm::vec3(1.0f, 0.2f, 0.8f), 0.5f}); // Pink
    m_neonSigns.push_back({{  10.0f, 4.0f, -25.0f }, glm::vec3(1.2f), glm::vec3(0.2f, 0.8f, 1.0f), 0.3f}); // Cyan
    m_neonSigns.push_back({{ -12.0f, 5.0f,  20.0f }, glm::vec3(0.8f), glm::vec3(0.8f, 1.0f, 0.2f), 0.8f}); // Yellow
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
        if (c.isPlayerDriven) {
            // Manual speed interpolation for player (ignoring traffic lights/AI)
            if (c.speed < c.targetSpeed) c.speed = glm::min(c.targetSpeed, c.speed + c.acceleration * 5.0f * dt);
            else if (c.speed > c.targetSpeed) c.speed = glm::max(c.targetSpeed, c.speed - c.acceleration * 10.0f * dt);

            c.pos += c.dir * (c.speed * dt);
            if (c.pos.x > CITY_HALF) c.pos.x -= 2*CITY_HALF;
            if (c.pos.x < -CITY_HALF) c.pos.x += 2*CITY_HALF;
            if (c.pos.z > CITY_HALF) c.pos.z -= 2*CITY_HALF;
            if (c.pos.z < -CITY_HALF) c.pos.z += 2*CITY_HALF;
            continue;
        }

        const bool isNS = (std::abs(c.dir.z) > 0.5f);
        c.stopped = false;

        // --- Traffic light stop logic ---
        if (isNS && !nsGreen) {
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

                if (isNS) {
                    if (std::abs(c.pos.x - other.pos.x) > 1.5f) continue;
                } else {
                    if (std::abs(c.pos.z - other.pos.z) > 1.5f) continue;
                }

                glm::vec3 toOther = other.pos - c.pos;
                float ahead = glm::dot(toOther, c.dir);
                if (ahead > 0.0f && ahead < 12.0f) {
                    c.stopped = true;
                    break;
                }
            }
        }
        
        // --- Anti-collision: pedestrians ---
        if (!c.stopped) {
            for (const auto& pd : m_pedestrians) {
                glm::vec3 toPd = pd.pos - c.pos;
                float ahead = glm::dot(toPd, c.dir);
                float side = glm::length(toPd - c.dir * ahead);
                if (ahead > 0.0f && ahead < 10.0f && side < 2.0f) {
                    c.stopped = true;
                    break;
                }
            }
        }

        // Smooth acceleration/deceleration
        if (c.stopped) {
            c.speed = glm::max(0.0f, c.speed - c.acceleration * 3.0f * dt);
        } else {
            c.speed = glm::min(c.targetSpeed, c.speed + c.acceleration * dt);
        }

        c.pos += c.dir * (c.speed * dt);
        if (c.pos.x > CITY_HALF) c.pos.x -= 2*CITY_HALF;
        if (c.pos.x < -CITY_HALF) c.pos.x += 2*CITY_HALF;
        if (c.pos.z > CITY_HALF) c.pos.z -= 2*CITY_HALF;
        if (c.pos.z < -CITY_HALF) c.pos.z += 2*CITY_HALF;

        // Emit particles
        if (std::abs(c.speed) > 2.0f && m_wetness > 0.1f) {
            // Emit behind tires
            glm::vec3 right = glm::vec3(-c.dir.z, 0.0f, c.dir.x);
            glm::vec3 tireOffsetL = -c.dir * 1.5f - right * 0.8f;
            glm::vec3 tireOffsetR = -c.dir * 1.5f + right * 0.8f;
            glm::vec3 vel = -c.dir * (c.speed * 0.3f) + glm::vec3(0.0f, c.speed * 0.1f, 0.0f);
            
            // Randomize velocity a bit
            vel.x += ((rand() % 100) / 100.0f - 0.5f) * 0.5f;
            vel.z += ((rand() % 100) / 100.0f - 0.5f) * 0.5f;
            vel.y += ((rand() % 100) / 100.0f) * 1.0f;

            m_splashSystem.emit(c.pos + tireOffsetL, vel, 0.3f + ((rand() % 100)/100.0f)*0.2f, 0.1f * m_wetness);
            m_splashSystem.emit(c.pos + tireOffsetR, vel, 0.3f + ((rand() % 100)/100.0f)*0.2f, 0.1f * m_wetness);
        }
    }

    m_splashSystem.update(dt);

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

    // --- Pigeons Update ---
    for (auto& pg : m_pigeons) {
        pg.timer -= dt;
        if (pg.state == 0) { // Pecking
            if (pg.timer <= 0) {
                pg.timer = 0.5f + ((rand() % 100) / 100.0f) * 1.5f;
            }
            // Check for nearby cars or pedestrians to trigger escape
            bool scared = false;
            for (const auto& c : m_cars) {
                if (glm::distance(c.pos, pg.pos) < 6.0f) { scared = true; break; }
            }
            if (!scared) {
                for (const auto& pd : m_pedestrians) {
                    if (glm::distance(pd.pos, pg.pos) < 4.0f) { scared = true; break; }
                }
            }
            if (scared) {
                pg.state = 1; // Fly
                pg.vel = glm::vec3(((rand() % 100)/100.0f - 0.5f) * 4.0f, 4.0f + ((rand() % 100)/100.0f)*2.0f, ((rand() % 100)/100.0f - 0.5f) * 4.0f);
            }
        } else if (pg.state == 1) { // Flying
            pg.pos += pg.vel * dt;
            pg.vel.y += 2.0f * dt; // accelerate upwards
            pg.flapPhase += dt * 15.0f;
            if (pg.pos.y > 20.0f) {
                // Reset pigeon
                pg.state = 0;
                pg.pos.y = 0.05f;
                pg.pos.x = ((rand() % 100) / 100.0f - 0.5f) * 60.0f;
                pg.pos.z = ((rand() % 100) / 100.0f - 0.5f) * 60.0f;
                pg.timer = 1.0f + ((rand() % 100)/100.0f)*2.0f;
            }
        }
    }
}

// ─── draw ────────────────────────────────────────────────────────
void Scene::drawCars(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    for (const auto& c : m_cars) {
        glm::vec3 globalPos = glm::vec3(world * glm::vec4(c.pos, 1.0f));
        if (glm::distance(camPos, globalPos) > 150.0f) continue;
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
        glm::vec3 globalPos = glm::vec3(world * glm::vec4(pd.pos, 1.0f));
        if (glm::distance(camPos, globalPos) > 120.0f) continue;

        // --- Deterministic randomization based on position ---
        auto hash = [](float x, float z) {
            float v = std::sin(x * 12.9898f + z * 78.233f) * 43758.5453f;
            return v - std::floor(v);
        };
        float h1 = hash(pd.pos.x, pd.pos.z);
        float h2 = hash(pd.pos.z, pd.pos.x);
        float h3 = hash(pd.pos.x + 0.1f, pd.pos.z - 0.1f);

        glm::vec4 skinCol = glm::vec4(0.85f + h1*0.1f, 0.65f + h2*0.1f, 0.45f + h3*0.1f, 1.0f);
        if (h1 > 0.8f) skinCol = glm::vec4(0.35f, 0.25f, 0.15f, 1.0f);
        glm::vec4 clothCol = glm::vec4(0.2f + h2*0.6f, 0.2f + h3*0.6f, 0.2f + h1*0.6f, 1.0f);
        glm::vec4 hairCol = (h3 > 0.5f) ? glm::vec4(0.12f, 0.07f, 0.02f, 1.0f) : glm::vec4(0.55f, 0.35f, 0.15f, 1.0f);

        // --- Animation ---
        float bob = std::abs(std::sin(pd.walkCycle * PI)) * 0.08f;
        float limbSwing = std::sin(pd.walkCycle * PI) * 0.45f;
        float rot = std::atan2(pd.dir.x, pd.dir.z);

        // --- Character Variety Scale ---
        float heightVar = 0.92f + h1 * 0.16f;
        float widthVar = 0.95f + h2 * 0.15f;

        glm::mat4 baseModel = glm::translate(world, pd.pos + glm::vec3(0, bob, 0));
        baseModel = glm::rotate(baseModel, rot, glm::vec3(0, 1, 0));
        baseModel = glm::scale(baseModel, glm::vec3(widthVar, heightVar, widthVar));

        // 1. Body
        shader.setVec4("uBaseColorFactor", clothCol);
        shader.setMat4("uModel", baseModel);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(baseModel))));
        m_pedBodyMesh.draw();

        // 2. Head
        shader.setVec4("uBaseColorFactor", skinCol);
        shader.setMat4("uModel", baseModel);
        m_pedHeadMesh.draw();

        // 2.1 Hair
        shader.setVec4("uBaseColorFactor", hairCol);
        glm::mat4 hairModel = glm::translate(baseModel, glm::vec3(0, 1.7f, 0));
        if (h2 > 0.5f) hairModel = glm::scale(hairModel, glm::vec3(1.1f, 0.8f, 1.1f)); // Different style
        shader.setMat4("uModel", hairModel);
        m_pedHairMesh.draw();

        // 2.2 Eyes (always look in dir)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
        // Left Eye
        glm::mat4 lEye = glm::translate(baseModel, glm::vec3(-0.04f, 1.68f, 0.1f));
        shader.setMat4("uModel", lEye);
        m_pedEyeMesh.draw();
        // Right Eye
        glm::mat4 rEye = glm::translate(baseModel, glm::vec3(0.04f, 1.68f, 0.1f));
        shader.setMat4("uModel", rEye);
        m_pedEyeMesh.draw();

        // 2.3 Backpack (Chance)
        if (h1 > 0.65f) {
            shader.setVec4("uBaseColorFactor", clothCol * 0.4f); // Darker backpack
            glm::mat4 pack = glm::translate(baseModel, glm::vec3(0, 1.15f, -0.18f));
            shader.setMat4("uModel", pack);
            m_pedPackMesh.draw();
        }

        // 3. Legs (swinging)
        shader.setVec4("uBaseColorFactor", clothCol * 0.85f);
        // Left
        glm::mat4 lLeg = glm::translate(baseModel, glm::vec3(-0.1f, 0.85f, 0));
        lLeg = glm::rotate(lLeg, limbSwing, glm::vec3(1, 0, 0));
        lLeg = glm::scale(lLeg, glm::vec3(1, 0.85f, 1)); 
        lLeg = glm::translate(lLeg, glm::vec3(0, -1.0f, 0)); 
        shader.setMat4("uModel", lLeg);
        m_pedLimbMesh.draw();
        // Right
        glm::mat4 rLeg = glm::translate(baseModel, glm::vec3(0.1f, 0.85f, 0));
        rLeg = glm::rotate(rLeg, -limbSwing, glm::vec3(1, 0, 0));
        rLeg = glm::scale(rLeg, glm::vec3(1, 0.85f, 1));
        rLeg = glm::translate(rLeg, glm::vec3(0, -1.0f, 0));
        shader.setMat4("uModel", rLeg);
        m_pedLimbMesh.draw();

        // 4. Arms
        shader.setVec4("uBaseColorFactor", skinCol);
        // Left
        glm::mat4 lArm = glm::translate(baseModel, glm::vec3(-0.22f, 1.45f, 0));
        lArm = glm::rotate(lArm, -limbSwing * 0.7f, glm::vec3(1, 0, 0));
        lArm = glm::scale(lArm, glm::vec3(0.8f, 0.5f, 0.8f));
        lArm = glm::translate(lArm, glm::vec3(0, -1.0f, 0));
        shader.setMat4("uModel", lArm);
        m_pedLimbMesh.draw();
        // Right
        glm::mat4 rArm = glm::translate(baseModel, glm::vec3(0.22f, 1.45f, 0));
        rArm = glm::rotate(rArm, limbSwing * 0.7f, glm::vec3(1, 0, 0));
        rArm = glm::scale(rArm, glm::vec3(0.8f, 0.5f, 0.8f));
        rArm = glm::translate(rArm, glm::vec3(0, -1.0f, 0));
        shader.setMat4("uModel", rArm);
        m_pedLimbMesh.draw();

        // 5. Umbrella
        if (m_wetness > 0.4f) {
            glm::vec4 umbrellaCol = glm::vec4(h1, h2, h3, 1.0f);
            if (glm::length(glm::vec3(umbrellaCol)) < 0.3f) umbrellaCol = glm::vec4(0.9f, 0.1f, 0.1f, 1.0f);
            
            glm::mat4 umb = glm::translate(baseModel, glm::vec3(0.15f, 1.8f, 0));
            umb = glm::rotate(umb, 0.15f, glm::vec3(0, 0, 1));
            shader.setVec4("uBaseColorFactor", umbrellaCol);
            shader.setMat4("uModel", umb);
            shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(umb))));
            m_umbrellaMesh.draw();
        }
    }
}

void Scene::drawTrafficLights(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    for (const auto& tl : m_trafficLights) {
        glm::vec3 globalPos = glm::vec3(world * glm::vec4(tl.pos, 1.0f));
        if (glm::distance(camPos, globalPos) > 160.0f) continue;
        glm::mat4 base = glm::translate(world, tl.pos);

        // Pole (dark metallic gray)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.25f, 0.25f, 0.27f, 1.0f));
        shader.setMat4("uModel", base);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(base))));
        m_trafficLightPole.draw();
        m_trafficPoleMesh.draw();

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
        // Culling for the entire block
        glm::vec3 blockPos = glm::vec3(world[3]); 
        if (glm::distance(camera.position(), blockPos) > 220.0f) continue;

        m_cityModel.draw(shader, world);
        drawCars(shader, world, camera.position());
        drawPedestrians(shader, world, camera.position());
        drawTrafficLights(shader, world, camera.position());
        drawStreetLamps(shader, world, camera.position());
        drawPigeons(shader, world, camera.position());
        drawNeonSigns(shader, world, camera.position());
    }
}

void Scene::renderParticles(Shader& shader, const Camera& camera) {
    m_splashSystem.draw(shader, camera, m_aspect);
}

void Scene::drawStreetLamps(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    for (const auto& lamp : m_streetLamps) {
        glm::vec3 globalPos = glm::vec3(world * glm::vec4(lamp.pos, 1.0f));
        if (glm::distance(camPos, globalPos) > 180.0f) continue;
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

void Scene::drawPigeons(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    shader.setVec4("uBaseColorFactor", glm::vec4(0.4f, 0.4f, 0.45f, 1.0f)); // Greyish blue
    for (const auto& pg : m_pigeons) {
        glm::vec3 globalPos = glm::vec3(world * glm::vec4(pg.pos, 1.0f));
        if (glm::distance(camPos, globalPos) > 100.0f) continue;
        
        glm::mat4 base = glm::translate(world, pg.pos);
        // orient to velocity if flying, else random
        if (pg.state == 1 && glm::length(pg.vel) > 0.1f) {
            float angle = atan2(pg.vel.x, pg.vel.z);
            base = glm::rotate(base, angle, glm::vec3(0, 1, 0));
            // Tilt based on flap phase
            float flap = sin(pg.flapPhase);
            base = glm::scale(base, glm::vec3(1.0f, 1.0f + flap * 0.5f, 1.0f));
        } else {
            // Pecking animation
            float angle = pg.pos.x * 12.0f; // pseudo-random orientation
            base = glm::rotate(base, angle, glm::vec3(0, 1, 0));
            float peck = (pg.timer > 0.0f && pg.timer < 0.2f) ? sin(pg.timer * 30.0f) * 0.5f : 0.0f;
            base = glm::rotate(base, peck, glm::vec3(1, 0, 0));
        }
        shader.setMat4("uModel", base);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(base))));
        m_pigeonMesh.draw();
    }
}

void Scene::drawNeonSigns(Shader& shader, const glm::mat4& world, const glm::vec3& camPos) const {
    shader.setBool("uUseTexture", false);
    for (const auto& ns : m_neonSigns) {
        glm::vec3 globalPos = glm::vec3(world * glm::vec4(ns.pos, 1.0f));
        if (glm::distance(camPos, globalPos) > 150.0f) continue;
        
        glm::mat4 base = glm::translate(world, ns.pos);
        base = glm::scale(base, ns.scale);
        
        // Flicker logic using global dayFactor or random
        float flicker = sin(globalPos.x * 5.0f + m_dayFactor * 100.0f * ns.flickerSpeed);
        flicker = (flicker > 0.8f) ? 0.2f : 1.0f; // occasional off
        
        // Emissive in dark
        float nightFactor = 1.0f - m_dayFactor;
        float emissive = nightFactor > 0.1f ? nightFactor * 8.0f * flicker : 0.0f;
        glm::vec3 col = ns.color * (0.2f + emissive);
        
        shader.setVec4("uBaseColorFactor", glm::vec4(col, 1.0f));
        shader.setMat4("uModel", base);
        shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(base))));
        m_neonSignMesh.draw();
    }
}
