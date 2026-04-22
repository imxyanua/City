#pragma once

#include <string>

class Model;

namespace gltf {

// Parses .glb via tinygltf, uploads geometry and textures (mipmaps, linear filtering).
bool loadGLB(const std::string& path, Model& model, std::string& errorMessage);

} // namespace gltf
