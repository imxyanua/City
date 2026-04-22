#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <vector>

// One drawable primitive: VAO + separate VBOs (position, normal, UV) + optional EBO.
class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(
        const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& uvs,
        const std::vector<unsigned int>& indices);

    void draw() const;

    int materialIndex() const { return m_materialIndex; }
    void setMaterialIndex(int idx) { m_materialIndex = idx; }

private:
    void release();

    GLuint m_vao = 0;
    GLuint m_vboPos = 0;
    GLuint m_vboNrm = 0;
    GLuint m_vboUv = 0;
    GLuint m_ebo = 0;
    GLsizei m_indexCount = 0;
    GLenum m_indexType = GL_UNSIGNED_INT;
    bool m_indexed = true;
    int m_materialIndex = 0;
};
