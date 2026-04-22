#pragma once

#include "Mesh.h"
#include "Shader.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct GpuMaterial {
    GLuint diffuseTexture = 0;
    bool useTexture = false;
    glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    std::string name;
};

class Model;

namespace gltf {
// Loads a binary glTF (.glb). Populates meshes/materials on GPU. Returns false on error.
bool loadGLB(const std::string& path, Model& model, std::string& errorMessage);
}

class Model {
    friend bool gltf::loadGLB(const std::string&, Model&, std::string&);

public:
    Model() = default;
    ~Model() = default;

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    bool loadFromGLB(const std::string& path);

    void draw(Shader& shader, const glm::mat4& world) const;

    const std::vector<Mesh>& meshes() const { return m_meshes; }
    const std::vector<GpuMaterial>& materials() const { return m_materials; }

    const std::string& sourcePath() const { return m_sourcePath; }

    void clear();

private:
    std::string m_sourcePath;
    std::vector<Mesh> m_meshes;
    std::vector<GpuMaterial> m_materials;
};
