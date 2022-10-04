#ifndef BASE_RENDERING_SYSTEM_H
#define BASE_RENDERING_SYSTEM_H

#include "world.h"
#include "cglm_components.h"
#include "components/renderer/renderer_components.h"

typedef struct PosVertex {
    float x;
    float y;
    float z;
} PosVertex;

EQUILIBRIUM_API
bgfx_texture_format_t find_depth_format(uint64_t textureFlags, bool stencil);

EQUILIBRIUM_API void BaseRenderingSystemImport(world_t *world);

CGLM_INLINE
void glm_mat3_adjugate(mat3 mat, mat3 dest) {
    float det;
    float a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[1][0], e = mat[1][1], f = mat[1][2],
          g = mat[2][0], h = mat[2][1], i = mat[2][2];

    dest[0][0] = e * i - f * h;
    dest[0][1] = -(b * i - h * c);
    dest[0][2] = b * f - e * c;
    dest[1][0] = -(d * i - g * f);
    dest[1][1] = a * i - c * g;
    dest[1][2] = -(a * f - d * c);
    dest[2][0] = d * h - g * e;
    dest[2][1] = -(a * h - g * b);
    dest[2][2] = a * e - b * d;

    // det = 1.0f / (a * dest[0][0] + b * dest[1][0] + c * dest[2][0]);

    // glm_mat3_scale(dest, det);
}

static void set_normal_matrix(FrameData *frame_data, mat4 model_matrix) {
    // usually the normal matrix is based on the model view matrix
    // but shading is done in world space (not eye space) so it's just the model
    // matrix glm::mat4 modelViewMat = viewMat * modelMat;

    // if we don't do non-uniform scaling, the normal matrix is the same as the
    // model-view matrix (only the magnitude of the normal is changed, but we
    // normalize either way) glm::mat3 normalMat = glm::mat3(modelMat);

    // use adjugate instead of inverse
    // see
    // https://github.com/graphitemaster/normals_revisited#the-details-of-transforming-normals
    // cofactor is the transpose of the adjugate

    mat3 temp;
    mat3 normal;

    glm_mat4_pick3(model_matrix, temp);
    glm_mat3_adjugate(temp, normal);
    glm_mat3_transpose(normal);

    //   mat3 normal_matrix =
    //   glm::transpose(glm::adjugate(glm::mat3(modelMat)));
    bgfx_set_uniform(frame_data->normal_matrix_uniform, &normal[0], UINT16_MAX);
}

#endif