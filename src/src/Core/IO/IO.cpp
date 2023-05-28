#include <IO/IO.h>
#include <CoreAssert.h>
#include <CommandLine.h>
#include <fstream>
#include <filesystem>

using namespace Zn;

namespace
{
String GRootPath = "";
}

void IO::Initialize(const String& root_)
{
    GRootPath = root_;
}

bool IO::ReadBinaryFile(const String& filename_, Vector<uint8>& outData_)
{
    String absFilename = GetAbsolutePath(filename_);

    // Open the file. With cursor at the end
    std::ifstream file(absFilename.c_str(), std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    size_t size = file.tellg();

    outData_.resize(size);

    file.seekg(0);

    file.read((char*) outData_.data(), size);

    file.close();

    return true;
}

bool IO::ReadTextFile(const String& filename_, Vector<const char*>& outData_)
{
    check(false); // Not Implemented
    return false;
}

String IO::GetAbsolutePath(const String& filename_)
{
    std::filesystem::path path(filename_);

    if (path.is_absolute())
    {
        return filename_;
    }
    else
    {
        std::filesystem::path outPath = std::filesystem::path(GRootPath) / path;

        return outPath.string();
    }
}
