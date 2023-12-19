#pragma once

#include <Core/CoreMinimal.h>
#include <Core/Log/LogMacros.h>

DECLARE_LOG_CATEGORY(LogMeshImporter);

namespace Zn
{
struct RHIMesh;
struct RHIPrimitive;
struct TextureSource;
struct TextureSampler;

struct MeshImporterOutput
{
    Vector<RHIPrimitive>                  primitives;
    Map<String, SharedPtr<TextureSource>> textures;
    Map<String, TextureSampler>           samplers;
};

class MeshImporter
{
  public:
    static bool Import(const String& fileName_, RHIMesh& mesh_);
    static bool ImportAll(const String& fileName_, MeshImporterOutput& output_);

  private:
    static bool Import_Obj(const String& fileName_, RHIMesh& mesh_);

    static bool ImportAll_GLTF(const String& fileName_, MeshImporterOutput& mesh_);
};
} // namespace Zn
