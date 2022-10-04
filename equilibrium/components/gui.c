#include "gui.h"

ECS_COMPONENT_DECLARE(GuiSystem);
ECS_COMPONENT_DECLARE(GuiContext);
ECS_COMPONENT_DECLARE(AppWindow);
ECS_COMPONENT_DECLARE(AppWindowHandle);
ECS_COMPONENT_DECLARE(Renderer);

void GuiComponentsImport(world_t *world) {
    ECS_MODULE(world, GuiComponents);

    ECS_COMPONENT_DEFINE(world, GuiSystem);
    ECS_COMPONENT_DEFINE(world, GuiContext);
    ECS_COMPONENT_DEFINE(world, AppWindow);
    ECS_COMPONENT_DEFINE(world, AppWindowHandle);
    ECS_COMPONENT_DEFINE(world, Renderer);

    ecs_struct_init(world, &(ecs_struct_desc_t){
                               .entity  = ecs_id(Renderer),
                               .members = {{.name = "type", .type = ecs_id(ecs_i32_t)},
                                           {.name = "type_name", .type = ecs_id(ecs_string_t)}}});

    ecs_struct_init(world, &(ecs_struct_desc_t){
                               .entity  = ecs_id(AppWindow),
                               .members = {{.name = "width", .type = ecs_id(ecs_i32_t)},
                                           {.name = "height", .type = ecs_id(ecs_i32_t)},
                                           {.name = "maximized", .type = ecs_id(ecs_bool_t)}}});

    ecs_struct_init(world, &(ecs_struct_desc_t){
                               .entity  = ecs_id(AppWindowHandle),
                               .members = {{.name = "value",
                                            .type = sizeof(void *) == 4 ? ecs_id(ecs_i32_t)
                                                                        : ecs_id(ecs_i64_t)}}});
}