#include <memory>
#include <limits>
#include <cstdio>
#include <fstream>
#include <vector>

#include "FastNoise.h"
#include "dualmc-compute.h"


int main()
{
  FastNoise noise;
  noise.SetSeed(42424242);
  noise.SetFractalOctaves(5);
  noise.SetFractalGain(0.5);
  std::unique_ptr<DualMCComputeFramework> computeFramework = std::make_unique<DualMCComputeFramework>();

  // Fill short array
  auto volumeData = std::make_unique<std::array<unsigned short, DualMCComputeFramework::VolumeDataSize * 2>>();
  volumeData->fill(0);
  for (int z = 0; z < 36; ++z)
  {
    for (int y = 0; y < 36; ++y)
    {
      for (int x = 0; x < 36; ++x)
      {
        const glm::uint mortonCode = computeFramework->morton3D(glm::ivec3(x, y, z));

        float val = noise.GetSimplexFractal((float)x, (float)y, (float)z);

        // Clamp, shift range, covert to short
        val = glm::clamp(val, -1.f, 1.f);
        val = (val * 0.5f) + 0.5f;
        const uint16_t shortVal = static_cast<uint16_t>(val * std::numeric_limits<uint16_t>::max());
        (*volumeData)[mortonCode] = shortVal;
      }
    }
  }
  computeFramework->SetData(*reinterpret_cast<std::array<glm::uint, DualMCComputeFramework::VolumeDataSize>*>(volumeData.get()));

  computeFramework->execute();

  const auto& [triangles, count] = computeFramework->GetTriangles();
  if (count > 0)
  {
    printf("Writing obj file...");

    std::ofstream file("output.obj");
    if (!file)
      return -1;

    // vert loop
    for (glm::uint triIdx = 0; triIdx < count; ++triIdx)
    {
      const Triangle& tri = triangles[triIdx];
      file << "v " << tri.Vert0.x << " " << tri.Vert0.y << " " << tri.Vert0.z << "\n";
      file << "v " << tri.Vert1.x << " " << tri.Vert1.y << " " << tri.Vert1.z << "\n";
      file << "v " << tri.Vert2.x << " " << tri.Vert2.y << " " << tri.Vert2.z << "\n";
    }

    // normal loop
    for (glm::uint triIdx = 0; triIdx < count; ++triIdx)
    {
      const Triangle& tri = triangles[triIdx];
      file << "vn " << tri.Normal.x << " " << tri.Normal.y << " " << tri.Normal.z << "\n";
      file << "vn " << tri.Normal.x << " " << tri.Normal.y << " " << tri.Normal.z << "\n";
      file << "vn " << tri.Normal.x << " " << tri.Normal.y << " " << tri.Normal.z << "\n";
    }

    // face loop
    for (glm::uint triIdx = 0; triIdx < count; ++triIdx)
    {
      const Triangle& tri = triangles[triIdx];
      const glm::uint triIdx3 = triIdx * 3;
      file << "f " << triIdx3+1 << " " << triIdx3+2 << " " << triIdx3+3 << "\n";
    }

    file.close();
  }

  return 0;
}
