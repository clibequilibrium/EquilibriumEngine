#ifndef ASSIMP_UTILS_H
#define ASSIMP_UTILS_H

#include "components/cglm_components.h"
#include "components/scene/scene_components.h"
#include "base.h"
#include <assimp/mesh.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/camera.h>
#include <stdbool.h>
#include <string.h>

typedef struct PosNormalTangentTexcoordVertex {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
} PosNormalTangentTexcoordVertex;

static Material materials[1024];

static void aiString_set(struct aiString *string, const char *str) {

    const ai_int32 len = (ai_uint32)strlen(str);
    if (len > (ai_int32)MAXLEN - 1) {
        return;
    }
    string->length = len;
    ecs_os_memcpy(string->data, str, len);
    string->data[len] = 0;
}

static void aiString_append(struct aiString *string, const char *app) {
    const ai_uint32 len = (ai_uint32)strlen(app);
    if (!len) {
        return;
    }
    if (string->length + len >= MAXLEN) {
        return;
    }

    ecs_os_memcpy(&string->data[string->length], app, len + 1);
    string->length += len;
}

static bool aiString_equal(struct aiString *str1, struct aiString *str2) {
    return (str1->length == str2->length &&
            0 == ecs_os_memcmp(str1->data, str2->data, str1->length));
}

static bool aiString_not_equal(struct aiString *str1, struct aiString *str2) {
    return (str1->length != str2->length ||
            0 != ecs_os_memcmp(str1->data, str2->data, str1->length));
}

static Material material_load(world_t *world, const struct aiMaterial *material, const char *dir) {
    Material out;

    out.base_color_texture         = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.normal_texture             = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.emissive_texture           = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.occlusion_texture          = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    out.metallic_roughness_texture = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;

    // technically there is a difference between MASK and BLEND mode
    // but for our purposes it's enough if we sort properly
    struct aiString alphaMode;
    aiGetMaterialString(material, AI_MATKEY_GLTF_ALPHAMODE, &alphaMode);

    struct aiString alphaModeOpaque;
    char            value[] = "OPAQUE";
    strncpy_s(alphaModeOpaque.data, 1024, value, strlen(value));

    alphaModeOpaque.length = strlen(alphaModeOpaque.data);

    out.blend = aiString_not_equal(&alphaMode, &alphaModeOpaque);

    const struct aiMaterialProperty *property;
    aiGetMaterialProperty(material, AI_MATKEY_TWOSIDED, &property);

    out.double_sided = *(bool *)property->mData;

    // texture files

    struct aiString fileBaseColor, fileMetallicRoughness, fileNormals, fileOcclusion, fileEmissive;
    aiGetMaterialTexture(material, AI_MATKEY_BASE_COLOR_TEXTURE, &fileBaseColor, NULL, NULL, NULL,
                         NULL, NULL, NULL);

    aiGetMaterialTexture(material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE,
                         &fileMetallicRoughness, NULL, NULL, NULL, NULL, NULL, NULL);
    aiGetMaterialTexture(material, aiTextureType_NORMALS, 0, &fileNormals, NULL, NULL, NULL, NULL,
                         NULL, NULL);
    aiGetMaterialTexture(material, aiTextureType_LIGHTMAP, 0, &fileOcclusion, NULL, NULL, NULL,
                         NULL, NULL, NULL);
    aiGetMaterialTexture(material, aiTextureType_EMISSIVE, 0, &fileEmissive, NULL, NULL, NULL, NULL,
                         NULL, NULL);

    // diffuse

    if (fileBaseColor.length > 0) {
        struct aiString pathBaseColor;

        aiString_set(&pathBaseColor, dir);
        aiString_append(&pathBaseColor, fileBaseColor.data);
        out.base_color_texture = load_texture(world, pathBaseColor.data);
    }

    struct aiColor4D baseColorFactor;
    if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &baseColorFactor)) {
        glm_vec4_copy(
            (vec4){baseColorFactor.r, baseColorFactor.g, baseColorFactor.b, baseColorFactor.a},
            out.base_color_factor);
    }

    glm_vec4_clamp(out.base_color_factor, 0.0f, 1.0f);

    // metallic/roughness

    if (fileMetallicRoughness.length > 0) {
        struct aiString pathMetallicRoughness;

        aiString_set(&pathMetallicRoughness, dir);
        aiString_append(&pathMetallicRoughness, fileMetallicRoughness.data);
        out.metallic_roughness_texture = load_texture(world, pathMetallicRoughness.data);
    }

    ai_real metallicFactor;
    if (AI_SUCCESS ==
        aiGetMaterialFloatArray(material, AI_MATKEY_METALLIC_FACTOR, &metallicFactor, NULL)) {

        out.metallic_factor = glm_clamp(metallicFactor, 0.0f, 1.0f);
    }

    ai_real roughnessFactor;
    if (AI_SUCCESS ==
        aiGetMaterialFloatArray(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughnessFactor, NULL)) {
        out.roughness_factor = glm_clamp(roughnessFactor, 0.0f, 1.0f);
    }

    // normal map

    if (fileNormals.length > 0) {
        struct aiString pathNormals;
        aiString_set(&pathNormals, dir);
        aiString_append(&pathNormals, fileNormals.data);

        out.normal_texture = load_texture(world, pathNormals.data);
    }

    ai_real normalScale;
    if (AI_SUCCESS ==
        aiGetMaterialFloatArray(material, AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0),
                                &normalScale, NULL)) {
        out.normal_scale = normalScale;
    }

    // occlusion texture

    if (aiString_equal(&fileOcclusion, &fileMetallicRoughness)) {
        // some GLTF files combine metallic/roughness and occlusion values into
        // one texture don't load it twice
        out.occlusion_texture = out.metallic_roughness_texture;
    } else if (fileOcclusion.length > 0) {
        struct aiString pathOcclusion;
        aiString_set(&pathOcclusion, dir);
        aiString_append(&pathOcclusion, fileOcclusion.data);

        out.occlusion_texture = load_texture(world, pathOcclusion.data);
    } else {
        out.occlusion_texture = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    }

    ai_real occlusionStrength;
    if (AI_SUCCESS == aiGetMaterialFloatArray(
                          material, AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0),
                          &occlusionStrength, NULL)) {
        out.occlusion_strength = glm_clamp(occlusionStrength, 0.0f, 1.0f);
    }

    // emissive texture

    if (fileEmissive.length > 0) {
        struct aiString pathEmissive;
        aiString_set(&pathEmissive, dir);
        aiString_append(&pathEmissive, fileEmissive.data);

        out.emissive_texture = load_texture(world, pathEmissive.data);
    }

// assimp doesn't define this
#ifndef AI_MATKEY_GLTF_EMISSIVE_FACTOR
#define AI_MATKEY_GLTF_EMISSIVE_FACTOR AI_MATKEY_COLOR_EMISSIVE
#endif

    struct aiColor4D emissive_color;

    if (AI_SUCCESS ==
        aiGetMaterialColor(material, AI_MATKEY_GLTF_EMISSIVE_FACTOR, &emissive_color)) {
        out.emissive_factor[0] = emissive_color.r;
        out.emissive_factor[1] = emissive_color.g;
        out.emissive_factor[2] = emissive_color.b;
    }

    glm_vec3_clamp(out.emissive_factor, 0.0f, 1.0f);
    return out;
}

static Group group_load(world_t *world, const struct aiMesh *mesh, int *material_index) {
    Group result;

    if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
        ecs_err("Mesh has incompatible primitive type");

    if (mesh->mNumVertices > (UINT16_MAX + 1u))
        ecs_err("Mesh has too many vertices %d", UINT16_MAX + 1);

    size_t coords     = 0;
    bool   hasTexture = mesh->mNumUVComponents[coords] == 2 && mesh->mTextureCoords[coords] != NULL;

    // vertices
    bgfx_vertex_layout_t pcvDecl;

    bgfx_vertex_layout_begin(&pcvDecl, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_NORMAL, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_TANGENT, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false,
                           false);
    bgfx_vertex_layout_end(&pcvDecl);

    uint32_t stride = pcvDecl.stride;

    const bgfx_memory_t *vertexMem = bgfx_alloc(mesh->mNumVertices * stride);

    for (size_t i = 0; i < mesh->mNumVertices; i++) {
        PosNormalTangentTexcoordVertex *vertex =
            (PosNormalTangentTexcoordVertex *)(vertexMem->data + (i * stride));

        struct aiVector3D *pos = &mesh->mVertices[i];
        vertex->position[0]    = pos->x;
        vertex->position[1]    = pos->y;
        vertex->position[2]    = pos->z;

        // minBounds = glm::min(minBounds, {pos.x, pos.y, pos.z});
        // maxBounds = glm::max(maxBounds, {pos.x, pos.y, pos.z});

        struct aiVector3D *normal = &mesh->mNormals[i];
        vertex->normal[0]         = normal->x;
        vertex->normal[1]         = normal->y;
        vertex->normal[2]         = normal->z;

        if (mesh->mTangents != NULL) {
            struct aiVector3D *tangent = &mesh->mTangents[i];
            vertex->tangent[0]         = tangent->x;
            vertex->tangent[1]         = tangent->y;
            vertex->tangent[2]         = tangent->z;
        } else {
            vertex->tangent[0] = 0;
            vertex->tangent[1] = 0;
            vertex->tangent[2] = 0;
        }

        // ecs_trace("Vertex: %f, %f, %f| Normal: %f, %f, %f, | "
        //           "Tangent: %f, %f, %f",
        //           vertex->position[0], vertex->position[1],
        //           vertex->position[2], vertex->normal[0], vertex->normal[1],
        //           vertex->normal[2], vertex->tangent[0], vertex->tangent[1],
        //           vertex->tangent[2]);

        if (hasTexture) {
            struct aiVector3D uv = mesh->mTextureCoords[coords][i];
            vertex->uv[0]        = uv.x;
            vertex->uv[1]        = uv.y;
        }
    }

    result.vertex_buffer = create_vertex_buffer(world, vertexMem, &pcvDecl, BGFX_BUFFER_NONE);

    const bgfx_memory_t *iMem    = bgfx_alloc(mesh->mNumFaces * 3 * sizeof(uint16_t));
    uint16_t            *indices = (uint16_t *)iMem->data;

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        ecs_assert(mesh->mFaces[i].mNumIndices == 3, ECS_INVALID_COMPONENT_ALIGNMENT, NULL);
        indices[(3 * i) + 0] = (uint16_t)mesh->mFaces[i].mIndices[0];
        indices[(3 * i) + 1] = (uint16_t)mesh->mFaces[i].mIndices[1];
        indices[(3 * i) + 2] = (uint16_t)mesh->mFaces[i].mIndices[2];
    }

    result.index_buffer = create_index_buffer(world, iMem, BGFX_BUFFER_NONE);
    *material_index     = mesh->mMaterialIndex;

    return result;
}

static bool assimp_scene_load(const char *file, world_t *world) {
    struct aiPropertyStore *store = aiCreatePropertyStore();
    // Settings for aiProcess_SortByPType
    // only take triangles or higher (polygons are triangulated during import)
    aiSetImportPropertyInteger(store, AI_CONFIG_PP_SBP_REMOVE,
                               aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    // Settings for aiProcess_SplitLargeMeshes
    // Limit vertices to 65k (we use 16-bit indices)
    aiSetImportPropertyInteger(store, AI_CONFIG_PP_SLM_VERTEX_LIMIT, UINT16_MAX);

    unsigned int flags = aiProcessPreset_TargetRealtime_Quality | // some optimizations and
                                                                  // safety checks
                         aiProcess_OptimizeMeshes |               // minimize number of meshes
                         aiProcess_PreTransformVertices |         // apply node matrices
                         aiProcess_FixInfacingNormals |
                         aiProcess_TransformUVCoords | // apply UV transformations
                         // aiProcess_FlipWindingOrder   | // we cull clock-wise, keep the
                         // default CCW winding order
                         aiProcess_MakeLeftHanded | // we set GLM_FORCE_LEFT_HANDED and use
                                                    // left-handed bx matrix functions
                         aiProcess_FlipUVs; // bimg loads textures with flipped Y (top left is
                                            // 0,0)

    const struct aiScene *scene = aiImportFileExWithProperties(file, flags, NULL, store);

    aiReleasePropertyStore(store);

    // If the import failed, report it
    if (!scene) {
        ecs_err("%s", aiGetErrorString());
        return false;
    }

    // Now we can access the file's contents
    if (!(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {

        char dir[1024] = "";
        bx_string_copy(dir, (char *)file);

        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            materials[i] = material_load(world, scene->mMaterials[i], dir);
        }

        for (size_t i = 0; i < scene->mNumMeshes; i++) {
            int  material_index;
            Mesh mesh;
            mesh.groups = ecs_vector_new(Group, 0);

            Group group = group_load(world, scene->mMeshes[i], &material_index);
            ecs_os_memcpy(ecs_vector_add(&mesh.groups, Group), &group, sizeof(Group));
            group.index_buffer  = (bgfx_index_buffer_handle_t)BGFX_INVALID_HANDLE;
            group.vertex_buffer = (bgfx_vertex_buffer_handle_t)BGFX_INVALID_HANDLE;
            group.num_vertices  = 0;
            group.vertices      = NULL;
            group.num_indices   = 0;
            group.indices       = NULL;

            entity_t meshEntity = entity_create(world, file, Mesh, {mesh.groups});
            entity_add_component(meshEntity, Position, {0, 0, 0});
            entity_add_component(meshEntity, Rotation, {0, 0, 0});
            entity_add_component(meshEntity, Scale, {1, 1, 1});

            Material material = materials[material_index];
            if (BGFX_HANDLE_IS_VALID(material.base_color_texture)) {
                Material *ecs_material = entity_get_or_add_component(meshEntity, Material);
                ecs_os_memcpy(ecs_material, &material, sizeof(Material));

                // ecs_material->emissive_texture =
                // (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
                // glm_vec3_copy(GLM_VEC3_ZERO, ecs_material->emissive_factor);
            }
        }

        // center = minBounds + (maxBounds - minBounds) / 2.0f;
        // glm::vec3 extent = glm::abs(maxBounds - minBounds);
        // diagonal = glm::sqrt(glm::dot(extent, extent));

        // bring opaque meshes to the front so alpha blending works
        // still need depth sorting for scenes with overlapping transparent
        // meshes std::partition(meshes.begin(), meshes.end(),
        //                [this](const Mesh &mesh) { return
        //                !materials[mesh.material].blend; });

        // if (scene->HasCameras()) {
        //   camera = loadCamera(scene->mCameras[0]);
        // } else {
        //   Log->info("No camera");
        //   camera.lookAt(center - glm::vec3(0.0f, 0.0f, diagonal / 2.0f),
        //   center,
        //                 glm::vec3(0.0f, 1.0f, 0.0f));
        //   camera.zFar = diagonal;
        //   camera.zNear = camera.zFar / 50.0f;
        // }

        // loaded = true;
    } else {
        ecs_err("Scene is incomplete or invalid");
        return false;
    }

    // We're done. Release all resources associated with this import
    aiReleaseImport(scene);

    return true;
}

#endif