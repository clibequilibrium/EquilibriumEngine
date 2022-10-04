#ifndef PBR_SYSTEM_H
#define PBR_SYSTEM_H

#include "world.h"
#include "components/renderer/renderer_components.h"
#include "components/scene/scene_components.h"

EQUILIBRIUM_API
void PBRSystemImport(world_t *world);

EQUILIBRIUM_API
uint64_t bind_material(PBRShader *pbr_shader, Material *material);

#endif