#ifndef BGFX_UTILS_H
#define BGFX_UTILS_H

#include "bgfx/c99/bgfx.h"
#include "components/renderer/renderer_components.h"
#include "components/scene/scene_components.h"
#include "components/cglm_components.h"
#include "components/transform.h"
#include "bgfx_utils_wrapper.h"
#include "flecs.h"
#include "systems/rendering/gfx_resource_system.h"
#include <corecrt.h>
#include <stdint.h>
#include <stdio.h>
#include "bgfx_components.h"

#define BX_COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))
#define BX_MAKEFOURCC(_a, _b, _c, _d)                                                              \
    (((uint32_t)(_a) | ((uint32_t)(_b) << 8) | ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

#define kChunkVertexBuffer           BX_MAKEFOURCC('V', 'B', ' ', 0x1)
#define kChunkVertexBufferCompressed BX_MAKEFOURCC('V', 'B', 'C', 0x0)
#define kChunkIndexBuffer            BX_MAKEFOURCC('I', 'B', ' ', 0x0)
#define kChunkIndexBufferCompressed  BX_MAKEFOURCC('I', 'B', 'C', 0x1)
#define kChunkPrimitive              BX_MAKEFOURCC('P', 'R', 'I', 0x0)

typedef struct ShaderHandle {
    bgfx_shader_handle_t handle;
    char                *file_path;
} ShaderHandle;

typedef struct AttribToId {
    bgfx_attrib_t atrib;
    uint16_t      id;
} AttribToId;

static AttribToId s_attribToId[] = {
    // NOTICE:
    // Attrib must be in order how it appears in Attrib::Enum! id is
    // unique and should not be changed if new Attribs are added.
    {BGFX_ATTRIB_POSITION, 0x0001},  {BGFX_ATTRIB_NORMAL, 0x0002},
    {BGFX_ATTRIB_TANGENT, 0x0003},   {BGFX_ATTRIB_BITANGENT, 0x0004},
    {BGFX_ATTRIB_COLOR0, 0x0005},    {BGFX_ATTRIB_COLOR1, 0x0006},
    {BGFX_ATTRIB_COLOR2, 0x0018},    {BGFX_ATTRIB_COLOR3, 0x0019},
    {BGFX_ATTRIB_INDICES, 0x000e},   {BGFX_ATTRIB_WEIGHT, 0x000f},
    {BGFX_ATTRIB_TEXCOORD0, 0x0010}, {BGFX_ATTRIB_TEXCOORD1, 0x0011},
    {BGFX_ATTRIB_TEXCOORD2, 0x0012}, {BGFX_ATTRIB_TEXCOORD3, 0x0013},
    {BGFX_ATTRIB_TEXCOORD4, 0x0014}, {BGFX_ATTRIB_TEXCOORD5, 0x0015},
    {BGFX_ATTRIB_TEXCOORD6, 0x0016}, {BGFX_ATTRIB_TEXCOORD7, 0x0017},
};

typedef struct AttribTypeToId {
    bgfx_attrib_type_t type;
    uint16_t           id;
} AttribTypeToId;

static AttribTypeToId s_attribTypeToId[] = {
    // NOTICE:
    // AttribType must be in order how it appears in AttribType::Enum!
    // id is unique and should not be changed if new AttribTypes are
    // added.
    {BGFX_ATTRIB_TYPE_UINT8, 0x0001}, {BGFX_ATTRIB_TYPE_UINT10, 0x0005},
    {BGFX_ATTRIB_TYPE_INT16, 0x0002}, {BGFX_ATTRIB_TYPE_HALF, 0x0003},
    {BGFX_ATTRIB_TYPE_FLOAT, 0x0004},
};

static inline bgfx_attrib_t idToAttrib(uint16_t id) {
    for (uint32_t ii = 0; ii < BX_COUNT_OF(s_attribToId); ++ii) {
        if (s_attribToId[ii].id == id) {
            return s_attribToId[ii].atrib;
        }
    }

    return BGFX_ATTRIB_COUNT;
}

static inline bgfx_attrib_type_t idToAttribType(uint16_t id) {
    for (uint32_t ii = 0; ii < BX_COUNT_OF(s_attribTypeToId); ++ii) {
        if (s_attribTypeToId[ii].id == id) {
            return s_attribTypeToId[ii].type;
        }
    }

    return BGFX_ATTRIB_TYPE_COUNT;
}

static int32_t read_vertex_layout(bgfx_vertex_layout_t *layout, FILE *file) {
    int32_t total = 0;

    uint8_t numAttrs;
    total += fread(&numAttrs, 1, sizeof(numAttrs), file);

    uint16_t stride;
    total += fread(&stride, 1, sizeof(stride), file);

    if (ferror(file) || feof(file)) {
        return total;
    }

    bgfx_vertex_layout_begin(layout, bgfx_get_renderer_type());

    for (uint32_t ii = 0; ii < numAttrs; ++ii) {
        uint16_t offset;
        total += fread(&offset, 1, sizeof(offset), file);

        uint16_t attribId = 0;
        total += fread(&attribId, 1, sizeof(attribId), file);

        uint8_t num;
        total += fread(&num, 1, sizeof(num), file);

        uint16_t attribTypeId;
        total += fread(&attribTypeId, 1, sizeof(attribTypeId), file);

        bool normalized;
        total += fread(&normalized, 1, sizeof(normalized), file);

        bool asInt;
        total += fread(&asInt, 1, sizeof(asInt), file);

        if (ferror(file) || feof(file)) {
            return total;
        }

        bgfx_attrib_t      attr = idToAttrib(attribId);
        bgfx_attrib_type_t type = idToAttribType(attribTypeId);

        if (BGFX_ATTRIB_COUNT != attr && BGFX_ATTRIB_TYPE_COUNT != type) {
            bgfx_vertex_layout_add(layout, attr, num, type, normalized, asInt);
            layout->offset[attr] = offset;
        }
    }

    bgfx_vertex_layout_end(layout);
    layout->stride = stride;

    return total;
}

static inline entity_t create_gfx_resource(world_t *world, ResourceType type, uint16_t handle) {
    static const char *strings[] = {"Invalid",     "Texture", "VertexBuffer", "DynamicVertexBuffer",
                                    "IndexBuffer", "Program", "Shader",       "FrameBuffer",
                                    "Uniform"};

    if (ecs_id_is_valid(world, ecs_id(GfxResource))) {
        return entity_create(world, strings[type], GfxResource, {type, handle});
    } else {
        return (entity_t){INT64_MIN, NULL};
    }
}

static ShaderHandle shader_load(world_t *world, const char *name, bool name_contains_shader_path) {

    char                *file_path;
    bgfx_shader_handle_t invalid = BGFX_INVALID_HANDLE;

    if (name_contains_shader_path) {
        file_path = (char *)name;
    } else {
        const char *shader_path = "???";

        // dx11/  dx9/   essl/  glsl/  metal/ pssl/  spirv/

        switch ((int32_t)bgfx_get_renderer_type()) {
        case BGFX_RENDERER_TYPE_NOOP:
        case BGFX_RENDERER_TYPE_DIRECT3D9:
            shader_path = "shaders/dx9/";
            break;
        case BGFX_RENDERER_TYPE_DIRECT3D11:
        case BGFX_RENDERER_TYPE_DIRECT3D12:
            shader_path = "shaders/dx11/";
            break;
        case BGFX_RENDERER_TYPE_GNM:
            shader_path = "shaders/pssl/";
            break;
        case BGFX_RENDERER_TYPE_METAL:
            shader_path = "shaders/metal/";
            break;
        case BGFX_RENDERER_TYPE_OPENGL:
            shader_path = "shaders/glsl/";
            break;
        case BGFX_RENDERER_TYPE_OPENGLES:
            shader_path = "shaders/essl/";
            break;
        case BGFX_RENDERER_TYPE_VULKAN:
            shader_path = "shaders/spirv/";
            break;
        case BGFX_RENDERER_TYPE_NVN:
        case BGFX_RENDERER_TYPE_WEBGPU:
        case BGFX_RENDERER_TYPE_COUNT:
            return (ShaderHandle){invalid, NULL}; // count included to keep compiler warnings happy
        }

        size_t shader_length = strlen(shader_path);
        size_t file_length   = strlen(name);
        file_path            = (char *)malloc(shader_length + file_length + 1);
        memcpy(file_path, shader_path, shader_length);
        memcpy(&file_path[shader_length], name, file_length);
        file_path[shader_length + file_length] = 0; // properly null terminate
    }

    errno_t err;
    FILE   *file;
    err = fopen_s(&file, file_path, "rb");

    if (err != 0) {
        ecs_err("Shader file %s not found.", name);
        return (ShaderHandle){invalid, NULL};
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);

    if (file_size == 0) {
        return (ShaderHandle){invalid, NULL};
    }

    fseek(file, 0, SEEK_SET);

    const bgfx_memory_t *memory = bgfx_alloc(file_size);
    fread(memory->data, 1, file_size, file);
    fclose(file);

    bgfx_shader_handle_t handle = bgfx_create_shader(memory);

    if (!BGFX_HANDLE_IS_VALID(handle)) {
        ecs_err("Shader model not supported for %s", name);
    }

    return (ShaderHandle){handle, file_path};
}

static inline bgfx_uniform_handle_t create_uniform(world_t *world, const char *name,
                                                   bgfx_uniform_type_t type) {
    bgfx_uniform_handle_t handle = bgfx_create_uniform(name, type, 1);
    create_gfx_resource(world, RESOURCE_TYPE_UNIFORM, handle.idx);
    return handle;
}

static inline bgfx_uniform_handle_t create_uniform_w_num(world_t *world, const char *name,
                                                         bgfx_uniform_type_t type, uint16_t num) {
    bgfx_uniform_handle_t handle = bgfx_create_uniform(name, type, num);
    create_gfx_resource(world, RESOURCE_TYPE_UNIFORM, handle.idx);
    return handle;
}

#define create_program(entity, member_name, T, vertex_shader_name, fragment_shader_name)           \
    ({                                                                                             \
        ShaderHandle vertex_handle   = shader_load(entity.world, vertex_shader_name, false);       \
        ShaderHandle fragment_handle = shader_load(entity.world, fragment_shader_name, false);     \
        bgfx_program_handle_t handle =                                                             \
            bgfx_create_program(vertex_handle.handle, fragment_handle.handle, true);               \
        entity_t program_entity =                                                                  \
            create_gfx_resource(entity.world, RESOURCE_TYPE_PROGRAM, handle.idx);                  \
                                                                                                   \
        if (entity_valid(program_entity)) {                                                        \
            ecs_doc_set_name(program_entity.world, program_entity.handle, #member_name);           \
            ecs_set(program_entity.world, program_entity.handle, HotReloadableShader,              \
                    {(entity_t){entity.handle, entity.world}, ecs_id(T), ECS_SIZEOF(T),            \
                     offsetof(T, member_name), vertex_handle.file_path,                            \
                     fragment_handle.file_path});                                                  \
        }                                                                                          \
                                                                                                   \
        handle;                                                                                    \
    })

// static bgfx_program_handle_t create_program(world_t *world, const char
// *vertex_shader_name,
//                                             const char *fragment_shader_name)
//                                             {

//   bgfx_program_handle_t handle = bgfx_create_program(
//       shader_load(world, vertex_shader_name), shader_load(world,
//       fragment_shader_name), false);
//   create_gfx_resource(world, Program, handle.idx);
//   return handle;
// }

static inline bgfx_program_handle_t create_compute_program(world_t    *world,
                                                           const char *shader_name) {
    bgfx_program_handle_t handle =
        bgfx_create_compute_program(shader_load(world, shader_name, false).handle, true);
    create_gfx_resource(world, RESOURCE_TYPE_PROGRAM, handle.idx);
    return handle;
}

static inline bgfx_texture_handle_t
create_texture_2d(world_t *world, uint16_t width, uint16_t height, bool hasMips, uint16_t numLayers,
                  bgfx_texture_format_t format, uint64_t flags, const bgfx_memory_t *mem) {
    bgfx_texture_handle_t handle =
        bgfx_create_texture_2d(width, height, hasMips, numLayers, format, flags, mem);
    create_gfx_resource(world, RESOURCE_TYPE_TEXTURE, handle.idx);
    return handle;
}

static inline bgfx_texture_handle_t load_texture(world_t *world, const char *file) {
    bgfx_texture_handle_t handle = loadTexture(file);

    if (!BGFX_HANDLE_IS_VALID(handle)) {
        handle = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    } else {
        create_gfx_resource(world, RESOURCE_TYPE_TEXTURE, handle.idx);
    }

    return handle;
}

static inline bgfx_texture_handle_t
create_texture_2d_scaled(world_t *world, bgfx_backbuffer_ratio_t ratio, bool hasMips,
                         uint16_t numLayers, bgfx_texture_format_t format, uint64_t flags) {
    bgfx_texture_handle_t handle =
        bgfx_create_texture_2d_scaled(ratio, hasMips, numLayers, format, flags);
    create_gfx_resource(world, RESOURCE_TYPE_TEXTURE, handle.idx);
    return handle;
}

static inline bgfx_dynamic_vertex_buffer_handle_t
create_dynamic_vertex_buffer(world_t *world, uint32_t num, const bgfx_vertex_layout_t *layout,
                             uint16_t flags) {
    bgfx_dynamic_vertex_buffer_handle_t handle =
        bgfx_create_dynamic_vertex_buffer(num, layout, flags);
    create_gfx_resource(world, RESOURCE_TYPE_DYNAMIC_VERTEX_BUFFER, handle.idx);
    return handle;
}

static inline bgfx_vertex_buffer_handle_t create_vertex_buffer(world_t                    *world,
                                                               const bgfx_memory_t        *mem,
                                                               const bgfx_vertex_layout_t *layout,
                                                               uint16_t                    flags) {
    bgfx_vertex_buffer_handle_t handle = bgfx_create_vertex_buffer(mem, layout, flags);
    create_gfx_resource(world, RESOURCE_TYPE_VERTEX_BUFFER, handle.idx);
    return handle;
}

static inline bgfx_index_buffer_handle_t
create_index_buffer(world_t *world, const bgfx_memory_t *mem, uint16_t flags) {
    bgfx_index_buffer_handle_t handle = bgfx_create_index_buffer(mem, flags);
    create_gfx_resource(world, RESOURCE_TYPE_INDEX_BUFFER, handle.idx);
    return handle;
}

static inline bgfx_frame_buffer_handle_t
create_frame_buffer_from_handles(world_t *world, uint8_t num,
                                 const bgfx_texture_handle_t *handles) {
    bgfx_frame_buffer_handle_t handle = bgfx_create_frame_buffer_from_handles(num, handles, false);
    create_gfx_resource(world, RESOURCE_TYPE_FRAME_BUFFER, handle.idx);
    return handle;
}

static entity_t mesh_load(const char *name, world_t *world) {
    const char *shader_path = "models/";

    Mesh mesh;
    mesh.groups = ecs_vector_new(Group, 0);

    size_t shader_length = strlen(shader_path);
    size_t file_length   = strlen(name);
    char  *file_path     = (char *)malloc(shader_length + file_length + 1);
    memcpy(file_path, shader_path, shader_length);
    memcpy(&file_path[shader_length], name, file_length);
    file_path[shader_length + file_length] = 0; // properly null terminate

    errno_t err;
    FILE   *file;
    err = fopen_s(&file, file_path, "rb");

    if (err != 0) {
        ecs_err("Model file %s not found.", name);
        return (entity_t){};
    }

    Group                group;
    bgfx_vertex_layout_t layout;

    group.primitives = ecs_vector_new(Primitive, 0);

    uint32_t chunk;
    while ((4 == fread(&chunk, 1, sizeof(chunk), file))) {

        switch (chunk) {
        case kChunkVertexBuffer: {
            fread(&group.sphere, 1, sizeof(Sphere), file);
            fread(&group.aabb, 1, sizeof(AABB), file);
            fread(&group.obb, 1, sizeof(OBB), file);

            read_vertex_layout(&layout, file);
            uint16_t stride = layout.stride;

            fread(&group.num_vertices, 1, sizeof(group.num_vertices), file);
            const bgfx_memory_t *mem = bgfx_alloc(group.num_vertices * stride);
            fread(mem->data, 1, mem->size, file);

            // if (_ramcopy) {
            //   group.m_vertices = (uint8_t *)BX_ALLOC(allocator,
            //   group.m_numVertices * stride); bx::memCopy(group.m_vertices,
            //   mem->data, mem->size);
            // }

            group.vertex_buffer = create_vertex_buffer(world, mem, &layout, BGFX_BUFFER_NONE);
        } break;

        case kChunkVertexBufferCompressed: {
            fread(&group.sphere, 1, sizeof(group.sphere), file);
            fread(&group.aabb, 1, sizeof(group.aabb), file);
            fread(&group.obb, 1, sizeof(group.obb), file);

            read_vertex_layout(&layout, file);
            uint16_t stride = layout.stride;

            fread(&group.num_vertices, 1, sizeof(group.num_vertices), file);
            const bgfx_memory_t *mem = bgfx_alloc(group.num_vertices * stride);

            uint32_t compressedSize;
            fread(&compressedSize, 1, sizeof(compressedSize), file);

            void *compressedVertices = ecs_os_alloca(compressedSize);
            fread(&compressedVertices, 1, compressedSize, file);

            // TODO
            // decodeVertexBuffer(mem->data, group.num_vertices, stride,
            // (uint8_t
            // *)compressedVertices,
            //                    compressedSize);

            // BX_FREE(allocator, compressedVertices);

            // if (_ramcopy) {
            //   group.m_vertices = (uint8_t *)BX_ALLOC(allocator,
            //   group.m_numVertices * stride); bx::memCopy(group.m_vertices,
            //   mem->data, mem->size);
            // }

            // group.m_vbh = bgfx::createVertexBuffer(mem, m_layout);
        } break;

        case kChunkIndexBuffer: {
            fread(&group.num_indices, 1, sizeof(group.num_indices), file);

            const bgfx_memory_t *mem = bgfx_alloc(group.num_indices * 2);
            fread(mem->data, 1, mem->size, file);

            // if (_ramcopy) {
            //   group.m_indices = (uint16_t *)BX_ALLOC(allocator,
            //   group.m_numIndices * 2); bx::memCopy(group.m_indices,
            //   mem->data, mem->size);
            // }

            group.index_buffer = create_index_buffer(world, mem, BGFX_BUFFER_NONE);
        } break;

        case kChunkIndexBufferCompressed: {
            // bx::read(_reader, group.m_numIndices, &err);

            // const bgfx::Memory *mem = bgfx::alloc(group.m_numIndices * 2);

            // uint32_t compressedSize;
            // bx::read(_reader, compressedSize, &err);

            // void *compressedIndices = BX_ALLOC(allocator, compressedSize);

            // bx::read(_reader, compressedIndices, compressedSize, &err);

            // meshopt_decodeIndexBuffer(mem->data, group.m_numIndices, 2,
            // (uint8_t
            // *)compressedIndices,
            //                           compressedSize);

            // BX_FREE(allocator, compressedIndices);

            // if (_ramcopy) {
            //   group.m_indices = (uint16_t *)BX_ALLOC(allocator,
            //   group.m_numIndices * 2); bx::memCopy(group.m_indices,
            //   mem->data, mem->size);
            // }

            // group.m_ibh = bgfx::createIndexBuffer(mem);
        } break;

        case kChunkPrimitive: {
            uint16_t len;
            fread(&len, 1, sizeof(len), file);

            char material[len];
            fread(&material, 1, len, file);

            uint16_t num;
            fread(&num, 1, sizeof(num), file);

            for (uint32_t ii = 0; ii < num; ++ii) {
                fread(&len, 1, sizeof(len), file);

                char name[len];
                fread(&name, 1, len, file);

                Primitive *prim = ecs_vector_add(&group.primitives, Primitive);
                fread(&prim->start_index, 1, sizeof(prim->start_index), file);
                fread(&prim->num_indices, 1, sizeof(prim->num_indices), file);
                fread(&prim->start_vertex, 1, sizeof(prim->start_vertex), file);
                fread(&prim->num_vertices, 1, sizeof(prim->num_vertices), file);
                fread(&prim->sphere, 1, sizeof(Sphere), file);
                fread(&prim->aabb, 1, sizeof(AABB), file);
                fread(&prim->obb, 1, sizeof(OBB), file);
            }

            ecs_os_memcpy(ecs_vector_add(&mesh.groups, Group), &group, sizeof(Group));
            group.index_buffer  = (bgfx_index_buffer_handle_t)BGFX_INVALID_HANDLE;
            group.vertex_buffer = (bgfx_vertex_buffer_handle_t)BGFX_INVALID_HANDLE;
            group.num_vertices  = 0;
            group.vertices      = NULL;
            group.num_indices   = 0;
            group.indices       = NULL;
            group.primitives    = ecs_vector_new(Primitive, 0);

        } break;

        default:
            ecs_err("%08x at %d", chunk, 0);
            break;
        }
    }

    fclose(file);
    free(file_path);

    entity_t meshEntity = entity_create(world, name, Mesh, {mesh.groups});
    entity_add_component(meshEntity, Position, {0, 0, 0});
    entity_add_component(meshEntity, Rotation, {0, 0, 0});
    entity_add_component(meshEntity, Scale, {1, 1, 1});

    return meshEntity;
}

static inline float truncbx(float a) { return (float)((int)(a)); }
static inline float fract(float a) { return a - truncbx(a); }

static inline float floorbx(float a) {
    if (a < 0.0f) {
        const float fr     = fract(-a);
        const float result = -a - fr;

        return -(0.0f != fr ? result + 1.0f : result);
    }

    return a - fract(a);
}

static inline float mod(float a, float b) { return a - b * floorbx(a / b); }

static inline void mul_h(vec3 vec, float *mat, vec3 dest) {

    const float xx   = vec[0] * mat[0] + vec[1] * mat[4] + vec[2] * mat[8] + mat[12];
    const float yy   = vec[0] * mat[1] + vec[1] * mat[5] + vec[2] * mat[9] + mat[13];
    const float zz   = vec[0] * mat[2] + vec[1] * mat[6] + vec[2] * mat[10] + mat[14];
    const float ww   = vec[0] * mat[3] + vec[1] * mat[7] + vec[2] * mat[11] + mat[15];
    const float invW = glm_sign(ww) / ww;

    vec3 result = {
        xx * invW,
        yy * invW,
        zz * invW,
    };

    glm_vec3_copy(result, dest);
}

static inline void mtx_cofactor(float *_result, float *_a) {
    const float xx = _a[0];
    const float xy = _a[1];
    const float xz = _a[2];
    const float xw = _a[3];
    const float yx = _a[4];
    const float yy = _a[5];
    const float yz = _a[6];
    const float yw = _a[7];
    const float zx = _a[8];
    const float zy = _a[9];
    const float zz = _a[10];
    const float zw = _a[11];
    const float wx = _a[12];
    const float wy = _a[13];
    const float wz = _a[14];
    const float ww = _a[15];

    _result[0] = +(yy * (zz * ww - wz * zw) - yz * (zy * ww - wy * zw) + yw * (zy * wz - wy * zz));
    _result[1] = -(yx * (zz * ww - wz * zw) - yz * (zx * ww - wx * zw) + yw * (zx * wz - wx * zz));
    _result[2] = +(yx * (zy * ww - wy * zw) - yy * (zx * ww - wx * zw) + yw * (zx * wy - wx * zy));
    _result[3] = -(yx * (zy * wz - wy * zz) - yy * (zx * wz - wx * zz) + yz * (zx * wy - wx * zy));

    _result[4] = -(xy * (zz * ww - wz * zw) - xz * (zy * ww - wy * zw) + xw * (zy * wz - wy * zz));
    _result[5] = +(xx * (zz * ww - wz * zw) - xz * (zx * ww - wx * zw) + xw * (zx * wz - wx * zz));
    _result[6] = -(xx * (zy * ww - wy * zw) - xy * (zx * ww - wx * zw) + xw * (zx * wy - wx * zy));
    _result[7] = +(xx * (zy * wz - wy * zz) - xy * (zx * wz - wx * zz) + xz * (zx * wy - wx * zy));

    _result[8]  = +(xy * (yz * ww - wz * yw) - xz * (yy * ww - wy * yw) + xw * (yy * wz - wy * yz));
    _result[9]  = -(xx * (yz * ww - wz * yw) - xz * (yx * ww - wx * yw) + xw * (yx * wz - wx * yz));
    _result[10] = +(xx * (yy * ww - wy * yw) - xy * (yx * ww - wx * yw) + xw * (yx * wy - wx * yy));
    _result[11] = -(xx * (yy * wz - wy * yz) - xy * (yx * wz - wx * yz) + xz * (yx * wy - wx * yy));

    _result[12] = -(xy * (yz * zw - zz * yw) - xz * (yy * zw - zy * yw) + xw * (yy * zz - zy * yz));
    _result[13] = +(xx * (yz * zw - zz * yw) - xz * (yx * zw - zx * yw) + xw * (yx * zz - zx * yz));
    _result[14] = -(xx * (yy * zw - zy * yw) - xy * (yx * zw - zx * yw) + xw * (yx * zy - zx * yy));
    _result[15] = +(xx * (yy * zz - zy * yz) - xy * (yx * zz - zx * yz) + xz * (yx * zy - zx * yy));
}

static bool renderer_supported(bool deferred) {
    const bgfx_caps_t *caps = bgfx_get_caps();

    bool supported =
        // SDR color attachment
        (caps->formats[BGFX_TEXTURE_FORMAT_BGRA8] & BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER) != 0 &&
        // HDR color attachment
        (caps->formats[BGFX_TEXTURE_FORMAT_RGBA16F] & BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER) != 0;

    if (deferred) {
        supported = supported && // blitting depth texture after geometry pass
                    (caps->supported & BGFX_CAPS_TEXTURE_BLIT) != 0 &&
                    // multiple render targets
                    // depth doesn't count as an attachment
                    caps->limits.maxFBAttachments >= GBufferAttachmentCount - 1;

        if (!supported) {
            return false;
        }

        for (size_t i = 0; i < BX_COUNT_OF(gBufferAttachmentFormats); i++) {
            if ((caps->formats[gBufferAttachmentFormats[i]] &
                 BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER) == 0)
                return false;
        }

        return true;

    } else {
        return supported;
    }
}

static inline void set_view_projection(bgfx_view_id_t view_id, Camera *camera, int32_t width,
                                       int32_t height) {
    // Submits final view
    mat4 view;
    mat4 rotation_mat;
    mat4 translation_mat = GLM_MAT4_IDENTITY_INIT;
    vec3 negative_pos;

    glm_vec3_negate_to(camera->position, negative_pos);
    glm_quat_mat4(camera->rotation, rotation_mat);
    glm_translate(translation_mat, negative_pos);

    glm_mat4_mul(rotation_mat, translation_mat, view);

    mat4 proj;
    glm_perspective(glm_rad(camera->fov), (float)width / (float)height, camera->near, camera->far,
                    proj);
    bgfx_set_view_transform(view_id, view, proj);

    glm_mat4_copy(view, camera->view);
    glm_mat4_copy(proj, camera->proj);
}

#endif