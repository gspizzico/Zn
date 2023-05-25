#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
struct TextureSource
{
    const i32  width;
    const i32  height;
    const i32  channels;
    Vector<u8> data;
};

class TextureImporter
{
  public:
    static SharedPtr<TextureSource> Import(const String& path);

  private:
    friend TextureSource;

    static void Release(const TextureSource& texture);
};
} // namespace Zn
