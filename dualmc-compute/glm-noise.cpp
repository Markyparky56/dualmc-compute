#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm-noise.h"
#include "glm/vec2.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/vec_swizzle.hpp"
#include <cassert>

const unsigned char perm[512] = {
  151, 160, 137, 91, 90, 15,
  131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
  190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
  88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
  77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
  102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
  135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
  5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
  223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
  129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
  251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
  49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
  138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
  // Repeat
  151, 160, 137, 91, 90, 15,
  131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
  190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
  88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
  77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
  102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
  135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
  5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
  223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
  129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
  251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
  49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
  138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};
const unsigned char perm12_36[512] = {
    21,12,15,21,18, 9,33, 3,27,33, 0,15, 6,15,21,27,
    24, 0,21,18,27,30,24, 9, 3, 0,27,30,33,30,18,12,
    21, 0,18, 9, 0, 6,15, 6,30, 0, 9,33,27,33,33,24,
    27,27,27,12,27,15,24, 9,18,24,15,12, 9, 0,24,21,
     6,27,33, 6,21, 0, 9,30,15, 6, 6, 9,33, 9, 3, 6,
     0,21, 3, 6,12,27,24,15,21,30,15,12,12,18,33,18,
    15, 3, 9,15, 3, 0,24, 3,15,12, 0,21,12,15,18, 3,
    24,12, 9,30,24,24, 9, 6,24,12, 3,18,15,18, 9,12,
    12, 3,30,30,12, 9,15,30, 6, 9,30,18, 9,30, 3,24,
     9, 6,33,33,33,12,30,15, 6,27,12,18,21, 9, 6,27,
    33,24,24, 6,24,30,21,30,15,27,15,33,33,21,12,27,
    27,30, 9, 3,21, 6, 0, 6,21,15,24,12,30,15,12,24,
     6,18, 3, 0,33,30, 6, 3,30,18, 0, 0,33,33,18, 3,
    27, 9, 3,21,27, 6,33,33, 3, 0,30,21, 3,21,30, 3,
    12, 0, 0,24,21, 3, 6,27,21,12,18, 6,18,24, 3,27,
    18,18,21,15, 0, 0, 9,27,24, 9,18,18,33, 3, 0, 0, 
    // Repeat
    21,12,15,21,18, 9,33, 3,27,33, 0,15, 6,15,21,27,
    24, 0,21,18,27,30,24, 9, 3, 0,27,30,33,30,18,12,
    21, 0,18, 9, 0, 6,15, 6,30, 0, 9,33,27,33,33,24,
    27,27,27,12,27,15,24, 9,18,24,15,12, 9, 0,24,21,
     6,27,33, 6,21, 0, 9,30,15, 6, 6, 9,33, 9, 3, 6,
     0,21, 3, 6,12,27,24,15,21,30,15,12,12,18,33,18,
    15, 3, 9,15, 3, 0,24, 3,15,12, 0,21,12,15,18, 3,
    24,12, 9,30,24,24, 9, 6,24,12, 3,18,15,18, 9,12,
    12, 3,30,30,12, 9,15,30, 6, 9,30,18, 9,30, 3,24,
     9, 6,33,33,33,12,30,15, 6,27,12,18,21, 9, 6,27,
    33,24,24, 6,24,30,21,30,15,27,15,33,33,21,12,27,
    27,30, 9, 3,21, 6, 0, 6,21,15,24,12,30,15,12,24,
     6,18, 3, 0,33,30, 6, 3,30,18, 0, 0,33,33,18, 3,
    27, 9, 3,21,27, 6,33,33, 3, 0,30,21, 3,21,30, 3,
    12, 0, 0,24,21, 3, 6,27,21,12,18, 6,18,24, 3,27,
    18,18,21,15, 0, 0, 9,27,24, 9,18,18,33, 3, 0, 0,
};
const float Gradients3D[3 * 12] = {
    0.70710678118f, 0.70710678118f, 0.f,
    -0.70710678118f, 0.70710678118f, 0.f,
    0.70710678118f, -0.70710678118f, 0.f,
    -0.70710678118f, -0.70710678118f, 0.f,
    0.70710678118f, 0.f, 0.70710678118f,
    -0.70710678118f, 0.f, 0.70710678118f,
    0.70710678118f, 0.f, -0.70710678118f,
    -0.70710678118f, 0.f, -0.70710678118f,
    0.f, 0.70710678118f, 0.70710678118f,
    0.f, -0.70710678118f, 0.70710678118f,
    0.f, 0.70710678118f, -0.70710678118f,
    0.f, -0.70710678118f, -0.70710678118f
};

//static const float Gradients3D[3 * 12] = {
//    1.f, 1.f, 0.f,
//    -1.f, 1.f, 0.f,
//    1.f, -1.f, 0.f,
//    -1.f, -1.f, 0.f,
//    1.f, 0.f, 1.f,
//    -1.f, 0.f, 1.f,
//    1.f, 0.f, -1.f,
//    -1.f, 0.f, -1.f,
//    0.f, 1.f, 1.f,
//    0.f, -1.f, 1.f,
//    0.f, 1.f, -1.f,
//    0.f, -1.f, -1.f
//};

static inline int Index3D(glm::ivec3 p, int seed)
{
  assert((p.z & 0xff) + (seed & 0xff) < 512);
  assert(perm[(p.z & 0xff) + (seed & 0xff)] < 256);
  assert((p.y & 0xff) + perm[(p.z & 0xff) + (seed & 0xff)] < 512);
  assert(perm[(p.y & 0xff) + perm[(p.z & 0xff) + (seed & 0xff)]] < 256);
  assert((p.x & 0xff) + perm[(p.y & 0xff) + perm[(p.z & 0xff) + (seed & 0xff)]] < 512);
  assert(perm12_36[(p.x & 0xff) + perm[(p.y & 0xff) + perm[(p.z & 0xff) + (seed & 0xff)]]] < 36);

  return perm12_36[(p.x & 0xff) + perm[(p.y & 0xff) + perm[(p.z & 0xff) + (seed & 0xff)]]];
}

static inline float Grad3D(glm::ivec3 i, glm::vec3 p, int seed)
{
  const int gradIdx = Index3D(p, seed);  
  glm::vec3 grad = glm::vec3(Gradients3D[gradIdx], Gradients3D[gradIdx + 1], Gradients3D[gradIdx + 2]);
  return glm::dot(p, grad);
}

static inline float Grad3D(glm::ivec3 i, glm::vec3 p, int seed, glm::vec3& grad)
{
  const int gradIdx = Index3D(i, seed);
  grad = glm::vec3(Gradients3D[gradIdx], Gradients3D[gradIdx+1], Gradients3D[gradIdx+2]);
  return glm::dot(p, grad);
}

// Skew/Unskew factors
#define F3 (1.0f / 3.0f)
#define G3 (1.0f / 6.0f)

float SimplexNoise3D(const glm::vec3 p, int seed)
{
  const glm::vec3 Skew = glm::vec3(F3, G3, 0.5);
  // First corner, skewed integer coord
  glm::vec3 pI = glm::floor(p + glm::dot(p, glm::xxx(Skew))); // ijk
  glm::vec3 surf0 = p - pI + glm::dot(pI, glm::yyy(Skew));

  // Other corners
  glm::vec3 g = glm::step(glm::yzx(surf0), glm::xyz(surf0)); // equivalent to (x < y), (y < z), (z < x)
  glm::vec3 l = 1.f - g; // inverts?
  // Simplex offsets (TODO, map this out for future reference?)
  glm::vec3 iSimplex1 = glm::min(g, glm::zxy(l));
  glm::vec3 iSimplex2 = glm::max(g, glm::zxy(l));

  // Surflet coords
  glm::vec3 surf1 = surf0 - iSimplex1 + glm::yyy(Skew); // G3
  glm::vec3 surf2 = surf0 - iSimplex2 + glm::xxx(Skew); // 2*(1/6) == (1/3) == F3
  glm::vec3 surf3 = surf0 -			        glm::zzz(Skew); // -1 + 3*(1/6) == -0.5

  glm::vec4 t = glm::max(0.6f - glm::vec4(glm::dot(surf0, surf0), glm::dot(surf1, surf1), glm::dot(surf2, surf2), glm::dot(surf3, surf3)), 0.f);
  t *= t;
  glm::vec4 grads = glm::vec4(Grad3D(pI, surf0, seed), Grad3D(pI + iSimplex1, surf1, seed), Grad3D(pI + iSimplex2, surf2, seed), Grad3D(pI + glm::vec3(1), surf3, seed));

  // GLSL version uses 105 here?
  return 46.f * glm::dot(t*t, grads);
}

glm::vec4 SimplexNoise3DGrad(const glm::vec3 p, int seed)
{
  const glm::vec3 Skew = glm::vec3(F3, G3, 0.5);
  // First corner, skewed integer coord
  glm::vec3 pI = glm::floor(p + glm::dot(p, glm::xxx(Skew)));
  glm::vec3 surf0 = p - pI + glm::dot(pI, glm::yyy(Skew));

  // Other corners
  glm::vec3 g = glm::step(glm::yzx(surf0), glm::xyz(surf0)); // equivalent to (x < y), (y < z), (z < x)
  glm::vec3 l = 1.f - g; // inverts?
  // Simplex offsets (TODO, map this out for future reference?)
  glm::vec3 iSimplex1 = glm::min(g, glm::zxy(l));
  glm::vec3 iSimplex2 = glm::max(g, glm::zxy(l));

  // Surflet coords
  glm::vec3 surf1 = surf0 - iSimplex1 + glm::yyy(Skew); // G3
  glm::vec3 surf2 = surf0 - iSimplex2 + glm::xxx(Skew); // 2*(1/6) == (1/3) == F3
  glm::vec3 surf3 = surf0 -			        glm::zzz(Skew); // -1 + 3*(1/6) == -0.5

  glm::vec4 t = glm::max(0.6f - glm::vec4(glm::dot(surf0, surf0), glm::dot(surf1, surf1), glm::dot(surf2, surf2), glm::dot(surf3, surf3)), 0.f);
  glm::vec3 grad0, grad1, grad2, grad3;
  // pdotx
  glm::vec4 grads = glm::vec4(
      Grad3D(pI, surf0, seed, grad0)
    , Grad3D(pI + iSimplex1, surf1, seed, grad1)
    , Grad3D(pI + iSimplex2, surf2, seed, grad2)
    , Grad3D(pI + glm::vec3(1), surf3, seed, grad3)
  );

  glm::vec4 t2 = t * t;
  glm::vec4 t4 = t2*t2;

  glm::vec4 temp = t2 * t * grads;
  glm::vec3 deriv = -8.f * (temp.x * surf0 + temp.y * surf1 + temp.z * surf2 + temp.w * surf3);
  deriv += t4.x * grad0 + t4.y * grad1 + t4.z * grad2 + t4.w * grad3;

  // GLSL version uses 105 here?
  const float valueScalar = 46.f;
  const float derivScalar = 7.4f;

  // x = noise value
  // y,z,w = derivative
  return glm::vec4(
    valueScalar * glm::dot(t4, grads),
    derivScalar * deriv
  );
}
