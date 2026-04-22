#pragma once

#include <glm/glm.hpp>

#include <vector>

// Builds a grid / street layout of instance transforms (model loaded once, drawn many times).
class CityBuilder {
public:
    // spacingZ: distance along the street (negative Z forward). spacingX: lateral offset between columns.
    // rows x cols grid, optional rotation (degrees) on Y for variety.
    static std::vector<glm::mat4> buildGrid(
        int rows,
        int cols,
        float spacingX,
        float spacingZ,
        float yRotationDegrees = 0.0f);
};
