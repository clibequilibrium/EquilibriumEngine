#include <systems/imgui/cimgui_base.h>
#include "imgui_dockspace_system.h"

static void DrawDockSpace(ecs_iter_t *it) {
    static bool               open            = true;
    static bool              *p_open          = &open;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
    // window not dockable into, because it would be confusing to have two
    // docking targets within each others.
    ImGuiWindowFlags     window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport *viewport     = igGetMainViewport();

    igSetNextWindowPos(viewport->WorkPos, 0, (ImVec2){});
    igSetNextWindowSize(viewport->WorkSize, 0);
    igSetNextWindowViewport(viewport->ID);

    igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);

    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will
    // render our background and handle the pass-thru hole, so we ask Begin() to
    // not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window
    // is collapsed). This is because we want to keep our DockSpace() active. If
    // a DockSpace() is inactive, all active windows docked into it will lose
    // their parent and become undocked. We cannot preserve the docking
    // relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in
    // limbo and never being visible.
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0.0f, 0.0f});

    // ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 7.5f));

    igBegin("DockSpace Demo", p_open, window_flags);
    igPopStyleVar(1);
    igPopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO *io = igGetIO();
    if (io->ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = igGetID_Str("MyDockSpace");
        igDockSpace(dockspace_id, (ImVec2){0.0f, 0.0f}, dockspace_flags, NULL);

        static bool first_time = true;
        if (first_time) {
            first_time = false;

            igDockBuilderRemoveNode(dockspace_id); // clear any previous layout
            igDockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
            igDockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID dock_id_left =
                igDockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, NULL, &dockspace_id);
            ImGuiID dock_id_down =
                igDockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, NULL, &dockspace_id);
            ImGuiID dock_id_up =
                igDockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.55f, NULL, &dockspace_id);

            ImGuiID right;
            ImGuiID left;

            igDockBuilderSplitNode(dock_id_up, ImGuiDir_Right, 0.55f, &right, &left);

            // we now dock our windows into the docking node we made above
            // igDockBuilderDockWindow("Entity Inspector", dock_id_left);
            // ImGui::DockBuilderDockWindow("Debug Log", right);

            // ImGui::DockBuilderDockWindow("Left", dock_id_left);
            igDockBuilderFinish(dockspace_id);
        }
    }

    if (igBeginMenuBar()) {
        if (igBeginMenu("File", true)) {
            if (igMenuItem_Bool("Exit", NULL, false, p_open != NULL))
                world_destroy(it->world);

            igEndMenu();
        }

        ImVec2 region;
        igGetWindowContentRegionMax(&region);

        // Center offset
        igSameLine((region.x / 2.0f) - (1.5f * (igGetFontSize() + igGetStyle()->ItemSpacing.x)), 0);

        entity_t entity = (entity_t){it->entities[0], it->world};
        igText("%s | %s", entity_get_name(entity),
               ecs_get(it->world, it->entities[0], Renderer)->type_name);

        igSameLine(region.x - 175.0f, 0);
        igText("%.2f FPS (%.2f ms)", igGetIO()->Framerate, 1000.0f / igGetIO()->Framerate);

        float button_size = 25.0f;
        igSameLine(region.x - button_size, 0);

        if (igButton("X", (ImVec2){button_size, 0.0f}))
            world_destroy(it->world);

        igEndMenuBar();
    }

    igEnd();
}

void ImguiDockspaceSystemImport(world_t *world) {
    ECS_MODULE(world, ImguiDockspaceSystem);
    ECS_IMPORT(world, GuiComponents);

    entity_create(world, "DockspaceSystem", GuiSystem, {DrawDockSpace});
}