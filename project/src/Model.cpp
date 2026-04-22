#include "Model.h"

#include <iostream>

bool Model::loadFromGLB(const std::string& path)
{
    clear();
    std::string err;
    if (!gltf::loadGLB(path, *this, err)) {
        std::cerr << "[Model] " << err << "\n";
        return false;
    }
    return true;
}

void Model::clear()
{
    for (GpuMaterial& m : m_materials) {
        if (m.diffuseTexture != 0) {
            glDeleteTextures(1, &m.diffuseTexture);
            m.diffuseTexture = 0;
        }
    }
    m_meshes.clear();
    m_materials.clear();
}

void Model::draw(Shader& shader, const glm::mat4& world) const
{
    shader.setMat4("uModel", world);

    for (const Mesh& mesh : m_meshes) {
        int mid = mesh.materialIndex();
        if (mid < 0 || mid >= static_cast<int>(m_materials.size()))
            mid = 0;

        const GpuMaterial& mat = m_materials[static_cast<size_t>(mid)];
        shader.setBool("uUseTexture", mat.useTexture && mat.diffuseTexture != 0);
        shader.setVec4("uBaseColorFactor", mat.baseColorFactor);

        if (mat.useTexture && mat.diffuseTexture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);
            shader.setInt("uDiffuseTex", 0);
        }

        mesh.draw();
    }
}
