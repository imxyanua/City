#include "Shader.h"

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace {

std::string readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

Shader::~Shader()
{
    if (m_program != 0)
        glDeleteProgram(m_program);
}

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program)
    , m_uniformCache(std::move(other.m_uniformCache))
{
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        if (m_program != 0)
            glDeleteProgram(m_program);
        m_program = other.m_program;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_program = 0;
    }
    return *this;
}

bool Shader::compileStage(GLenum type, const char* src, GLuint& outShader, std::string& log)
{
    outShader = glCreateShader(type);
    glShaderSource(outShader, 1, &src, nullptr);
    glCompileShader(outShader);

    GLint ok = 0;
    glGetShaderiv(outShader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(outShader, GL_INFO_LOG_LENGTH, &len);
        log.resize(static_cast<size_t>(len));
        glGetShaderInfoLog(outShader, len, nullptr, log.data());
        glDeleteShader(outShader);
        outShader = 0;
        return false;
    }
    return true;
}

bool Shader::linkProgram(GLuint vert, GLuint frag, GLuint& outProgram, std::string& log)
{
    outProgram = glCreateProgram();
    glAttachShader(outProgram, vert);
    glAttachShader(outProgram, frag);
    glLinkProgram(outProgram);

    GLint ok = 0;
    glGetProgramiv(outProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(outProgram, GL_INFO_LOG_LENGTH, &len);
        log.resize(static_cast<size_t>(len));
        glGetProgramInfoLog(outProgram, len, nullptr, log.data());
        glDeleteProgram(outProgram);
        outProgram = 0;
        return false;
    }
    return true;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vsrc;
    std::string fsrc;
    try {
        vsrc = readFile(vertexPath);
        fsrc = readFile(fragmentPath);
    } catch (const std::exception& e) {
        std::cerr << "[Shader] " << e.what() << "\n";
        return false;
    }

    GLuint vs = 0, fs = 0;
    std::string log;

    if (!compileStage(GL_VERTEX_SHADER, vsrc.c_str(), vs, log)) {
        std::cerr << "[Shader] Vertex compile failed:\n" << log << "\n";
        return false;
    }
    if (!compileStage(GL_FRAGMENT_SHADER, fsrc.c_str(), fs, log)) {
        std::cerr << "[Shader] Fragment compile failed:\n" << log << "\n";
        glDeleteShader(vs);
        return false;
    }

    GLuint prog = 0;
    if (!linkProgram(vs, fs, prog, log)) {
        std::cerr << "[Shader] Link failed:\n" << log << "\n";
        glDeleteShader(vs);
        glDeleteShader(fs);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (m_program != 0)
        glDeleteProgram(m_program);
    m_program = prog;
    m_uniformCache.clear();
    return true;
}

void Shader::use() const
{
    glUseProgram(m_program);
}

GLint Shader::getUniformLocation(const std::string& name) const
{
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end())
        return it->second;
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    m_uniformCache[name] = loc;
    if (loc < 0) {
        std::cerr << "[Shader] Warning: uniform not found or inactive: " << name << "\n";
    }
    return loc;
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(getUniformLocation(name), value ? 1 : 0);
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& v) const
{
    glUniform2fv(getUniformLocation(name), 1, &v[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& v) const
{
    glUniform3fv(getUniformLocation(name), 1, &v[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) const
{
    glUniform4fv(getUniformLocation(name), 1, &v[0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& m) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &m[0][0]);
}
