#pragma once

#include <CoreTypes.h>

namespace Zn
{
class IO
{
  public:
    static void Initialize(const String& root_);

    static bool ReadBinaryFile(const String& filename_, Vector<uint8>& outData_);

    static bool ReadTextFile(const String& filename_, Vector<const char*>& outData_);

    static String GetAbsolutePath(const String& filename_);
};

} // namespace Zn
