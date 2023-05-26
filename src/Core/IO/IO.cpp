#include <Corepch.h>
#include <IO/IO.h>
#include <CommandLine.h>

#include <fstream>
#include <filesystem>

using namespace Zn;

String IO::kExecutablePath = "";

String IO::kRootPath = "";

void Zn::IO::Initialize()
{
    kExecutablePath = CommandLine::Get().GetExeArgument();

    std::filesystem::path RootPath(kExecutablePath);

    kRootPath = RootPath.parent_path().parent_path().parent_path().parent_path().string();
}

bool IO::ReadBinaryFile(const String& InFilename, Vector<uint8>& OutData)
{
    String AbsFilename = GetAbsolutePath(InFilename);

    // Open the file. With cursor at the end
    std::ifstream File(AbsFilename.c_str(), std::ios::ate | std::ios::binary);

    if (!File.is_open())
    {
        return false;
    }

    size_t Size = File.tellg();

    OutData.resize(Size);

    File.seekg(0);

    File.read((char*) OutData.data(), Size);

    File.close();

    return true;
}

bool IO::ReadTextFile(const String& InFilename, Vector<const char*>& OutData)
{
    check(false); // Not Implemented
    return false;
}

String IO::GetAbsolutePath(const String& InFilename)
{
    std::filesystem::path Path(InFilename);

    if (Path.is_absolute())
    {
        return InFilename;
    }
    else
    {
        std::filesystem::path Root(kRootPath);
        std::filesystem::path OutputPath = Root / Path;

        return OutputPath.string();
    }
}
