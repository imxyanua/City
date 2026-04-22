#include "GLTFLoader.h"
#include "Mesh.h"
#include "Model.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

namespace gltf {
namespace {

bool readFloatVec3(const tinygltf::Model& model, int accessorIndex, std::vector<float>& out);

glm::mat4 getLocalTransform(const tinygltf::Node& n)
{
    if (n.matrix.size() == 16) {
        glm::mat4 M(1.0f);
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                M[c][r] = static_cast<float>(n.matrix[static_cast<size_t>(c * 4 + r)]);
            }
        }
        return M;
    }

    glm::vec3 T(0.0f);
    glm::vec3 S(1.0f);
    glm::quat Q(1.0f, 0.0f, 0.0f, 0.0f);

    if (n.translation.size() == 3) {
        T = glm::vec3(
            static_cast<float>(n.translation[0]),
            static_cast<float>(n.translation[1]),
            static_cast<float>(n.translation[2]));
    }
    if (n.scale.size() == 3) {
        S = glm::vec3(
            static_cast<float>(n.scale[0]),
            static_cast<float>(n.scale[1]),
            static_cast<float>(n.scale[2]));
    }
    if (n.rotation.size() == 4) {
        Q = glm::quat(
            static_cast<float>(n.rotation[3]),
            static_cast<float>(n.rotation[0]),
            static_cast<float>(n.rotation[1]),
            static_cast<float>(n.rotation[2]));
    }

    const glm::mat4 tr = glm::translate(glm::mat4(1.0f), T);
    const glm::mat4 rot = glm::mat4_cast(Q);
    const glm::mat4 sc = glm::scale(glm::mat4(1.0f), S);
    return tr * rot * sc;
}

void transformPositions(std::vector<float>& v, const glm::mat4& M)
{
    for (size_t i = 0; i + 2 < v.size(); i += 3) {
        const glm::vec4 p = M * glm::vec4(v[i], v[i + 1], v[i + 2], 1.0f);
        v[i] = p.x;
        v[i + 1] = p.y;
        v[i + 2] = p.z;
    }
}

void transformNormals(std::vector<float>& n, const glm::mat3& N)
{
    for (size_t i = 0; i + 2 < n.size(); i += 3) {
        glm::vec3 x = N * glm::vec3(n[i], n[i + 1], n[i + 2]);
        const float len = glm::length(x);
        if (len > 1e-8f)
            x /= len;
        n[i] = x.x;
        n[i + 1] = x.y;
        n[i + 2] = x.z;
    }
}

bool readFloatVec3(const tinygltf::Model& model, int accessorIndex, std::vector<float>& out)
{
    if (accessorIndex < 0)
        return false;
    const tinygltf::Accessor& acc = model.accessors[static_cast<size_t>(accessorIndex)];
    if (acc.type != TINYGLTF_TYPE_VEC3)
        return false;
    if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
        return false;

    const tinygltf::BufferView& view = model.bufferViews[static_cast<size_t>(acc.bufferView)];
    const tinygltf::Buffer& buf = model.buffers[static_cast<size_t>(view.buffer)];

    size_t stride = acc.ByteStride(view);
    if (stride == 0) {
        stride = sizeof(float) * 3;
    }

    const unsigned char* base = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize(acc.count * 3);
    for (size_t i = 0; i < acc.count; ++i) {
        const float* f = reinterpret_cast<const float*>(base + i * stride);
        out[i * 3 + 0] = f[0];
        out[i * 3 + 1] = f[1];
        out[i * 3 + 2] = f[2];
    }
    return true;
}

bool readFloatVec2(const tinygltf::Model& model, int accessorIndex, std::vector<float>& out)
{
    if (accessorIndex < 0)
        return false;
    const tinygltf::Accessor& acc = model.accessors[static_cast<size_t>(accessorIndex)];
    if (acc.type != TINYGLTF_TYPE_VEC2)
        return false;
    if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
        return false;

    const tinygltf::BufferView& view = model.bufferViews[static_cast<size_t>(acc.bufferView)];
    const tinygltf::Buffer& buf = model.buffers[static_cast<size_t>(view.buffer)];

    size_t stride = acc.ByteStride(view);
    if (stride == 0) {
        stride = sizeof(float) * 2;
    }

    const unsigned char* base = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize(acc.count * 2);
    for (size_t i = 0; i < acc.count; ++i) {
        const float* f = reinterpret_cast<const float*>(base + i * stride);
        out[i * 2 + 0] = f[0];
        out[i * 2 + 1] = f[1];
    }
    return true;
}

bool readIndices(const tinygltf::Model& model, int accessorIndex, std::vector<unsigned int>& out)
{
    if (accessorIndex < 0)
        return false;
    const tinygltf::Accessor& acc = model.accessors[static_cast<size_t>(accessorIndex)];
    if (acc.type != TINYGLTF_TYPE_SCALAR)
        return false;

    const tinygltf::BufferView& view = model.bufferViews[static_cast<size_t>(acc.bufferView)];
    const tinygltf::Buffer& buf = model.buffers[static_cast<size_t>(view.buffer)];

    size_t stride = acc.ByteStride(view);
    if (stride == 0) {
        stride = tinygltf::GetComponentSizeInBytes(acc.componentType);
    }

    const unsigned char* base = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize(acc.count);

    for (size_t i = 0; i < acc.count; ++i) {
        const unsigned char* p = base + i * stride;
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            std::uint32_t v = 0;
            std::memcpy(&v, p, sizeof(v));
            out[i] = v;
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            std::uint16_t v = 0;
            std::memcpy(&v, p, sizeof(v));
            out[i] = static_cast<unsigned int>(v);
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            out[i] = static_cast<unsigned int>(p[0]);
        } else {
            return false;
        }
    }
    return true;
}

GLuint createTexture2DFromImage(const tinygltf::Model& model, int imageIndex, std::string& warn)
{
    if (imageIndex < 0 || imageIndex >= static_cast<int>(model.images.size()))
        return 0;

    const tinygltf::Image& img = model.images[static_cast<size_t>(imageIndex)];
    if (img.width <= 0 || img.height <= 0 || img.image.empty()) {
        warn += "Image " + std::to_string(imageIndex) + " has no pixel data (embed or URI missing).\n";
        return 0;
    }

    const int w = img.width;
    const int h = img.height;
    const int c = img.component;

    GLenum format = GL_RGBA;
    GLenum internal = GL_SRGB8_ALPHA8;
    if (c == 3) {
        format = GL_RGB;
        internal = GL_SRGB8;
    } else if (c == 4) {
        format = GL_RGBA;
        internal = GL_SRGB8_ALPHA8;
    } else if (c == 1) {
        format = GL_RED;
        internal = GL_R8;
    } else {
        warn += "Unsupported channel count for image " + std::to_string(imageIndex) + ".\n";
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), w, h, 0, format, GL_UNSIGNED_BYTE,
        img.image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GpuMaterial buildMaterial(
    const tinygltf::Model& model,
    int materialIndex,
    std::vector<GLuint>& texByImage,
    std::string& warn)
{
    GpuMaterial g;
    g.baseColorFactor = glm::vec4(1.0f);
    g.useTexture = false;
    g.diffuseTexture = 0;

    if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
        return g;

    const tinygltf::Material& m = model.materials[static_cast<size_t>(materialIndex)];
    g.name = m.name;
    const auto& pbr = m.pbrMetallicRoughness;

    if (pbr.baseColorFactor.size() >= 4) {
        g.baseColorFactor = glm::vec4(
            static_cast<float>(pbr.baseColorFactor[0]),
            static_cast<float>(pbr.baseColorFactor[1]),
            static_cast<float>(pbr.baseColorFactor[2]),
            static_cast<float>(pbr.baseColorFactor[3]));
    }

    if (pbr.baseColorTexture.index >= 0) {
        const int texIdx = pbr.baseColorTexture.index;
        if (texIdx >= 0 && texIdx < static_cast<int>(model.textures.size())) {
            const int src = model.textures[static_cast<size_t>(texIdx)].source;
            if (src >= 0 && src < static_cast<int>(texByImage.size())) {
                if (texByImage[static_cast<size_t>(src)] == 0) {
                    texByImage[static_cast<size_t>(src)] = createTexture2DFromImage(model, src, warn);
                }
                g.diffuseTexture = texByImage[static_cast<size_t>(src)];
                g.useTexture = g.diffuseTexture != 0;
            }
        }
    }

    return g;
}

std::string makeMeshObjectName(
    const tinygltf::Node& node,
    const tinygltf::Mesh& tm,
    int nodeIndex,
    size_t primIndex)
{
    std::string base;
    if (!node.name.empty()) {
        base = node.name;
    } else if (!tm.name.empty()) {
        base = tm.name;
    } else {
        base = "gltf_node_" + std::to_string(nodeIndex);
    }
    if (tm.primitives.size() > 1) {
        base += "_prim" + std::to_string(primIndex);
    }
    return base;
}

void visitNode(
    const tinygltf::Model& model,
    int nodeIndex,
    const glm::mat4& parent,
    std::vector<Mesh>& meshesOut,
    std::vector<GLuint>& texByImage,
    std::string& warn)
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
        return;

    const tinygltf::Node& node = model.nodes[static_cast<size_t>(nodeIndex)];
    const glm::mat4 world = parent * getLocalTransform(node);

    if (node.mesh >= 0 && node.mesh < static_cast<int>(model.meshes.size())) {
        const tinygltf::Mesh& tm = model.meshes[static_cast<size_t>(node.mesh)];
        const glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(world)));

        for (size_t primI = 0; primI < tm.primitives.size(); ++primI) {
            const auto& prim = tm.primitives[primI];
            if (prim.mode != TINYGLTF_MODE_TRIANGLES && prim.mode != -1) {
                warn += "Skipping non-triangle primitive.\n";
                continue;
            }

            std::vector<float> pos, nrm, uv;
            auto posIt = prim.attributes.find("POSITION");
            if (posIt == prim.attributes.end()) {
                warn += "Primitive missing POSITION.\n";
                continue;
            }
            if (!readFloatVec3(model, posIt->second, pos)) {
                warn += "Failed to read POSITION accessor.\n";
                continue;
            }

            auto nIt = prim.attributes.find("NORMAL");
            if (nIt != prim.attributes.end()) {
                readFloatVec3(model, nIt->second, nrm);
            }

            auto uvIt = prim.attributes.find("TEXCOORD_0");
            if (uvIt != prim.attributes.end()) {
                readFloatVec2(model, uvIt->second, uv);
            }

            transformPositions(pos, world);
            if (!nrm.empty()) {
                transformNormals(nrm, normalMat);
            }

            std::vector<unsigned int> indices;
            if (prim.indices >= 0) {
                if (!readIndices(model, prim.indices, indices)) {
                    warn += "Failed to read index buffer.\n";
                    continue;
                }
            } else {
                const size_t nVtx = pos.size() / 3;
                indices.resize(nVtx);
                for (size_t i = 0; i < nVtx; ++i)
                    indices[static_cast<size_t>(i)] = static_cast<unsigned int>(i);
            }

            Mesh mesh;
            mesh.setObjectName(makeMeshObjectName(node, tm, nodeIndex, primI));
            mesh.setMaterialIndex(prim.material);
            mesh.upload(pos, nrm, uv, indices);
            meshesOut.push_back(std::move(mesh));
        }
    }

    for (int child : node.children) {
        visitNode(model, child, world, meshesOut, texByImage, warn);
    }
}

} // namespace

bool loadGLB(const std::string& path, Model& model, std::string& errorMessage)
{
    model.clear();

    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;
    std::string err;
    std::string warn;

    const bool ok = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
    if (!warn.empty()) {
        std::cerr << "[tinygltf] Warning: " << warn << "\n";
    }
    if (!ok) {
        errorMessage = "Failed to load GLB: " + err;
        model.clear();
        return false;
    }

    if (gltfModel.scenes.empty()) {
        errorMessage = "glTF file has no scenes.";
        model.clear();
        return false;
    }

    std::vector<GLuint> texByImage(gltfModel.images.size(), 0);

    model.m_materials.reserve(gltfModel.materials.size() + 1);
    model.m_materials.push_back(GpuMaterial{});

    for (size_t i = 0; i < gltfModel.materials.size(); ++i) {
        std::string tw;
        model.m_materials.push_back(buildMaterial(gltfModel, static_cast<int>(i), texByImage, tw));
        if (!tw.empty()) {
            std::cerr << "[GLTFLoader] Material " << i << ": " << tw;
        }
    }

    int sceneIndex = gltfModel.defaultScene;
    if (sceneIndex < 0) {
        sceneIndex = 0;
    }

    const tinygltf::Scene& scene = gltfModel.scenes[static_cast<size_t>(sceneIndex)];
    const glm::mat4 identity(1.0f);
    std::string localWarn;
    for (int root : scene.nodes) {
        visitNode(gltfModel, root, identity, model.m_meshes, texByImage, localWarn);
    }
    if (!localWarn.empty()) {
        std::cerr << "[GLTFLoader] " << localWarn;
    }

    std::cerr << "[GLTFLoader] " << model.meshes().size() << " mesh loaded.\n";

    if (model.m_meshes.empty()) {
        errorMessage = "No triangle meshes were built from GLB (check node/mesh structure).";
        model.clear();
        return false;
    }

    const int matCount = static_cast<int>(gltfModel.materials.size());
    for (Mesh& mesh : model.m_meshes) {
        int mid = mesh.materialIndex();
        if (mid < 0 || mid >= matCount) {
            mesh.setMaterialIndex(0);
        } else {
            mesh.setMaterialIndex(mid + 1);
        }
    }

    return true;
}

} // namespace gltf
