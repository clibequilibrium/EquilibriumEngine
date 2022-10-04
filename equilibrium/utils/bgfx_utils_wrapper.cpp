#include "bgfx_utils_wrapper.h"
#include <bgfx/bgfx.h>
#include <bimg/include/bimg/decode.h>
#include <bx/file.h>
#include <bx/bx.h>
#include <flecs.h>
#include <stdio.h>
#include <string>
#include <iostream>

using std::string;

static string getFileExt(const string &s) {

    size_t i = s.rfind('.', s.length());
    if (i != string::npos) {
        return (s.substr(i + 1, s.length() - i));
    }

    return ("");
}

static bx::DefaultAllocator allocator;
bgfx_texture_handle_t       loadTexture(const char *file) {

    if (getFileExt(string(file)) != "dds") {
        ecs_err("Only .dds textures are supported");
        return (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    }

    void    *data = nullptr;
    uint32_t size = 0;

    bx::FileReader reader;
    bx::Error      err;
    if (bx::open(&reader, file, &err)) {
        size = (uint32_t)bx::getSize(&reader);
        data = BX_ALLOC(&allocator, size);
        bx::read(&reader, data, size, &err);
        bx::close(&reader);
    }

    if (!err.isOk()) {
        BX_FREE(&allocator, data);
        printf("%s | %s", err.getMessage().getPtr(), file);
        return (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    }

    bimg::ImageContainer *image = bimg::imageParse(&allocator, data, size);
    if (image) {
        // the callback gets called when bgfx is done using the data (after 2
        // frames)
        const bgfx::Memory *mem = bgfx::makeRef(
            image->m_data, image->m_size,
            [](void *, void *data) { bimg::imageFree((bimg::ImageContainer *)data); }, image);
        BX_FREE(&allocator, data);

        // default wrap mode is repeat, there's no flag for it
        uint64_t textureFlags =
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;

        if (bgfx::isTextureValid(0, false, image->m_numLayers,
                                 (bgfx::TextureFormat::Enum)image->m_format, textureFlags)) {
            bgfx::TextureHandle tex = bgfx::createTexture2D(
                (uint16_t)image->m_width, (uint16_t)image->m_height, image->m_numMips > 1,
                image->m_numLayers, (bgfx::TextureFormat::Enum)image->m_format, textureFlags, mem);
            // bgfx::setName(tex, file); // causes debug errors with DirectX
            // SetPrivateProperty duplicate
            return (bgfx_texture_handle_t){tex.idx};
        } else {
            ecs_err("%s", "Unsupported image format");
            return (bgfx_texture_handle_t){bgfx::kInvalidHandle};
        }
    }

    BX_FREE(&allocator, data);
    ecs_err("%s", err.getMessage().getPtr());

    return (bgfx_texture_handle_t){bgfx::kInvalidHandle};
}

void bx_string_copy(char *dir, char *file) { bx::strCopy(dir, 1024, bx::FilePath(file).getPath()); }

int32_t bx_prettify(char *_out, int32_t _count, uint64_t _value) {
    return bx::prettify(_out, _count, _value);
}