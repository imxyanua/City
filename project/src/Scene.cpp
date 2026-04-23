#include "Scene.h"
#include <iostream>
#include <string>
#include <utility>
#include <cmath>
#include <cstdlib>
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

// ─── init ────────────────────────────────────────────────────────
void Scene::initTrafficAndPedestrians() {
    if (m_carModels.empty()) return;

    // --- Traffic lights at intersections ---
    m_trafficLights.clear();
    buildTrafficLightMesh();

    // Place traffic lights at the 4 corners of the main intersection (0,0)
    float offsets[] = { -8.0f, 8.0f };
    for (float ox : offsets) {
        for (float oz : offsets) {
            TrafficLight tl;
            tl.pos = glm::vec3(ox, 0.0f, oz);
            tl.phase = TrafficPhase::GreenNS;
            tl.timer = 0.0f;
            tl.greenDuration = 8.0f + (rand()%4);
            tl.yellowDuration = 2.0f;
            m_trafficLights.push_back(tl);
        }
    }

    // --- Cars on lanes ---
    // NS lanes: x = -3, x = 3 (driving along Z)
    // EW lanes: z = -3, z = 3 (driving along X)
    m_cars.clear();
    for (int i = 0; i < 40; ++i) {
        Car c;
        c.modelIdx = rand() % (int)m_carModels.size();
        c.speed = 6.0f + (rand() % 10);
        c.scale = m_carScale;
        c.stopped = false;

        if (i % 2 == 0) {
            // NS lane
            float lane = (i % 4 < 2) ? -3.0f : 3.0f;
            float zDir = (lane < 0) ? 1.0f : -1.0f;
            c.pos = glm::vec3(lane, 0.0f, (float)((rand()%300)-150));
            c.dir = glm::vec3(0, 0, zDir);
            c.rotOffset = (zDir > 0) ? 0.0f : PI;
        } else {
            // EW lane
            float lane = (i % 4 < 2) ? -3.0f : 3.0f;
            float xDir = (lane < 0) ? 1.0f : -1.0f;
            c.pos = glm::vec3((float)((rand()%300)-150), 0.0f, lane);
            c.dir = glm::vec3(xDir, 0, 0);
            c.rotOffset = (xDir > 0) ? PI*0.5f : -PI*0.5f;
        }
        m_cars.push_back(c);
    }

    // --- Pedestrians ---
    buildPersonMesh();
    m_pedestrians.clear();
    for (int i = 0; i < 120; ++i) {
        Pedestrian pd;
        // Walk on sidewalks (offset from road center)
        if (i % 2 == 0) {
            float sidewalk = (rand()%2==0) ? -7.0f : 7.0f;
            pd.pos = glm::vec3(sidewalk, 0.0f, (float)((rand()%300)-150));
            float zDir = (rand()%2==0) ? 1.0f : -1.0f;
            pd.dir = glm::vec3(0, 0, zDir);
        } else {
            float sidewalk = (rand()%2==0) ? -7.0f : 7.0f;
            pd.pos = glm::vec3((float)((rand()%300)-150), 0.0f, sidewalk);
            float xDir = (rand()%2==0) ? 1.0f : -1.0f;
            pd.dir = glm::vec3(xDir, 0, 0);
        }
        pd.speed = 0.8f + (rand()%10)/10.0f;
        pd.walkCycle = (float)(rand()%100);
        m_pedestrians.push_back(pd);
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

    // Update cars - check traffic lights
    for (auto& c : m_cars) {
        bool isNS = (std::abs(c.dir.z) > 0.5f); // driving North-South
        c.stopped = false;

        // Check each traffic light
        for (const auto& tl : m_trafficLights) {
            float distToLight;
            if (isNS) {
                distToLight = std::abs(c.pos.z - tl.pos.z);
                // Only care if on same lane side
                if (std::abs(c.pos.x - tl.pos.x) > 15.0f) continue;
            } else {
                distToLight = std::abs(c.pos.x - tl.pos.x);
                if (std::abs(c.pos.z - tl.pos.z) > 15.0f) continue;
            }
            // If close to the light and approaching it
            if (distToLight < STOP_DIST && distToLight > 1.0f) {
                bool approaching = false;
                if (isNS) approaching = (c.dir.z > 0 && c.pos.z < tl.pos.z) || (c.dir.z < 0 && c.pos.z > tl.pos.z);
                else      approaching = (c.dir.x > 0 && c.pos.x < tl.pos.x) || (c.dir.x < 0 && c.pos.x > tl.pos.x);

                if (approaching) {
                    bool red = isNS ? !tl.isGreenNS() : !tl.isGreenEW();
                    if (red) { c.stopped = true; break; }
                }
            }
        }

        // Check for other cars ahead in the same lane to avoid merging into each other
        if (!c.stopped) {
            for (const auto& other : m_cars) {
                if (&c == &other) continue;
                // If they are driving in the exact same direction
                if (glm::dot(c.dir, other.dir) > 0.9f) {
                    glm::vec3 toOther = other.pos - c.pos;
                    float dist = glm::length(toOther);
                    // Safe distance of 10 units (approx 2 car lengths)
                    if (dist < 10.0f && glm::dot(toOther, c.dir) > 0.0f) {
                        c.stopped = true;
                        break;
                    }
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

    // Update pedestrians
    for (auto& pd : m_pedestrians) {
        pd.pos += pd.dir * (pd.speed * dt);
        pd.walkCycle += dt * pd.speed * 2.0f;
        if (pd.pos.x > CITY_HALF) pd.pos.x -= 2*CITY_HALF;
        if (pd.pos.x < -CITY_HALF) pd.pos.x += 2*CITY_HALF;
        if (pd.pos.z > CITY_HALF) pd.pos.z -= 2*CITY_HALF;
        if (pd.pos.z < -CITY_HALF) pd.pos.z += 2*CITY_HALF;
    }
}

// ─── draw ────────────────────────────────────────────────────────
void Scene::drawCars(Shader& shader, const glm::mat4& world) const {
    for (const auto& c : m_cars) {
        if (c.modelIdx >= (int)m_carModels.size()) continue;
        glm::mat4 model = world;
        model = glm::translate(model, c.pos + glm::vec3(0, m_carYOffset, 0));
        model = glm::rotate(model, c.rotOffset, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(c.scale));
        m_carModels[c.modelIdx].draw(shader, model);
    }
}

void Scene::drawPedestrians(Shader& shader, const glm::mat4& world) const {
    shader.setBool("uUseTexture", false);
    for (const auto& pd : m_pedestrians) {
        glm::mat4 model = world;
        float bob = std::abs(std::sin(pd.walkCycle * PI)) * 0.08f;
        model = glm::translate(model, pd.pos + glm::vec3(0, bob, 0));
        float rot = std::atan2(pd.dir.x, pd.dir.z);
        model = glm::rotate(model, rot, glm::vec3(0, 1, 0));
        float sway = std::sin(pd.walkCycle * PI) * 0.04f;
        model = glm::rotate(model, sway, glm::vec3(0, 0, 1));
        shader.setMat4("uModel", model);
        // Clothing color from position hash
        float rV = std::sin(pd.pos.x * 12.9898f) * 43758.5453f;
        float gV = std::sin(pd.pos.z * 78.233f) * 43758.5453f;
        float tR = std::abs(rV - std::floor(rV));
        float tG = std::abs(gV - std::floor(gV));
        shader.setVec4("uBaseColorFactor", glm::vec4(0.15f+tR*0.55f, 0.15f+tG*0.55f, 0.25f, 1.0f));
        m_personMesh.draw();
    }
}

void Scene::drawTrafficLights(Shader& shader, const glm::mat4& world) const {
    shader.setBool("uUseTexture", false);
    for (const auto& tl : m_trafficLights) {
        glm::mat4 base = glm::translate(world, tl.pos);

        // Pole (dark metallic gray)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.25f, 0.25f, 0.27f, 1.0f));
        shader.setMat4("uModel", base);
        m_trafficLightPole.draw();

        // Head housing (dark charcoal)
        shader.setVec4("uBaseColorFactor", glm::vec4(0.08f, 0.08f, 0.08f, 1.0f));
        shader.setMat4("uModel", base);
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
        m_trafficLightBulb.draw();

        // Yellow bulb
        glm::vec3 ylwCol = isYellow ? glm::vec3(3.0f, 2.5f, 0.05f) : glm::vec3(0.12f, 0.10f, 0.02f);
        shader.setVec4("uBaseColorFactor", glm::vec4(ylwCol, 1.0f));
        glm::mat4 yb = glm::translate(base, glm::vec3(0, 5.35f, bulbZ));
        shader.setMat4("uModel", yb);
        m_trafficLightBulb.draw();

        // Green bulb
        glm::vec3 grnCol = isGreen ? glm::vec3(0.05f, 3.0f, 0.05f) : glm::vec3(0.02f, 0.12f, 0.02f);
        shader.setVec4("uBaseColorFactor", glm::vec4(grnCol, 1.0f));
        glm::mat4 gb = glm::translate(base, glm::vec3(0, 4.95f, bulbZ));
        shader.setMat4("uModel", gb);
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

    for (const glm::mat4& world : m_instanceTransforms) {
        m_cityModel.draw(shader, world);
        drawCars(shader, world);
        drawPedestrians(shader, world);
        drawTrafficLights(shader, world);
    }
}
