#include "Mesh.h"

#include <iostream>
#include <utility>

Mesh::~Mesh()
{
    release();
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao)
    , m_vboPos(other.m_vboPos)
    , m_vboNrm(other.m_vboNrm)
    , m_vboUv(other.m_vboUv)
    , m_ebo(other.m_ebo)
    , m_indexCount(other.m_indexCount)
    , m_indexType(other.m_indexType)
    , m_indexed(other.m_indexed)
    , m_materialIndex(other.m_materialIndex)
    , m_objectName(std::move(other.m_objectName))
{
    other.m_vao = 0;
    other.m_vboPos = other.m_vboNrm = other.m_vboUv = other.m_ebo = 0;
    other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        release();
        m_vao = other.m_vao;
        m_vboPos = other.m_vboPos;
        m_vboNrm = other.m_vboNrm;
        m_vboUv = other.m_vboUv;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_indexType = other.m_indexType;
        m_indexed = other.m_indexed;
        m_materialIndex = other.m_materialIndex;
        m_objectName = std::move(other.m_objectName);
        other.m_vao = 0;
        other.m_vboPos = other.m_vboNrm = other.m_vboUv = other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Mesh::release()
{
    if (m_ebo)
        glDeleteBuffers(1, &m_ebo);
    if (m_vboUv)
        glDeleteBuffers(1, &m_vboUv);
    if (m_vboNrm)
        glDeleteBuffers(1, &m_vboNrm);
    if (m_vboPos)
        glDeleteBuffers(1, &m_vboPos);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vboPos = m_vboNrm = m_vboUv = m_ebo = 0;
    m_indexCount = 0;
}

void Mesh::upload(
    const std::vector<float>& positions,
    const std::vector<float>& normals,
    const std::vector<float>& uvs,
    const std::vector<unsigned int>& indices)
{
    release();

    if (positions.empty()) {
        std::cerr << "[Mesh] upload failed: empty positions\n";
        return;
    }

    const size_t vtx = positions.size() / 3;
    std::vector<float> n = normals;
    std::vector<float> t = uvs;
    if (n.size() < vtx * 3) {
        n.resize(vtx * 3, 0.0f);
        for (size_t i = 1; i < n.size(); i += 3)
            n[i] = 1.0f;
    }
    if (t.size() < vtx * 2)
        t.resize(vtx * 2, 0.0f);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vboPos);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboPos);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(float)),
        positions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glGenBuffers(1, &m_vboNrm);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboNrm);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(n.size() * sizeof(float)),
        n.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glGenBuffers(1, &m_vboUv);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboUv);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(t.size() * sizeof(float)),
        t.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    if (!indices.empty()) {
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(), GL_STATIC_DRAW);
        m_indexCount = static_cast<GLsizei>(indices.size());
        m_indexType = GL_UNSIGNED_INT;
        m_indexed = true;
    } else {
        m_indexCount = static_cast<GLsizei>(vtx);
        m_indexed = false;
    }

    glBindVertexArray(0);
}

void Mesh::draw() const
{
    if (m_vao == 0 || m_indexCount == 0)
        return;
    glBindVertexArray(m_vao);
    if (m_indexed)
        glDrawElements(GL_TRIANGLES, m_indexCount, m_indexType, nullptr);
    else
        glDrawArrays(GL_TRIANGLES, 0, m_indexCount);
    glBindVertexArray(0);
}
