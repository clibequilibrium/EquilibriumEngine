#ifndef CGLM_COMPONENTS_H
#define CGLM_COMPONENTS_H

#define CGLM_FORCE_LEFT_HANDED
#include <cglm/cglm.h>
#include "base.h"

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(vec3);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(versor);

EQUILIBRIUM_API
void CglmComponentsImport(world_t *world);

#endif