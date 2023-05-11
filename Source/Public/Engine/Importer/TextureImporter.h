#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
enum class TextureImporterType
{
    None,
    STB
};

struct TextureSource
{
    ~TextureSource();

    const i32                 width;
    const i32                 height;
    const i32                 channels;
    const i32                 size;
    u8* const                 data;
    const TextureImporterType importerType;
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