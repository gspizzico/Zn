#include <Znpch.h>
#include <Engine/Importer/MeshImporter.h>
#include <Rendering/RHI/RHIMesh.h>
#include <filesystem>

using namespace Zn;

DEFINE_LOG_CATEGORY(LogMeshImporter, ELogVerbosity::Log);

bool Zn::MeshImporter::Import(const String& fileName, RHIMesh& mesh)
{
    std::filesystem::path filePath = fileName;

    if (filePath.extension() == ".obj")
    {
        return Import_Obj(fileName, mesh);
    }
    else if (filePath.extension() == ".gltf")
    {
        return Import_GLTF(fileName, mesh);
    }

    return false;
}

bool Zn::MeshImporter::ImportAll(const String& fileName, MeshImporterOutput& output)
{
    std::filesystem::path filePath = fileName;

    if (filePath.extension() == ".gltf")
    {
        return ImportAll_GLTF(fileName, output);
    }

    return false;
}
