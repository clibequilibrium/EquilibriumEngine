#include <systems/imgui/cimgui_base.h>
#include "imgui_entity_inspector.h"
#include <cr.h>

static entity_t CR_STATE current_entity;

void entity_button_draw(entity_t entity) {
    igPushID_Int(entity.handle);

    world_t *world = entity.world;

    if (entity_valid(entity)) {

        const char *name = entity_get_name(entity);

        if (name) {
            if (igButton(name, (ImVec2){0, 0})) {
                current_entity = entity;
            }
        }

    } else {
        igText("%s", "Invalid Entity");
    }

    igPopID();
}

static void render_entity_list(world_t *world) {
    ecs_filter_t *f = ecs_filter_init(world, &(ecs_filter_desc_t){.terms = {{ecs_id(Entity)}}});
    ecs_iter_t    iterator = ecs_filter_iter(world, f);

    static int count = 0;
    igText("Entity count: %d", count);
    count = 0;

    while (ecs_filter_next(&iterator)) {
        for (int i = 0; i < iterator.count; i++) {
            entity_button_draw((entity_t){iterator.entities[i], iterator.world});
            count++;
        }
    }
}

static void render_editor(world_t *world) {

    // igTextUnformatted("Editing: ", NULL);
    // igSameLine(0, 0);

    igPushID_Int(current_entity.handle);

    if (entity_valid(current_entity)) {

        const char *name = entity_get_name(current_entity);
        if (name) {
            igText("%s | ID: %d", name, current_entity.handle);
        } else {
            igText("ID: %d", current_entity.handle);
        }

    } else {
        igText("Please select entity");
    }

    igPopID();

    if (igButton("New", (ImVec2){0, 0})) {
        current_entity = entity_create_empty(world, NULL);
    }
    if (entity_valid(current_entity)) {
        igSameLine(0, 0);

        // clone would go here
        // if (ImGui::Button("Clone")) {
        // auto old_e = e;
        // e = registry.create();
        //}

        igDummy((ImVec2){10, 0}); // space destroy a bit, to not accidentally click it
        igSameLine(0, 0);

        // red button
        igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.65f, 0.15f, 0.15f, 1.f});
        igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.8f, 0.3f, 0.3f, 1.f});
        igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){1.f, 0.2f, 0.2f, 1.f});
        if (igButton("Destroy", (ImVec2){0, 0})) {
            entity_destroy(current_entity);
        }
        igPopStyleColor(3);
    }

    igSeparator();

    // Render components
    if (entity_valid(current_entity)) {
        igPushID_Int(current_entity.handle);

        // First get the entity's type, which is a vector of (component) ids.
        const ecs_type_t *type = ecs_get_type(current_entity.world, current_entity.handle);

        // 2. To print individual ids, iterate the type array with ecs_id_str
        ecs_id_t *type_ids = type->array;
        int32_t   i, count = type->count;

        for (i = 0; i < count; i++) {
            ecs_id_t id = type_ids[i];

            igPushID_Int(id);

            if (igButton("-", (ImVec2){0, 0})) {
                entity_remove_component_id(current_entity, id);
                igPopID();
                continue; // early out to prevent access to deleted data
            } else {
                igSameLine(0, 0);
            }

            char *id_str = ecs_id_str(current_entity.world, id);
            if (igCollapsingHeader_TreeNodeFlags(id_str, 0)) {
                igIndent(30.f);
                igPushID_Str("Widget");
                // draw widget here
                igPopID();
                igUnindent(30.f);
            }

            igPopID();
            // char *id_str = ecs_id_str(current_entity.world, id);
            // printf("%d: %s\n", i, id_str);
            ecs_os_free(id_str);
        }

        // for (auto &[component_type_id, ci] :
        // inspector.Value.GetComponentMap()) {
        //   if (inspector.Value.entityHasComponent(registry, e,
        //   component_type_id)) {
        //     ImGui::PushID(component_type_id);
        //     if (ImGui::Button("-")) {
        //       ci.destroy(registry, e);
        //       ImGui::PopID();
        //       continue; // early out to prevent access to deleted data
        //     } else {
        //       ImGui::SameLine();
        //     }

        //     if (ImGui::CollapsingHeader(ci.name.c_str())) {
        //       ImGui::Indent(30.f);
        //       ImGui::PushID("Widget");
        //       ci.widget(registry, e);
        //       ImGui::PopID();
        //       ImGui::Unindent(30.f);
        //     }
        //     ImGui::PopID();
        //   } else {
        //     has_not[component_type_id] = ci;
        //   }
        // }

        // if (!has_not.empty()) {
        //   if (ImGui::Button("+ Add Component")) {
        //     ImGui::OpenPopup("Add Component");
        //   }

        //   if (ImGui::BeginPopup("Add Component")) {
        //     ImGui::TextUnformatted("Available:");
        //     ImGui::Separator();

        //     for (auto &[component_type_id, ci] : has_not) {
        //       ImGui::PushID(component_type_id);
        //       if (ImGui::Selectable(ci.name.c_str())) {
        //         ci.create(registry, e);
        //       }
        //       ImGui::PopID();
        //     }
        //     ImGui::EndPopup();
        //   }
        // }
        igPopID();
    }
}

static void DrawEntityInspector(ecs_iter_t *it) {

    bool show_window = true;

    if (show_window) {
        igSetNextWindowSize((ImVec2){550, 400}, ImGuiCond_FirstUseEver);
        if (igBegin("Entity Inspector", &show_window, 0)) {
            if (igBeginChild_Str("list", (ImVec2){300, 0}, true, 0)) {
                render_entity_list(it->real_world);
            }
            igEndChild();
            igSameLine(0, 0);

            if (igBeginChild_Str("editor", (ImVec2){}, false, 0)) {
                render_editor(it->real_world);
            }
            igEndChild();
        }
        igEnd();
    }
}

void ImguiEntityInspectorImport(world_t *world) {
    ECS_MODULE(world, ImguiEntityInspector);
    ECS_IMPORT(world, GuiComponents);

    entity_create(world, "EntityInspector", GuiSystem, {DrawEntityInspector});
}