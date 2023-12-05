#pragma once
// SVD/QEF implementation for claculating feature points from hermite data based on glsl_svd.cpp from nickgildea/paniq

#include "glm/vec4.hpp"
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"

void qefAdd(const glm::vec3& n, const glm::vec3& p, glm::mat3& ATA, glm::vec3& ATb, glm::vec4& pointAccumulate);
float qefSolve(const glm::mat3& ATA, glm::vec3 ATb, const glm::vec4& pointAccumulate, glm::vec3& x);
