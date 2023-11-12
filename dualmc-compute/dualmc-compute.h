#pragma once
#include <array>
#include <tuple>
#include "glm/vec3.hpp"
#include "dualmc-dualpoints.h"

// 36 -2 (36 for a 32-cube with + 2 skirt)
// Reason to -2:
// 35 cubes (36-1)
// 34 iterations since we look at neighbour +1 along each axis
#define REDUCED_X 34
#define REDUCED_Y 34
#define REDUCED_Z 34

struct Triangle
{
  glm::vec3 Vert0;
  glm::vec3 Vert1;
  glm::vec3 Vert2;
  glm::vec3 Normal;
};

class DualMCComputeFramework
{
public:
  DualMCComputeFramework();

  glm::uint getProblematicConfigByteValue(int byteIndex);
  glm::uint getDualPointListEntry(int cellCode, int index);
  glm::uint morton3D(glm::ivec3 v);
  glm::uint getData(glm::uint shortIndex);
  int getCellCode(glm::ivec3 cellPos, float iso);
  glm::uint getDualPointCode(glm::ivec3 cellPos, float iso, int edgeCode);
  glm::vec3 calculateDualPoint(glm::ivec3 cellPos, float iso, glm::uint pointCode);
  glm::vec3 calculateFaceNormal(glm::vec3 vert0, glm::vec3 vert1, glm::vec3 vert2);
  
  void execute();

  // 64*64*64 is the maximum possible output from a 6-bit encoded morton code
  // TODO: work out the maximum output when we restrict the ranges to 36x36x36
  static const size_t VolumeDataSize = (64 * 64 * 64)+1;
  void SetData(const std::array<glm::uint, VolumeDataSize>& inData);

  static const size_t MaxTriangleCount = 34*34*34*18;
  std::tuple<const std::array<Triangle, MaxTriangleCount>&, glm::uint> GetTriangles() const
  {
    return { TriangleOutput.Triangles, TriangleCounter };
  }

protected:
  void exec_single(glm::uvec3 invocationID);

  void PackDualPointsList();
  void PackProblematicConfigs();

  // 6-bit morton encoding bumps us up to 63, even though we only use 36 in each dimension
  struct VolumeDataBuffer
  {
    //glm::uint Data[36*36*36];
    //std::array<glm::uint, 36 * 36 * 36> Data;
    std::array<glm::uint, VolumeDataSize> Data;
  } VolumeData;

  struct DualPointsListBuffer
  {
    /*glm::uint DualPointsList[512];
    glm::uint ProblematicConfigs[64];*/
    std::array<glm::uint, 512> DualPointsList;
    std::array<glm::uint, 64> ProblematicConfigs;
  } DualPoints;

  glm::uint TriangleCounter;

  struct TriangleBuffer
  {
    // Worst case, every cell produces 18 triangles... somehow
    // Don't think this is possible
    //Triangle Triangles[34 * 34 * 34 * 18];
    std::array<Triangle, 34*34*34*18> Triangles;
  } TriangleOutput;
};
