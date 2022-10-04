#ifndef GUI_COMPONENTS_H
#define GUI_COMPONENTS_H

#include "base.h"

typedef void (*UpdateCallback)(ecs_iter_t *it);

typedef struct GuiSystem {
    UpdateCallback Update;
} GuiSystem;

typedef struct GuiContext {
    void *value;
} GuiContext;

typedef struct AppWindow {
    int32_t width;
    int32_t height;
    bool    maximized;
} AppWindow;

typedef struct AppWindowHandle {
    void *value;
} AppWindowHandle;

typedef enum RendererType {
    Noop,       //!< No rendering.
    Agc,        //!< AGC
    Direct3D9,  //!< Direct3D 9.0
    Direct3D11, //!< Direct3D 11.0
    Direct3D12, //!< Direct3D 12.0
    Gnm,        //!< GNM
    Metal,      //!< Metal
    Nvn,        //!< NVN
    OpenGLES,   //!< OpenGL ES 2.0+
    OpenGL,     //!< OpenGL 2.1+
    Vulkan,     //!< Vulkan
    WebGPU,     //!< WebGPU

    Count
} RendererType;

typedef struct Renderer {
    RendererType type;
    char        *type_name;
} Renderer;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(GuiSystem);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(GuiContext);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(AppWindow);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(AppWindowHandle);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Renderer);

EQUILIBRIUM_API
void GuiComponentsImport(world_t *world);

#endif