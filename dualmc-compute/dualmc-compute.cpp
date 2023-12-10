#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "dualmc-compute.h"
#include "glm/geometric.hpp"

DualMCComputeFramework::DualMCComputeFramework()
  : TriangleCounter(0)
  , CachedPointCounter(0)
{
  PackDualPointsList();
  PackProblematicConfigs();

  DualPointMap.resize(64 * 64 * 64, ~0ull); // 2^18, space for every possible morton id
  PrecalculatedDualPoints.resize(34 * 34 * 34); // space for every cell's dual point
}

glm::uint DualMCComputeFramework::getProblematicConfigByteValue(int byteIndex)
{
  // Determine the index of the uint in the array
  int uintIndex = byteIndex / 4;

  // Calculate the bit offset within the uint
  int bitOffset = (byteIndex % 4)*8;

  // Shift to the starting bit position within the uint and mask out the byte
  return (DualPoints.ProblematicConfigs[uintIndex] >> bitOffset) & 0xFFu;
}

glm::uint DualMCComputeFramework::getDualPointListEntry(int cellCode, int index)
{
  // Two shorts to a uint
  const glm::uint idx = (cellCode * 2) + (index / 2);
  const glm::uint packedShorts = DualPoints.DualPointsList[idx];

  // Extract the short
  const glm::uint shiftAmount = (index % 2) * 16;
  return (packedShorts >> shiftAmount) & 0xFFFFu;
}

glm::uint DualMCComputeFramework::morton3D(glm::ivec3 v)
{
  v &= 63; // Ensure input components don't go above 63
  // Spread coordinate bits
  v = (v | (v << 8)) & 0x300F;
  v = (v | (v << 4)) & 0x30C3;
  v = (v | (v << 2)) & 0x9249;
  // Interleave
  const glm::uint code = (v.x) | ((v.y) << 1) | ((v.z) << 2);
  assert(code < VolumeDataSize);

  return code;
}

glm::uint DualMCComputeFramework::getData(glm::uint mortonCode)
{
  // 2-shorts to a uint
  const glm::uint dataIdx = mortonCode / 2;
  // Select which short we're fetching
  const glm::uint shortIdx = mortonCode % 2;
  // If second short, we need to shift
  const glm::uint offset = 16 * shortIdx;

  // Retrieve data and mask out the short
  return (VolumeData.Data[dataIdx] >> offset) & 0xFFFFu;
}

int DualMCComputeFramework::getCellCode(glm::ivec3 cellPos, float iso)
{
  int code = 0;

  code |= int(getData(morton3D(cellPos)) >= iso);
  code |= int(getData(morton3D(cellPos + glm::ivec3(1, 0, 0))) >= iso) << 1;
  code |= int(getData(morton3D(cellPos + glm::ivec3(0, 1, 0))) >= iso) << 2;
  code |= int(getData(morton3D(cellPos + glm::ivec3(1, 1, 0))) >= iso) << 3;
  code |= int(getData(morton3D(cellPos + glm::ivec3(0, 0, 1))) >= iso) << 4;
  code |= int(getData(morton3D(cellPos + glm::ivec3(1, 0, 1))) >= iso) << 5;
  code |= int(getData(morton3D(cellPos + glm::ivec3(0, 1, 1))) >= iso) << 6;
  code |= int(getData(morton3D(cellPos + glm::ivec3(1, 1, 1))) >= iso) << 7;

  return code;
}

glm::uint DualMCComputeFramework::getDualPointCode(glm::ivec3 cellPos, float iso, int edgeCode)
{
  int cellCode = getCellCode(cellPos, iso);

  // Manifold Dual Marching Cubes approach from Rephael Wenger
  // If a problematic C16 or C19 configuration shares the ambiguous face with
  // another C16 or C19 configuration we invert the cube code before looking up
  // dual points. Doing this for these pairs ensures manifold meshes
  // This remove dualism to marching cubes. A small price to pay for a manifold mesh.

  // Check if we have a potentially problematic configuration
  const glm::uint direction = getProblematicConfigByteValue(cellCode);
  // If the direction code is not 255 we have a C16 or C19 configuration
  if (direction != 255)
  {
    // Copy curreny cell coords so we can work out neighbour coords
    glm::ivec3 neighbourCoords = cellPos;

    // We have to check the neighbouring cell, which shares the ambiguous face
    // For this we decode the direction. This could be done with another lookup table

    // Get the dimension of the non-zero coordinate axis
    const glm::uint component = direction >> 1;
    // Get the sign of the direction
    const int delta = (direction & 1) != 0 ? 1 : -1;

    // We test if we have left the volume in this direction
    bool leftVolume = false;

    if (component == 0) // X
    {
      neighbourCoords.x += delta;
      leftVolume = neighbourCoords.x >= 0 && neighbourCoords.x < (REDUCED_X - 1);
    }
    else if (component == 1) // Y
    {
      neighbourCoords.y += delta;
      leftVolume = neighbourCoords.y >= 0 && neighbourCoords.y < (REDUCED_Y - 1);
    }
    else /* component == 2 */ // Z
    {
      neighbourCoords.z += delta;
      leftVolume = neighbourCoords.z >= 0 && neighbourCoords.z < (REDUCED_Z - 1);
    }

    // If we have left the volume...
    if (leftVolume)
    {
      // Get the cell code of the neighbour
      const int neighbourCellCode = getCellCode(neighbourCoords, iso);
      // Check if they also have a problematic configuration
      if (getProblematicConfigByteValue(neighbourCellCode) != 255)
      {
        // Replace cube configuration with its inverse
        cellCode ^= 0xFF;
      }
    }
  }

  // Now look up the code for this cell
  for (int i = 0; i < 4; ++i)
  {
    const glm::uint entry = getDualPointListEntry(cellCode, i);
    if ((entry & edgeCode) != 0)
    {
      return entry;
    }
  }
  return 0;
}

glm::vec3 DualMCComputeFramework::calculateDualPoint(glm::ivec3 cellPos, float iso, glm::uint pointCode)
{
  glm::uint cellCode = morton3D(cellPos);
  glm::vec3 outPos;
  if (FindDualPoint(cellCode, outPos))
  {
    return outPos;
  }

  // Initialise the point with lower voxel coordinates
  outPos = glm::vec3(cellPos);
  
  // Return here to generate a cubed surface, the rest of this function moves the cell's feature point relative
  // to the surrounding cells
  //return outPos;

  // Compute the dual point as the mean of the face vertices belonging to the original marching cubes face
  glm::vec3 p = glm::vec3(0.0);
  int points = 0;

  const float dataCellPos = float(getData(morton3D(cellPos)));
  const float dataCellPosX1 = float(getData(morton3D(cellPos + glm::ivec3(1, 0, 0))));
  const float dataCellPosY1 = float(getData(morton3D(cellPos + glm::ivec3(0, 1, 0))));
  const float dataCellPosZ1 = float(getData(morton3D(cellPos + glm::ivec3(0, 0, 1))));
  const float dataCellPosX1Y1 = float(getData(morton3D(cellPos + glm::ivec3(1, 1, 0))));
  const float dataCellPosX1Z1 = float(getData(morton3D(cellPos + glm::ivec3(1, 0, 1))));
  const float dataCellPosY1Z1 = float(getData(morton3D(cellPos + glm::ivec3(0, 1, 1))));
  const float dataCellPosX1Y1Z1 = float(getData(morton3D(cellPos + glm::ivec3(1, 1, 1))));

  // Sum edge intersection vertices using the point code
  if ((pointCode & EDGE0) != 0)
  {
    p.x += (iso - dataCellPos) / (dataCellPosX1 - dataCellPos);
    points += 1;
  }
  if ((pointCode & EDGE1) != 0)
  {
    p.x += 1.0;
    p.z += (iso - dataCellPosX1) / (dataCellPosX1Z1 - dataCellPosX1);
    points += 1;
  }
  if ((pointCode & EDGE2) != 0)
  {
    p.x += (iso - dataCellPosZ1) / (dataCellPosX1Z1 - dataCellPosZ1);
    p.z += 1.0;
    points += 1;
  }
  if ((pointCode & EDGE3) != 0)
  {
    p.z += (iso - dataCellPos) / (dataCellPosZ1 - dataCellPos);
    points += 1;
  }
  if ((pointCode & EDGE4) != 0)
  {
    p.x += (iso - dataCellPosY1) / (dataCellPosX1Y1 - dataCellPosY1);
    p.y += 1.0;
    points += 1;
  }
  if ((pointCode & EDGE5) != 0)
  {
    p.x += 1.0;
    p.y += 1.0;
    p.z += (iso - dataCellPosX1Y1) / (dataCellPosX1Y1Z1 - dataCellPosX1Y1);
    points += 1;
  }
  if ((pointCode & EDGE6) != 0)
  {
    p.x += (iso - dataCellPosY1Z1) / (dataCellPosX1Y1Z1 - dataCellPosY1Z1);
    p.y += 1.0;
    p.z += 1.0;
    points += 1;
  }
  if ((pointCode & EDGE7) != 0)
  {
    p.y += 1.0;
    p.z += (iso - dataCellPosY1) / (dataCellPosY1Z1 - dataCellPosY1);
    points += 1;
  }
  if ((pointCode & EDGE8) != 0)
  {
    p.y += (iso - dataCellPos) / (dataCellPosY1 - dataCellPos);
    points += 1;
  }
  if ((pointCode & EDGE9) != 0)
  {
    p.x += 1.0;
    p.y += (iso - dataCellPosX1) / (dataCellPosX1Y1 - dataCellPosX1);
    points += 1;
  }
  if ((pointCode & EDGE10) != 0)
  {
    p.x += 1.0;
    p.y += (iso - dataCellPosX1Z1) / (dataCellPosX1Y1Z1 - dataCellPosX1Z1);
    p.z += 1.0;
    points += 1;
  }
  if ((pointCode & EDGE11) != 0)
  {
    p.y += (iso - dataCellPosZ1) / (dataCellPosY1Z1 - dataCellPosZ1);
    p.z += 1.0;
    points += 1;
  }

  // Divide by number of accumulated points
  p /= glm::vec3((float)points);
  // Offset point by voxel coordinates
  outPos += p;

  InsertNewDualPoint(morton3D(cellPos), outPos);

  return outPos;
}

glm::vec3 DualMCComputeFramework::calculateFaceNormal(glm::vec3 vert0, glm::vec3 vert1, glm::vec3 vert2)
{
  const glm::vec3 p01 = vert1 - vert0;
  const glm::vec3 p02 = vert2 - vert0;
  return -glm::vec3(glm::normalize(glm::cross(p01, p02)));
}

void DualMCComputeFramework::execute()
{
  for (glm::uint z = 0; z < 36; ++z)
  {
    for (glm::uint y = 0; y < 36; ++y)
    {
      for (glm::uint x = 0; x < 36; ++x)
      {
        exec_single(glm::uvec3(x, y, z));
      }
    }
  }
}

int DualMCComputeFramework::getNumDualPointCodes() const
{
  return CachedPointCounter;
}

void DualMCComputeFramework::SetData(const std::array<glm::uint, VolumeDataSize>& inData)
{
  VolumeData.Data = inData;
}

void DualMCComputeFramework::exec_single(glm::uvec3 invocationID)
{
  const float iso = 0.5*(float)USHRT_MAX;
  const glm::ivec3 xyz = glm::ivec3(invocationID);
  // early-out for x,y,z >= REDUCED_X,Y,Z
  if (xyz.x >= REDUCED_X || xyz.y >= REDUCED_Y || xyz.z >= REDUCED_Z)
    return;

  const glm::ivec3 xyzX1 = xyz - glm::ivec3(1, 0, 0);
  const glm::ivec3 xyzY1 = xyz - glm::ivec3(0, 1, 0);
  const glm::ivec3 xyzZ1 = xyz - glm::ivec3(0, 0, 1);
  const glm::ivec3 xyzX1Y1 = xyz - glm::ivec3(1, 1, 0);
  const glm::ivec3 xyzX1Z1 = xyz - glm::ivec3(1, 0, 1);
  const glm::ivec3 xyzY1Z1 = xyz - glm::ivec3(0, 1, 1);

  const glm::uint dataCellPos = getData(morton3D(xyz));

  // construct quad for x edge
  if (xyz.z > 0 && xyz.y > 0)
  {
    // Is edge intersected?
    const glm::uint dataCellPosX1 = getData(morton3D(xyz + glm::ivec3(1, 0, 0)));
    const bool entering = dataCellPos < iso && dataCellPosX1 >= iso;
    const bool exiting = dataCellPos >= iso && dataCellPosX1 < iso;

    if (entering || exiting)
    {
      // Generate quad
      const glm::vec3 vert0 = calculateDualPoint(xyz, iso, getDualPointCode(xyz, iso, EDGE0));
      const glm::vec3 vert1 = calculateDualPoint(xyzZ1, iso, getDualPointCode(xyzZ1, iso, EDGE2));
      const glm::vec3 vert2 = calculateDualPoint(xyzY1Z1, iso, getDualPointCode(xyzY1Z1, iso, EDGE6));
      const glm::vec3 vert3 = calculateDualPoint(xyzY1, iso, getDualPointCode(xyzY1, iso, EDGE4));

      // Reserve the next 2 triangle indices
      // atomicAdd returns the value before addition
      const glm::uint startIdx = TriangleCounter; TriangleCounter += 2u;

      if (entering)
      {
        // Tri 1 face normal
        const glm::vec3 faceNorm1 = calculateFaceNormal(vert0, vert1, vert2);
        // Tri 2 face normal
        const glm::vec3 faceNorm2 = calculateFaceNormal(vert2, vert3, vert0);

        // Tri 1, v0, v1, v2
        TriangleOutput.Triangles[startIdx].Vert0 = vert0;
        TriangleOutput.Triangles[startIdx].Vert1 = vert1;
        TriangleOutput.Triangles[startIdx].Vert2 = vert2;
        TriangleOutput.Triangles[startIdx].Normal = faceNorm1;

        // Tri 2, v2, v3, v0
        TriangleOutput.Triangles[startIdx + 1].Vert0 = vert2;
        TriangleOutput.Triangles[startIdx + 1].Vert1 = vert3;
        TriangleOutput.Triangles[startIdx + 1].Vert2 = vert0;
        TriangleOutput.Triangles[startIdx + 1].Normal = faceNorm2;
      }
      else // exiting
      {
        // Tri 1 face normal
        const glm::vec3 faceNorm1 = calculateFaceNormal(vert2, vert1, vert0);
        // Tri 2 face normal
        const glm::vec3 faceNorm2 = calculateFaceNormal(vert0, vert3, vert2);

        // Tri 1, v2, v1, v0
        TriangleOutput.Triangles[startIdx].Vert0 = vert2;
        TriangleOutput.Triangles[startIdx].Vert1 = vert1;
        TriangleOutput.Triangles[startIdx].Vert2 = vert0;
        TriangleOutput.Triangles[startIdx].Normal = faceNorm1;

        // Tri 2, v0, v3, v2
        TriangleOutput.Triangles[startIdx + 1].Vert0 = vert0;
        TriangleOutput.Triangles[startIdx + 1].Vert1 = vert3;
        TriangleOutput.Triangles[startIdx + 1].Vert2 = vert2;
        TriangleOutput.Triangles[startIdx + 1].Normal = faceNorm2;
      }
    }
  }

  // Construct quad for y edge
  if (xyz.z > 0 && xyz.x > 0)
  {
    // Is edge intersected?
    const glm::uint dataCellPosY1 = getData(morton3D(xyz + glm::ivec3(0, 1, 0)));
    const bool entering = dataCellPos < iso && dataCellPosY1 >= iso;
    const bool exiting = dataCellPos >= iso && dataCellPosY1 < iso;

    if (entering || exiting)
    {
      // Generate quad
      const glm::vec3 vert0 = calculateDualPoint(xyz, iso, getDualPointCode(xyz, iso, EDGE8));
      const glm::vec3 vert1 = calculateDualPoint(xyzZ1, iso, getDualPointCode(xyzZ1, iso, EDGE11));
      const glm::vec3 vert2 = calculateDualPoint(xyzX1Z1, iso, getDualPointCode(xyzX1Z1, iso, EDGE10));
      const glm::vec3 vert3 = calculateDualPoint(xyzX1, iso, getDualPointCode(xyzX1, iso, EDGE9));

      // Reserve the next 2 triangle indices
      // atomicAdd returns the value before addition
      const glm::uint startIdx = TriangleCounter; TriangleCounter += 2u;

      if (exiting)
      {
        // Tri 1 face normal
        const glm::vec3 faceNorm1 = calculateFaceNormal(vert0, vert1, vert2);
        // Tri 2 face normal
        const glm::vec3 faceNorm2 = calculateFaceNormal(vert2, vert3, vert0);

        // Tri 1, v0, v1, v2
        TriangleOutput.Triangles[startIdx].Vert0 = vert0;
        TriangleOutput.Triangles[startIdx].Vert1 = vert1;
        TriangleOutput.Triangles[startIdx].Vert2 = vert2;
        TriangleOutput.Triangles[startIdx].Normal = faceNorm1;

        // Tri 2, v2, v3, v0
        TriangleOutput.Triangles[startIdx + 1].Vert0 = vert2;
        TriangleOutput.Triangles[startIdx + 1].Vert1 = vert3;
        TriangleOutput.Triangles[startIdx + 1].Vert2 = vert0;
        TriangleOutput.Triangles[startIdx + 1].Normal = faceNorm2;
      }
      else // entering
      {
        // Tri 1 face normal
        const glm::vec3 faceNorm1 = calculateFaceNormal(vert2, vert1, vert0);
        // Tri 2 face normal
        const glm::vec3 faceNorm2 = calculateFaceNormal(vert0, vert3, vert2);

        // Tri 1, v2, v1, v0
        TriangleOutput.Triangles[startIdx].Vert0 = vert2;
        TriangleOutput.Triangles[startIdx].Vert1 = vert1;
        TriangleOutput.Triangles[startIdx].Vert2 = vert0;
        TriangleOutput.Triangles[startIdx].Normal = faceNorm1;

        // Tri 2, v0, v3, v2
        TriangleOutput.Triangles[startIdx + 1].Vert0 = vert0;
        TriangleOutput.Triangles[startIdx + 1].Vert1 = vert3;
        TriangleOutput.Triangles[startIdx + 1].Vert2 = vert2;
        TriangleOutput.Triangles[startIdx + 1].Normal = faceNorm2;
      }
    }
  }

  // Construct quad for z edge
  if (xyz.x > 0 && xyz.y > 0)
  {
    // Is edge intersected?
    const glm::uint dataCellPosZ1 = getData(morton3D(xyz + glm::ivec3(0, 0, 1)));
    const bool entering = dataCellPos < iso && dataCellPosZ1 >= iso;
    const bool exiting = dataCellPos >= iso && dataCellPosZ1 < iso;

    if (entering || exiting)
    {
      // Generate quad
      const glm::vec3 vert0 = calculateDualPoint(xyz, iso, getDualPointCode(xyz, iso, EDGE3));
      const glm::vec3 vert1 = calculateDualPoint(xyzX1, iso, getDualPointCode(xyzX1, iso, EDGE1));
      const glm::vec3 vert2 = calculateDualPoint(xyzX1Y1, iso, getDualPointCode(xyzX1Y1, iso, EDGE5));
      const glm::vec3 vert3 = calculateDualPoint(xyzY1, iso, getDualPointCode(xyzY1, iso, EDGE7));

      // Reserve the next 2 triangle indices
      // atomicAdd returns the value before addition
      const glm::uint startIdx = TriangleCounter; TriangleCounter += 2u;

      if (exiting)
      {
        // Tri 1 face normal
        const glm::vec3 faceNorm1 = calculateFaceNormal(vert0, vert1, vert2);
        // Tri 2 face normal
        const glm::vec3 faceNorm2 = calculateFaceNormal(vert2, vert3, vert0);

        // Tri 1, v0, v1, v2
        TriangleOutput.Triangles[startIdx].Vert0 = vert0;
        TriangleOutput.Triangles[startIdx].Vert1 = vert1;
        TriangleOutput.Triangles[startIdx].Vert2 = vert2;
        TriangleOutput.Triangles[startIdx].Normal = faceNorm1;

        // Tri 2, v2, v3, v0
        TriangleOutput.Triangles[startIdx + 1].Vert0 = vert2;
        TriangleOutput.Triangles[startIdx + 1].Vert1 = vert3;
        TriangleOutput.Triangles[startIdx + 1].Vert2 = vert0;
        TriangleOutput.Triangles[startIdx + 1].Normal = faceNorm2;
      }
      else // entering
      {
        // Tri 1 face normal
        const glm::vec3 faceNorm1 = calculateFaceNormal(vert2, vert1, vert0);
        // Tri 2 face normal
        const glm::vec3 faceNorm2 = calculateFaceNormal(vert0, vert3, vert2);

        // Tri 1, v2, v1, v0
        TriangleOutput.Triangles[startIdx].Vert0 = vert2;
        TriangleOutput.Triangles[startIdx].Vert1 = vert1;
        TriangleOutput.Triangles[startIdx].Vert2 = vert0;
        TriangleOutput.Triangles[startIdx].Normal = faceNorm1;

        // Tri 2, v0, v3, v2
        TriangleOutput.Triangles[startIdx + 1].Vert0 = vert0;
        TriangleOutput.Triangles[startIdx + 1].Vert1 = vert3;
        TriangleOutput.Triangles[startIdx + 1].Vert2 = vert2;
        TriangleOutput.Triangles[startIdx + 1].Normal = faceNorm2;
      }
    }
  }
}

void DualMCComputeFramework::PackDualPointsList()
{
  // 1024 shorts (256*4), 512 uints
  DualPoints.DualPointsList.fill(0);

  for (int cubeCase = 0; cubeCase < 256; ++cubeCase)
  {
    const int startIdx = cubeCase * 2;

    const glm::uint* faces = dualPointsList[cubeCase];
    for (int i = 0; i < 2; ++i)
    {
      const int faceIdx = i * 2;
      DualPoints.DualPointsList[startIdx + i] |=  (faces[faceIdx  ] & 0xFFFFu);
      DualPoints.DualPointsList[startIdx + i] |=  (faces[faceIdx+1] & 0xFFFFu) << 16;
    }
  }
}

void DualMCComputeFramework::PackProblematicConfigs()
{
  // 256 bytes, 64 uints
  DualPoints.ProblematicConfigs.fill(0);

  for (int pack = 0; pack < 64; ++pack)
  {
    const int config = pack * 4;
    DualPoints.ProblematicConfigs[pack] |= (problematicConfigs[config   ]);
    DualPoints.ProblematicConfigs[pack] |= (problematicConfigs[config+1]) << 8;
    DualPoints.ProblematicConfigs[pack] |= (problematicConfigs[config+2]) << 16;
    DualPoints.ProblematicConfigs[pack] |= (problematicConfigs[config+3]) << 24;
  }
}

void DualMCComputeFramework::InsertNewDualPoint(uint32_t id, glm::vec3 p)
{
  id &= 0x3ffff; // Only want 18 bits
  // Use linear probing to find an open bucket
  while (true)
  {
    const uint64_t kv = DualPointMap[id];
    // If kv is sentinel, we can insert there, else we keep incrementing until we find an unoccupied bucket
    if (kv == ~0ull)
    {
      // atomicAdd returns current value then increments by given amount
      glm::uint pointIdx = CachedPointCounter; CachedPointCounter += 1;
      // Assign id and pointIdx
      DualPointMap[id] = ((uint64_t)id << 32) | (pointIdx & 0xffffffff);
      PrecalculatedDualPoints[pointIdx] = p;
      return;
    }
    id = (id + 1) & 0x3ffff;
  }
}

bool DualMCComputeFramework::FindDualPoint(uint32_t id, glm::vec3& outPoint)
{
  id &= 0x3ffff;
  // Use linear probing to find the point for this id, if it exists
  // If the matching bucket is filled, keep looking at the following buckets until we find one with the matching id
  // If we find an empty bucket before finding the matching id, the point is not in the map
  while (true)
  {
    const uint64_t& kv = DualPointMap[id];
    if (kv != ~0ull)
    {
      // Occupied, check key
      if (((kv >> 32) & 0x3ffff) == id)
      {
        // Found matching key, retrieve point index and return
        const uint32_t idx = kv & 0xffffffff;
        outPoint = PrecalculatedDualPoints[idx];
        return true;
      }
      // Else, we increment id and keep looking
      id = (id + 1) & 0x3ffff;
    }
    else // Equals sentinel value
    {
      break;
    }
  }

  return false;
}
