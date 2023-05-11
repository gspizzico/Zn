#include <Znpch.h>

#include <Engine/Importer/TextureImporter.h>

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

using namespace Zn;

Zn::TextureSource::~TextureSource()
{
    if (data && importerType != TextureImporterType::None)
    {
        TextureImporter::Release(*this);
    }
}

SharedPtr<TextureSource> Zn::TextureImporter::Import(const String& path)
{
    i32 width    = 0;
    i32 height   = 0;
    i32 channels = 0;

    if (u8* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha))
    {
        return SharedPtr<TextureSource>(new TextureSource {
            .width = width, .height = height, .channels = channels, .size = (width * height * 4), .data = data, .importerType = TextureImporterType::STB});
    }

    return nullptr;
}

void Zn::TextureImporter::Release(const TextureSource& texture)
{
    if (texture.data != nullptr)
    {
        if (texture.importerType == TextureImporterType::STB)
        {
            stbi_image_free(texture.data);
        }
    }
}
