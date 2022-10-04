
#include "components/scene/scene_components.h"
#include "components/gui.h"
#include "components/input.h"

#include "camera_system.h"
#include "utils/bgfx_utils.h"
#include "bgfx_components.h"
#include <SDL.h>

static float MIN_FOV = 10.0f;
static float MAX_FOV = 120.0f;

static void InitializeCamera(ecs_iter_t *it) {
    Camera *camera = ecs_field(it, Camera, 2);

    static vec3 up = GLM_YUP;

    for (int i = 0; i < it->count; i++) {
        glm_vec3_copy(GLM_YUP, camera[i].up);

        camera[i].position[0] = -35.0f;
        camera[i].position[1] = 15.0f;
        camera[i].position[2] = -15.0f;

        vec3 target = {0, 0, 1};
        vec3 forward;

        // model rotation
        // maps vectors to camera space (x, y, z)
        glm_vec3_sub(target, camera[i].position, forward);
        glm_vec3_normalize(forward);

        glm_quat_from_vecs(forward, GLM_ZUP, camera[i].rotation);

        // correct the up vector
        // the cross product of non-orthogonal vectors is not normalized
        vec3 right;
        glm_vec3_normalize(up);
        glm_vec3_crossn(up, forward, right); // left-handed coordinate system

        vec3 orth_up;
        glm_vec3_cross(forward, right, orth_up);

        versor up_rotation;
        versor temp_rotation;

        vec3 temp;

        glm_quat_rotatev(camera[i].rotation, orth_up, temp);

        glm_normalize(temp);
        glm_quat_from_vecs(temp, GLM_YUP, up_rotation);

        glm_quat_mul(up_rotation, camera[i].rotation, camera[i].rotation);

        // inverse of the model rotation
        glm_quat_conjugate(camera[i].rotation, camera[i].inverse_rotation);
    }
}

static void UpdateCamera(ecs_iter_t *it) {
    AppWindow *app_window = ecs_field(it, AppWindow, 1);
    Camera    *camera     = ecs_field(it, Camera, 2);
    Input     *input      = ecs_field(it, Input, 3);

    for (int i = 0; i < it->count; i++) {

        const float angularVelocity = 180.0f / 600.0f; // degrees/pixel

        // Zoom
        float scroll_y = input->mouse.scroll.y;
        if (scroll_y != 0) {
            camera[i].fov -= scroll_y * 2.0f;
            camera[i].fov = glm_clamp(camera[i].fov, MIN_FOV, MAX_FOV);
        }

        // Preparing axis
        const float velocity = input->keys[ECS_KEY_SHIFT].state ? 20.0f : 10.0f;
        vec3        delta;
        vec3        forward;
        vec3        right;
        vec3        up;

        glm_quat_rotatev(camera[i].inverse_rotation, GLM_ZUP, forward);
        glm_quat_rotatev(camera[i].inverse_rotation, GLM_XUP, right);
        glm_quat_rotatev(camera[i].inverse_rotation, GLM_YUP, up);

        vec2 mouse_delta = {input->mouse.rel.y, input->mouse.rel.x};

        if (input->mouse.right.state) {

            if (SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE)
                SDL_ShowCursor(SDL_DISABLE);

            // Rotation
            glm_vec2_negate(mouse_delta);
            glm_vec2_scale(mouse_delta, angularVelocity, mouse_delta);

            mouse_delta[0] = glm_rad(mouse_delta[0]);
            mouse_delta[1] = glm_rad(mouse_delta[1]);

            float dot = glm_dot(camera[i].up, forward);

            // limit pitch
            if ((dot < -0.99f && mouse_delta[0] < 0.0f) || // angle nearing 180 degrees
                (dot > 0.99f && mouse_delta[0] > 0.0f))    // angle nearing 0 degrees
                mouse_delta[0] = 0.0f;

            // pitch is relative to current sideways rotation
            // yaw happens independently
            // this prevents roll
            versor pitch             = GLM_QUAT_IDENTITY_INIT;
            versor yaw               = GLM_QUAT_IDENTITY_INIT;
            versor temp_pith_yaw_mul = GLM_QUAT_IDENTITY_INIT;

            glm_quatv(pitch, mouse_delta[0], GLM_XUP);
            glm_quatv(yaw, mouse_delta[1], GLM_YUP);

            glm_quat_mul(pitch, camera[i].rotation, temp_pith_yaw_mul);
            glm_quat_mul(temp_pith_yaw_mul, yaw, camera[i].rotation);

            glm_quat_conjugate(camera[i].rotation, camera[i].inverse_rotation);
        } else {
            if (SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE) {
                SDL_WarpMouseGlobal(app_window->width / 2.f, app_window->height / 2.f);
                SDL_ShowCursor(SDL_ENABLE);
            }
        }

        // Translation
        if (input->keys[ECS_KEY_W].state) {
            glm_vec3_scale(forward, velocity * it->delta_time, delta);
            glm_vec3_add(delta, camera[i].position, camera[i].position);
        }

        if (input->keys[ECS_KEY_S].state) {

            vec3 backward;
            glm_vec3_negate_to(forward, backward);

            glm_vec3_scale(backward, velocity * it->delta_time, delta);
            glm_vec3_add(delta, camera[i].position, camera[i].position);
        }

        if (input->keys[ECS_KEY_A].state) {
            vec3 left;
            glm_vec3_negate_to(right, left);

            glm_vec3_scale(left, velocity * it->delta_time, delta);
            glm_vec3_add(delta, camera[i].position, camera[i].position);
        }

        if (input->keys[ECS_KEY_D].state) {
            glm_vec3_scale(right, velocity * it->delta_time, delta);
            glm_vec3_add(delta, camera[i].position, camera[i].position);
        }

        if (input->keys[ECS_KEY_SPACE].state) {
            glm_vec3_scale(up, velocity * it->delta_time, delta);
            glm_vec3_add(delta, camera[i].position, camera[i].position);
        }

        if (input->keys[ECS_KEY_Q].state) {
            vec3 down;
            glm_vec3_negate_to(up, down);

            glm_vec3_scale(down, velocity * it->delta_time, delta);
            glm_vec3_add(delta, camera[i].position, camera[i].position);
        }
    }
}

void CameraSystemImport(world_t *world) {
    ECS_TAG(world, OnInput);

    ECS_MODULE(world, CameraSystem);
    ECS_IMPORT(world, SceneComponents);
    ECS_IMPORT(world, GuiComponents);
    ECS_IMPORT(world, InputComponents);

    ECS_OBSERVER(world, InitializeCamera, EcsOnSet, [in] gui.components.AppWindow,
                 scene.components.Camera);

    ECS_SYSTEM(world, UpdateCamera, OnInput, [in] gui.components.AppWindow, scene.components.Camera,
               input.components.Input($));
}