#pragma once

#include <Core/HAL/BasicTypes.h>

DECLARE_LOG_CATEGORY(LogMeshImporter);

namespace Zn
{
struct RHIMesh;
struct RHIPrimitive;
struct TextureSource;
struct TextureSampler;

struct MeshImporterOutput
{
    Vector<RHIPrimitive>                           primitives;
    UnorderedMap<String, SharedPtr<TextureSource>> textures;
    UnorderedMap<String, TextureSampler>           samplers;
};

class MeshImporter
{
  public:
    static bool Import(const String& fileName, RHIMesh& mesh);
    static bool ImportAll(const String& fileName, MeshImporterOutput& output);

  private:
    static bool Import_Obj(const String& fileName, RHIMesh& mesh);

    static bool ImportAll_GLTF(const String& fileName, MeshImporterOutput& mesh);
};
} // namespace Zn
