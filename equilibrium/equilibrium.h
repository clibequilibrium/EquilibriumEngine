#ifndef EQUILIBRIUM_H
#define EQUILIBRIUM_H

#include "engine.h"

#include "components/gui.h"
#include "components/input.h"
#include "components/sdl_window.h"
#include "components/cglm_components.h"
#include "components/transform.h"
#include "components/hot_reloadable_module.h"

#include "systems/sdl_system.h"
#include "systems/bgfx_system.h"
#include "systems/transform_system.h"

#include "systems/rendering/forward_renderer_system.h"
#include "systems/rendering/deferred_renderer_system.h"
#include "systems/scene/camera_system.h"
#include "systems/sky_system/sky_system.h"
#include "systems/rendering/gfx_resource_system.h"

#include "systems/imgui/imgui_bgfx_sdl_system.h"
#include "utils/bgfx_utils.h"
#include "utils/assimp_utils.h"
#include "utils/cgltf_utils.h"

#include <cr.h>

#endif
