#pragma once
#include <functional>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/geometric.hpp"

float SimplexNoise3D(const glm::vec3 p, int seed);
glm::vec4 SimplexNoise3DGrad(const glm::vec3 p, int seed);

// Courtesy of FastNoise <3
constexpr inline float CalcFractalBounding(int octaves, float gain)
{
  float amp = gain;
  float ampFractal = 1.0f;
  for (int i = 1; i < octaves; i++)
  {
    ampFractal += amp;
    amp *= gain;
  }
  return 1.0f / ampFractal;
}

template<typename T, typename Func>
T NoiseFBM(Func func, glm::vec3 p, int seed, int octaves, float lacunarity, float gain)
{
  T sum = std::invoke(func, p, seed);
  float amp = 1.f;
  for (int i = 1; i < octaves; ++i)
  {
    p *= lacunarity;
    amp *= gain;
    sum += std::invoke(func, p, seed+i) * amp;
  }

  return sum * CalcFractalBounding(octaves, gain);
}

inline float NoiseFBM(glm::vec3 p, int seed, int octaves, float lacunarity, float gain)
{
  return NoiseFBM<float>(&SimplexNoise3D, p, seed, octaves, lacunarity, gain);
}

inline glm::vec4 NoiseFBMGrads(glm::vec3 p, int seed, int octaves, float lacunarity, float gain)
{
  return NoiseFBM<glm::vec4>(&SimplexNoise3DGrad, p, seed, octaves, lacunarity, gain);
}

template<typename T, typename Func>
T NoiseBillow(Func func, glm::vec3 p, int seed, int octaves, float lacunarity, float gain)
{
  T sum = glm::abs(std::invoke(func, p, seed)) * 2.f - 1.f;
  float amp = 1;
  for (int i = 1; i < octaves; ++i)
  {
    p *= lacunarity;
    amp *= gain;
    sum += (glm::abs(std::invoke(func, p, seed + i)) * 2.f - 1.f) * amp;
  }
  return sum * CalcFractalBounding(octaves, gain);
}

inline float NoiseBillow(glm::vec3 p, int seed, int octaves, float lacuranirty, float gain)
{
  return NoiseBillow<float>(&SimplexNoise3D, p, seed, octaves, lacuranirty, gain);
}

inline glm::vec4 NoiseBillowGrads(glm::vec3 p, int seed, int octaves, float lacuranity, float gain)
{
  return NoiseBillow<glm::vec4>(&SimplexNoise3DGrad, p, seed, octaves, lacuranity, gain);
}
