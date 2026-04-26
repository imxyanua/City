#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

// Minimal shader wrapper: compile/link with error logging (console).
class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const;

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& v) const;
    void setVec3(const std::string& name, const glm::vec3& v) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setMat3(const std::string& name, const glm::mat3& m) const;
    void setMat4(const std::string& name, const glm::mat4& m) const;

    GLuint programId() const { return m_program; }

private:
    GLuint m_program = 0;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;

    static bool compileStage(GLenum type, const char* src, GLuint& outShader, std::string& log);
    static bool linkProgram(GLuint vert, GLuint frag, GLuint& outProgram, std::string& log);

    GLint getUniformLocation(const std::string& name) const;
};
