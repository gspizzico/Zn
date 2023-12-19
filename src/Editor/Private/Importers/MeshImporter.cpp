#include <Importers/MeshImporter.h>
#include <Rendering/RHI/RHIMesh.h>
#include <filesystem>

using namespace Zn;

DEFINE_LOG_CATEGORY(LogMeshImporter, ELogVerbosity::Log);

bool Zn::MeshImporter::Import(const String& fileName_, RHIMesh& mesh_)
{
    std::filesystem::path filePath = fileName_;

    if (filePath.extension() == ".obj")
    {
        return Import_Obj(fileName_, mesh_);
    }

    return false;
}

bool Zn::MeshImporter::ImportAll(const String& fileName_, MeshImporterOutput& output_)
{
    std::filesystem::path filePath = fileName_;

    if (filePath.extension() == ".gltf")
    {
        return ImportAll_GLTF(fileName_, output_);
    }

    return false;
}
