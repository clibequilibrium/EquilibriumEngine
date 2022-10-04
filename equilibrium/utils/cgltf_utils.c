#include "cgltf_utils.h"
#include "bgfx/3rdparty/cgltf/cgltf.h"
#include "bgfx/c99/bgfx.h"
#include "bgfx/defines.h"
#include "bgfx_utils_wrapper.h"
#include "bgfx_utils.h"
#include "components/scene/scene_components.h"
#include "components/cglm_components.h"
#include "flecs.h"
#include "inttypes.h"
#include "stdbool.h"
#include "utils/bgfx_utils.h"
#include "utils/bgfx_utils_wrapper.h"
#include <stddef.h>
#include <stdint.h>
#include <mikktspace.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define MAXLEN 1024

typedef struct MaterialWrapper {
    cgltf_material *ptr;
    Material        mat;
} MaterialWrapper;

typedef struct VertexData {
    size_t        index_stride;
    void         *p_indices;
    bgfx_memory_t data;
    size_t        numFaces;
} VertexData;

typedef struct PosNormalTangentTexcoordVertex {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
} PosNormalTangentTexcoordVertex;

// TODO : redo
static MaterialWrapper materials[1024];

// static ecs_hashmap_t materials = flecs_hashmap_init(hm, K, V, compare, hash);

static int getNumFaces(const SMikkTSpaceContext *ctx) {
    const VertexData *vertexData = (VertexData *)(ctx->m_pUserData);
    return vertexData->numFaces;
};

static int getNumVerticesOfFace(const SMikkTSpaceContext *ctx, const int count) { return 3; };

static void getNormal(const SMikkTSpaceContext *ctx, float normals[], const int iface,
                      const int ivert) {
    int               i          = iface * 3 + ivert;
    const VertexData *vertexData = (VertexData *)(ctx->m_pUserData);

    if (vertexData->index_stride == sizeof(uint16_t)) {
        uint16_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        normals[0] = vertex->normal[0];
        normals[1] = vertex->normal[1];
        normals[2] = vertex->normal[2];
    } else {
        uint32_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        normals[0] = vertex->normal[0];
        normals[1] = vertex->normal[1];
        normals[2] = vertex->normal[2];
    }
};

static void getTexCoord(const SMikkTSpaceContext *ctx, float texCoordOut[], const int iface,
                        const int ivert) {
    int               i          = iface * 3 + ivert;
    const VertexData *vertexData = (VertexData *)(ctx->m_pUserData);

    if (vertexData->index_stride == sizeof(uint16_t)) {
        uint16_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        texCoordOut[0] = vertex->uv[0];
        texCoordOut[1] = vertex->uv[1];
    } else {
        uint32_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        texCoordOut[0] = vertex->uv[0];
        texCoordOut[1] = vertex->uv[1];
    }
};

static void getPosition(const SMikkTSpaceContext *ctx, float positions[], const int iface,
                        const int ivert) {

    int               i          = iface * 3 + ivert;
    const VertexData *vertexData = (VertexData *)(ctx->m_pUserData);

    if (vertexData->index_stride == sizeof(uint16_t)) {
        uint16_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        positions[0] = vertex->position[0];
        positions[1] = vertex->position[1];
        positions[2] = vertex->position[2];

    } else {
        uint32_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        positions[0] = vertex->position[0];
        positions[1] = vertex->position[1];
        positions[2] = vertex->position[2];
    }
};

static void setTSpaceBasic(const SMikkTSpaceContext *ctx, const float tangent[], const float sign,
                           const int iface, const int ivert) {
    int               i          = iface * 3 + ivert;
    const VertexData *vertexData = (VertexData *)(ctx->m_pUserData);

    // Invert the sign because of glTF convention.
    vec4 t = (vec4){tangent[0], tangent[1], tangent[2], -sign};

    if (vertexData->index_stride == sizeof(uint16_t)) {
        uint16_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        vertex->tangent[0] = t[0];
        vertex->tangent[1] = t[1];
        vertex->tangent[2] = t[2];

    } else {
        uint32_t index = *((uint16_t *)vertexData->p_indices + i);

        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexData->data.data +
                                               (index * sizeof(PosNormalTangentTexcoordVertex)));

        vertex->tangent[0] = t[0];
        vertex->tangent[1] = t[1];
        vertex->tangent[2] = t[2];
    }
};

static void calc_tangets(VertexData *vertexData) {
    SMikkTSpaceInterface iface;
    iface.m_getNumFaces          = getNumFaces;
    iface.m_getNumVerticesOfFace = getNumVerticesOfFace;
    iface.m_getPosition          = getPosition;
    iface.m_getNormal            = getNormal;
    iface.m_getTexCoord          = getTexCoord;
    iface.m_setTSpaceBasic       = setTSpaceBasic;
    iface.m_setTSpace            = NULL;

    SMikkTSpaceContext context = (SMikkTSpaceContext){
        &iface,
        (void *)vertexData,
    };
    genTangSpace(&context, 45);
}

static void path_append(char *lhs, const char *base, char *rhs) {
    ecs_os_strcpy(lhs, base);
    ecs_os_strcat(lhs, rhs);
}

static int gltf_texture_index(const cgltf_data *gltf, const cgltf_texture *tex) {
    assert(tex);
    return (int)(tex - gltf->textures);
}

// static inline void process_transform(cgltf_node *node, mat4 parent, mat4
// dest) {
//   mat4 local = GLM_MAT4_IDENTITY_INIT;

//   if (node->has_scale) {
//     glm_scale(local, (vec3){node->scale[0], node->scale[1], node->scale[2]});
//   }

//   if (node->has_rotation) {
//     versor rotation;
//     glm_quat_init(rotation, node->rotation[0], node->rotation[1],
//     node->rotation[2],
//                   node->rotation[3]);

//     mat4 temp;
//     glm_quat_mat4(rotation, temp);

//     glm_mat4_mul(temp, local, local);
//   }

//   if (node->has_translation) {
//     glm_translate(local, (vec3){node->translation[0], node->translation[1],
//     node->translation[2]});
//   }

//   glm_mat4_mul(parent, local, dest);
// }

static Material material_load(world_t *world, const cgltf_data *data,
                              const cgltf_material *material, const char *dir) {
    Material out;

    out.base_color_texture         = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.normal_texture             = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.emissive_texture           = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.occlusion_texture          = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.metallic_roughness_texture = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;

    out.blend        = material->alpha_mode != cgltf_alpha_mode_opaque;
    out.double_sided = material->double_sided;
    glm_vec4_copy(GLM_VEC4_ONE, out.base_color_factor);
    glm_vec3_copy(GLM_VEC3_ZERO, out.emissive_factor);

    char texture_path[1024];

    if (material->has_pbr_metallic_roughness) {

        // Base color
        cgltf_texture *texture = material->pbr_metallic_roughness.base_color_texture.texture;

        if (texture != NULL) {
            path_append(texture_path, dir,
                        data->textures[gltf_texture_index(data, texture)].image->uri);
            out.base_color_texture = load_texture(world, texture_path);

            glm_vec4_copy(*(vec4 *)material->pbr_metallic_roughness.base_color_factor,
                          out.base_color_factor);

            glm_vec4_clamp(out.base_color_factor, 0.0f, 1.0f);
        }

        // Metallic/roughness

        texture = material->pbr_metallic_roughness.metallic_roughness_texture.texture;

        if (texture != NULL) {
            path_append(texture_path, dir,
                        data->textures[gltf_texture_index(data, texture)].image->uri);
            out.metallic_roughness_texture = load_texture(world, texture_path);
        }

        out.metallic_factor  = material->pbr_metallic_roughness.metallic_factor;
        out.roughness_factor = material->pbr_metallic_roughness.roughness_factor;

        // Normal map

        texture = material->normal_texture.texture;

        if (texture != NULL) {
            path_append(texture_path, dir,
                        data->textures[gltf_texture_index(data, texture)].image->uri);
            out.normal_texture = load_texture(world, texture_path);
            out.normal_scale   = material->normal_texture.scale;
        }

        // Occlusion texture

        // Some GLTF files combine metallic/roughness and occlusion values into
        // one texture don't load it twice
        if (material->occlusion_texture.texture ==
            material->pbr_metallic_roughness.metallic_roughness_texture.texture) {
            out.occlusion_texture = out.metallic_roughness_texture;
        } else {
            texture = material->occlusion_texture.texture;

            if (texture != NULL) {
                path_append(texture_path, dir,
                            data->textures[gltf_texture_index(data, texture)].image->uri);
                out.occlusion_texture  = load_texture(world, texture_path);
                out.occlusion_strength = glm_clamp(material->occlusion_texture.scale, 0.0f, 1.0f);
            }
        }

        // emissive texture

        texture = material->emissive_texture.texture;

        if (texture != NULL) {
            path_append(texture_path, dir,
                        data->textures[gltf_texture_index(data, texture)].image->uri);
            out.emissive_texture = load_texture(world, texture_path);

            glm_vec3_copy(*(vec3 *)material->emissive_factor, out.emissive_factor);
            glm_vec3_clamp(out.emissive_factor, 0.0f, 1.0f);
        }

    } else {
        ecs_err("Unhandled PBR type");
    }

    return out;
}

static Group group_load(world_t *world, const cgltf_data *data, const cgltf_primitive *primitive,
                        float *node_to_world, float *node_to_world_normal) {
    Group       result;
    VertexData *vertexData = ecs_os_malloc((sizeof(VertexData)));
    // indices

    if (primitive->indices != 0) {

        cgltf_accessor *indices_accessor = primitive->indices;

        if (indices_accessor->type != cgltf_type_scalar) {
            ecs_err("Don't know how to handle non scalar indices");
        }

        const int index_stride = indices_accessor->component_type == cgltf_component_type_r_16u
                                     ? sizeof(uint16_t)
                                     : sizeof(uint32_t);

        vertexData->index_stride = index_stride;
        vertexData->numFaces     = primitive->indices->count / 3u;
        vertexData->p_indices    = ecs_os_malloc(primitive->indices->count * index_stride);

        const bgfx_memory_t *index_memory = bgfx_alloc(primitive->indices->count * index_stride);

        for (int i = 0; i < primitive->indices->count; i++) {
            if (index_stride == sizeof(uint16_t)) {
                uint16_t *index_value = (uint16_t *)(index_memory->data + (i * sizeof(uint16_t)));
                *index_value          = cgltf_accessor_read_index(primitive->indices, i);
            } else {
                uint32_t *index_value = (uint32_t *)(index_memory->data + (i * sizeof(uint32_t)));
                *index_value          = cgltf_accessor_read_index(primitive->indices, i);
            }
        }

        // TODO change this
        for (int i = 0; i < primitive->indices->count; i++) {
            if (index_stride == sizeof(uint16_t)) {
                uint16_t *index_value =
                    (uint16_t *)(vertexData->p_indices + (i * sizeof(uint16_t)));
                *index_value = cgltf_accessor_read_index(primitive->indices, i);
            } else {
                uint32_t *index_value =
                    (uint32_t *)(vertexData->p_indices + (i * sizeof(uint32_t)));
                *index_value = cgltf_accessor_read_index(primitive->indices, i);
            }
        }

        result.num_indices  = primitive->indices->count;
        result.index_buffer = create_index_buffer(
            world, index_memory, index_stride == 2 ? BGFX_BUFFER_NONE : BGFX_BUFFER_INDEX32);
    }

    // vertices
    bgfx_vertex_layout_t pcvDecl;

    bgfx_vertex_layout_begin(&pcvDecl, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_NORMAL, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_TANGENT, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false,
                           false);
    bgfx_vertex_layout_end(&pcvDecl);

    uint32_t             stride = pcvDecl.stride;
    const bgfx_memory_t *vertex_memory;

    for (int i = 0; i < primitive->attributes_count; i++) {
        cgltf_attribute *attribute = &primitive->attributes[i];

        if (attribute->type == cgltf_attribute_type_position) {
            vertex_memory       = bgfx_alloc(attribute->data->count * stride);
            result.num_vertices = attribute->data->count;

            break;
        }
    }

    for (int i = 0; i < result.num_vertices; i++) {
        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertex_memory->data + (i * stride));

        glm_vec3_copy(GLM_VEC3_ZERO, vertex->tangent);
        for (int j = 0; j < primitive->attributes_count; j++) {
            cgltf_attribute *attribute = &primitive->attributes[j];

            if (attribute->type == cgltf_attribute_type_position) {
                cgltf_accessor_read_float(attribute->data, i, vertex->position,
                                          cgltf_num_components(attribute->data->type));

                glm_mat4_mulv3(*(mat4 *)node_to_world, vertex->position, 1.0f, vertex->position);
            } else if (attribute->type == cgltf_attribute_type_normal) {
                cgltf_accessor_read_float(attribute->data, i, vertex->normal,
                                          cgltf_num_components(attribute->data->type));
                glm_mat4_mulv3(*(mat4 *)node_to_world_normal, vertex->normal, 1.0f, vertex->normal);

            } else if (attribute->type == cgltf_attribute_type_tangent) {
                cgltf_accessor_read_float(attribute->data, i, vertex->tangent,
                                          cgltf_num_components(attribute->data->type));
            } else if (attribute->type == cgltf_attribute_type_texcoord) {
                cgltf_accessor_read_float(attribute->data, i, vertex->uv,
                                          cgltf_num_components(attribute->data->type));
            }
        }
    }

    /// glTF is a right-handed coordinate system, where the 'right' direction is
    /// -X relative to engine's coordinate system. glTF matrix: column vectors,
    /// column-major storage, +Y up, +Z forward, -X right, right-handed
    /// Equilibrium matrix: column vectors, column-major storage, +Y up, +Z
    /// forward, +X right, left-handed multiply by a negative X scale to convert
    /// handedness
    for (int i = 0; i < result.num_vertices; i++) {
        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertex_memory->data + (i * stride));

        vertex->position[0] = -vertex->position[0];
        vertex->normal[0]   = -vertex->normal[0];
        vertex->tangent[0]  = -vertex->tangent[0];
    }

    vertexData->data = (bgfx_memory_t){vertex_memory->data, vertex_memory->size};
    // calc_tangets(vertexData);

    ecs_os_free(vertexData->p_indices);
    ecs_os_free(vertexData);

    result.vertex_buffer = create_vertex_buffer(world, vertex_memory, &pcvDecl, BGFX_BUFFER_NONE);

    return result;
}

// static void process_node(cgltf_node *node, mat4 parent_transform, world_t
// *world,
//                          cgltf_data *data) {

static void process_node(cgltf_node *node, world_t *world, cgltf_data *data) {

    cgltf_mesh *cgltf_mesh = node->mesh;
    // mat4        transform;
    // process_transform(node, parent_transform, transform);

    if (!cgltf_mesh) {
        for (size_t i = 0; i < node->children_count; i++) {
            process_node(node->children[i], world, data);
        }

        return;
    }

    if (cgltf_mesh->primitives->type != cgltf_primitive_type_triangles) {
        ecs_err("Mesh has incompatible primitive type");
        return;
    }

    float node_to_world[16];
    cgltf_node_transform_world(node, node_to_world);
    float node_to_world_normal[16];
    mtx_cofactor(node_to_world_normal, node_to_world);

    for (cgltf_size primitiveIndex = 0; primitiveIndex < cgltf_mesh->primitives_count;
         ++primitiveIndex) {

        cgltf_primitive *primitive = &cgltf_mesh->primitives[primitiveIndex];

        int  material_index;
        Mesh mesh;
        mesh.groups = ecs_vector_new(Group, 0);

        Group group = group_load(world, data, primitive, node_to_world, node_to_world_normal);
        ecs_os_memcpy(ecs_vector_add(&mesh.groups, Group), &group, sizeof(Group));
        group.index_buffer  = (bgfx_index_buffer_handle_t)BGFX_INVALID_HANDLE;
        group.vertex_buffer = (bgfx_vertex_buffer_handle_t)BGFX_INVALID_HANDLE;
        group.num_vertices  = 0;
        group.vertices      = NULL;
        group.num_indices   = 0;
        group.indices       = NULL;

        entity_t meshEntity = entity_create(world, "", Mesh, {mesh.groups});
        entity_add_component(meshEntity, Position, {0, 0, 0});
        entity_add_component(meshEntity, Rotation, {0, 0, 0});
        entity_add_component(meshEntity, Scale, {1, 1, 1});

        Material material;

        for (int x = 0; x < 1024; x++) {
            if (materials[x].ptr == primitive->material) {
                material = materials[x].mat;
                break;
            }
        }

        Material *ecs_material = entity_get_or_add_component(meshEntity, Material);
        ecs_os_memcpy(ecs_material, &material, sizeof(Material));
    }
}

bool cgltf_model_load(const char *file, world_t *world) {
    cgltf_options options = {0};

    cgltf_data  *data   = NULL;
    cgltf_result result = cgltf_parse_file(&options, file, &data);
    result              = cgltf_load_buffers(&options, data, file);
    result              = cgltf_validate(data);

    if (result == cgltf_result_success) {

        char dir[1024] = "";
        bx_string_copy(dir, (char *)file);

        // Parse materials
        for (size_t i = 0; i < data->materials_count; i++) {
            materials[i] = (MaterialWrapper){&data->materials[i],
                                             material_load(world, data, &data->materials[i], dir)};
        }

        for (size_t i = 0; i < data->nodes_count; i++) {
            cgltf_node *node       = &data->nodes[i];
            cgltf_mesh *cgltf_mesh = node->mesh;

            process_node(node, world, data);
            // process_node(node, GLM_MAT4_IDENTITY, world, data);
        }

        cgltf_free(data);
    }

    return result;
}