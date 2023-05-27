#pragma once

namespace Zn
{
class IO
{
  public:
    static void Initialize(const String& root);

    static bool ReadBinaryFile(const String& InFilename, Vector<uint8>& OutData);

    static bool ReadTextFile(const String& InFilename, Vector<const char*>& OutData);

    static String GetAbsolutePath(const String& InFilename);

  private:
    static String kRootPath;
};

} // namespace Zn
