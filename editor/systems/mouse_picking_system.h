#ifndef MOUSE_PICKING_SYSTEM_H
#define MOUSE_PICKING_SYSTEM_H

#include <bgfx_components.h>
#include "editor_base.h"

#define ID_DIM 8

typedef struct MousePickingData {
    bgfx_program_handle_t      shading_program;
    bgfx_program_handle_t      id_program;
    bgfx_uniform_handle_t      tint;
    bgfx_uniform_handle_t      id;
    bgfx_texture_handle_t      picking_rt;
    bgfx_texture_handle_t      picking_rt_depth;
    bgfx_texture_handle_t      blit_texture;
    bgfx_frame_buffer_handle_t picking_frame_buffer;

    uint8_t  blid_data[ID_DIM * ID_DIM * 4]; // Read blit into this
    uint32_t reading;
    uint32_t current_frame;

} MousePickingData;

EDITOR_API extern ECS_COMPONENT_DECLARE(MousePickingData);

EDITOR_API
void MousePickingSystemImport(world_t *world);

#endif