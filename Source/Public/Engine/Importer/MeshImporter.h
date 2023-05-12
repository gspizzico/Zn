#pragma once

#include <Core/HAL/BasicTypes.h>

DECLARE_LOG_CATEGORY(LogMeshImporter);

namespace Zn
{
struct RHIMesh;

class MeshImporter
{
  public:
    static bool Import(const String& fileName, RHIMesh& mesh);

  private:
    static bool Import_Obj(const String& fileName, RHIMesh& mesh);
    static bool Import_GLTF(const String& fileName, RHIMesh& mesh);
};
} // namespace Zn
