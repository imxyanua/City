#include "CityBuilder.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

std::vector<glm::mat4> CityBuilder::buildGrid(
    int rows,
    int cols,
    float spacingX,
    float spacingZ,
    float yRotationDegrees)
{
    std::vector<glm::mat4> out;
    out.reserve(static_cast<size_t>(rows * cols));

    const float ox = -0.5f * static_cast<float>(cols - 1) * spacingX;
    const float oz = 0.0f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            glm::mat4 M(1.0f);
            M = glm::translate(M, glm::vec3(ox + static_cast<float>(c) * spacingX, 0.0f, oz - static_cast<float>(r) * spacingZ));
            if (std::abs(yRotationDegrees) > 1e-5f) {
                M = glm::rotate(M, glm::radians(yRotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
            }
            out.push_back(M);
        }
    }
    return out;
}
