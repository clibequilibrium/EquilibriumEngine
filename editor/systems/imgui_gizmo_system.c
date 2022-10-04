#include <systems/imgui/cimgui_base.h>
#include "imgui_gizmo_system.h"
#include <components/scene/scene_components.h>
#include <components/gui.h>
#include <components/transform.h>
#include <cr.h>

static ecs_query_t *camera_query;
static ecs_query_t *transform_query;

static CR_STATE OPERATION mCurrentGizmoOperation = ROTATE;
static CR_STATE MODE      mCurrentGizmoMode      = WORLD;

static const float identityMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                         0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

static void EditTransform(Camera *camera, mat4 matrix) {

    bool show_window = true;
    // igBegin("Editor", &show_window, 0);

    if (igIsKeyPressed(ImGuiKey_W, true))
        mCurrentGizmoOperation = TRANSLATE;
    if (igIsKeyPressed(ImGuiKey_E, true))
        mCurrentGizmoOperation = ROTATE;
    if (igIsKeyPressed(ImGuiKey_R, true)) // r Key
        mCurrentGizmoOperation = SCALE;
    if (igRadioButton_Bool("Translate", mCurrentGizmoOperation == TRANSLATE))
        mCurrentGizmoOperation = TRANSLATE;
    igSameLine(0, 0);
    if (igRadioButton_Bool("Rotate", mCurrentGizmoOperation == ROTATE))
        mCurrentGizmoOperation = ROTATE;
    igSameLine(0, 0);
    if (igRadioButton_Bool("Scale", mCurrentGizmoOperation == SCALE))
        mCurrentGizmoOperation = SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];

    ImGuizmo_DecomposeMatrixToComponents((float *)matrix, matrixTranslation, matrixRotation,
                                         matrixScale);
    igInputFloat3("Position", matrixTranslation, "%.3f", 0);
    igInputFloat3("Rotation", matrixRotation, "%.3f", 0);
    igInputFloat3("Scale", matrixScale, "%.3f", 0);
    ImGuizmo_RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale,
                                           (float *)matrix);

    if (mCurrentGizmoOperation != SCALE) {
        if (igRadioButton_Bool("Local", mCurrentGizmoMode == LOCAL))
            mCurrentGizmoMode = LOCAL;
        igSameLine(0, 0);
        if (igRadioButton_Bool("World", mCurrentGizmoMode == WORLD))
            mCurrentGizmoMode = WORLD;
    }
    static bool useSnap = false;
    if (igIsKeyPressed(83, true))
        useSnap = !useSnap;

    igCheckbox("Snap", &useSnap);
    igSameLine(0, 0);
    vec4 snap;
    // switch (mCurrentGizmoOperation) {
    // case TRANSLATE:
    //   snap = config.mSnapTranslation;
    //   ImGui::InputFloat3("Snap", &snap.x);
    //   break;
    // case ROTATE:
    //   snap = config.mSnapRotation;
    //   ImGui::InputFloat("Angle Snap", &snap.x);
    //   break;
    // case SCALE:
    //   snap = config.mSnapScale;
    //   ImGui::InputFloat("Scale Snap", &snap.x);
    //   break;
    // }
    ImGuiIO *io = igGetIO();

    ImGuizmo_SetRect(0, 0, io->DisplaySize.x, io->DisplaySize.y);

    // ImGuizmo_DrawGrid(camera->view, camera->proj, identityMatrix, 100.0f);

    ImGuizmo_Manipulate((float *)camera->view, (float *)camera->proj, mCurrentGizmoOperation,
                        mCurrentGizmoMode, (float *)matrix, NULL, useSnap ? &snap[0] : NULL, NULL,
                        NULL);

    // igEnd();
}

static void DrawGizmo(ecs_iter_t *it) {

    Transform *current_transform;
    ecs_iter_t transform_iterator = ecs_query_iter(it->world, transform_query);
    while (ecs_query_next(&transform_iterator)) {
        Transform *transform = ecs_field(&transform_iterator, Transform, 1);

        for (int j = 0; j < transform_iterator.count; j++) {
            current_transform = &transform[j];
        }
    }

    ecs_iter_t camera_iterator = ecs_query_iter(it->world, camera_query);
    while (ecs_query_next(&camera_iterator)) {
        Camera     *camera       = ecs_field(&camera_iterator, Camera, 1);
        GuiContext *gui_contexts = ecs_field(&camera_iterator, GuiContext, 2);

        for (int j = 0; j < camera_iterator.count; j++) {
            // mat4 mat = GLM_MAT4_IDENTITY_INIT;
            // mat4 mat2 = GLM_MAT4_IDENTITY_INIT;
            // glm_trasnslate(mat, (vec3){5.f, 0, 0});

            // igSetCurrentContext(gui_contexts[j].value);
            // ImGuizmo_SetImGuiContext(gui_contexts[j].value);
            // ImGuizmo_BeginFrame();

            // EditTransform(&camera[j], current_transform->value);
        }
    }
}

void ImguiGizmoSystemImport(world_t *world) {
    ECS_MODULE(world, ImguiGizmoSystem);
    ECS_IMPORT(world, GuiComponents);
    ECS_IMPORT(world, TransformComponents);
    ECS_IMPORT(world, SceneComponents);

    entity_create(world, "ImguiGizmoSystem", GuiSystem, {DrawGizmo});
    camera_query    = ecs_query_new(world, "Camera, GuiContext");
    transform_query = ecs_query_new(world, "Transform");
}