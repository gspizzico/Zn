#include <Importers/TextureImporter.h>

#include <stb_image.h>

using namespace Zn;

namespace
{
struct STBILoader
{
    i32 width    = 0;
    i32 height   = 0;
    i32 channels = 0;
    i32 size     = 0;
    u8* data     = nullptr;

    STBILoader(const String& path)
    {
        data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (data)
        {
            size = width * height * 4;
        }
    }

    ~STBILoader()
    {
        if (data)
        {
            stbi_image_free(data);
        }
    }

    operator bool() const
    {
        return data != nullptr;
    }
};
} // namespace

SharedPtr<TextureSource> Zn::TextureImporter::Import(const String& path)
{
    const STBILoader loader(path);

    if (loader)
    {
        return SharedPtr<TextureSource>(new TextureSource {
            .width    = loader.width,
            .height   = loader.height,
            .channels = loader.channels,
            .data     = Vector<u8>(loader.data, loader.data + loader.size),
        });
    }

    return nullptr;
}
