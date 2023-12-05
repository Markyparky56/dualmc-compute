#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

float SimplexNoise3D(const glm::vec3 p);
glm::vec4 SimplexNoise3DGrad(const glm::vec3 p);
