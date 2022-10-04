#include "renderer_components.h"

ECS_COMPONENT_DECLARE(LightShader);
ECS_COMPONENT_DECLARE(ForwardRenderer);
ECS_COMPONENT_DECLARE(DeferredRenderer);
ECS_COMPONENT_DECLARE(PBRShader);
ECS_COMPONENT_DECLARE(FrameData);

void RendererComponentsImport(world_t *world) {
    ECS_MODULE(world, RendererComponents);

    ECS_COMPONENT_DEFINE(world, LightShader);
    ECS_COMPONENT_DEFINE(world, ForwardRenderer);
    ECS_COMPONENT_DEFINE(world, DeferredRenderer);
    ECS_COMPONENT_DEFINE(world, PBRShader);
    ECS_COMPONENT_DEFINE(world, FrameData);
}