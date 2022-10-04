#ifndef BGFX_UTILS_WRAPPER_H
#define BGFX_UTILS_WRAPPER_H

#include "bgfx_components.h"

#ifdef __cplusplus
extern "C" {
#endif

EQUILIBRIUM_API bgfx_texture_handle_t loadTexture(const char *file);
EQUILIBRIUM_API void                  bx_string_copy(char *dir, char *file);
EQUILIBRIUM_API int32_t               bx_prettify(char *_out, int32_t _count, uint64_t _value);

#ifdef __cplusplus
}
#endif

#endif