// Derived from this Gist by Richard Gale:
//     https://gist.github.com/RichardGale/6e2b74bc42b3005e08397236e4be0fd0

// ImGui BFFX binding
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture
// identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See
// main.cpp for an example of using this. If you use this binding you'll need to
// call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(),
// ImGui::Render() and ImGui_ImplXXXX_Shutdown(). If you are new to ImGui, see
// examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// BGFX/BX
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>

#include "imgui_impl_bgfx.h"
#include "vs_ocornut_imgui.bin.h"
#include "fs_ocornut_imgui.bin.h"

#include "vs_imgui_image.bin.h"
#include "fs_imgui_image.bin.h"

#include <vector>
#include <SDL.h>
#include <SDL_syswm.h>
#include <imgui.h>

// Data
static uint8_t             g_View              = 255;
static bgfx::TextureHandle g_FontTexture       = BGFX_INVALID_HANDLE;
static bgfx::ProgramHandle g_ShaderHandle      = BGFX_INVALID_HANDLE;
static bgfx::ProgramHandle m_imageProgram      = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle g_AttribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout  g_VertexLayout;
static bgfx::UniformHandle u_imageLodEnabled;

static std::vector<bgfx::ViewId> free_view_ids;
static bgfx::ViewId              sub_view_id = 100;

static bgfx::ViewId allocate_view_id() {
    if (!free_view_ids.empty()) {
        auto id = free_view_ids.back();
        free_view_ids.pop_back();
        return id;
    }
    return sub_view_id++;
}

static void free_view_id(bgfx::ViewId id) { free_view_ids.push_back(id); }

static inline bool checkAvailTransientBuffers(uint32_t                  _numVertices,
                                              const bgfx::VertexLayout &_layout,
                                              uint32_t                  _numIndices) {
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout) &&
           (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices));
}

struct imgui_viewport_data {
    bgfx::FrameBufferHandle frameBufferHandle;
    bgfx::ViewId            viewId = 0;
    uint16_t                width  = 0;
    uint16_t                height = 0;
};

static void *native_window_handle(void *window) {
    SDL_Window   *sdl_window = (SDL_Window *)window;
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(sdl_window, &wmi)) {
        return NULL;
    }
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#if ENTRY_CONFIG_USE_WAYLAND
    wl_egl_window *win_impl = (wl_egl_window *)SDL_GetWindowData(sdl_window, "wl_egl_window");
    if (!win_impl) {
        int width, height;
        SDL_GetWindowSize(sdl_window, &width, &height);
        struct wl_surface *surface = wmi.info.wl.surface;
        if (!surface)
            return nullptr;
        win_impl = wl_egl_window_create(surface, width, height);
        SDL_SetWindowData(sdl_window, "wl_egl_window", win_impl);
    }
    return (void *)(uintptr_t)win_impl;
#else
    return (void *)wmi.info.x11.window;
#endif
#elif BX_PLATFORM_OSX
    return wmi.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
    return wmi.info.win.window;
#endif // BX_PLATFORM_
}

static void ImguiBgfxOnCreateWindow(ImGuiViewport *viewport) {
    auto data = new imgui_viewport_data();

    // Setup view id and size
    data->viewId = allocate_view_id();
    data->width  = bx::max<uint16_t>((uint16_t)viewport->Size.x, 1);
    data->height = bx::max<uint16_t>((uint16_t)viewport->Size.y, 1);
    // Create frame buffer
    data->frameBufferHandle = bgfx::createFrameBuffer(
        native_window_handle((SDL_Window *)viewport->PlatformHandle), data->width, data->height);
    viewport->RendererUserData = data;
}

static void ImguiBgfxOnDestroyWindow(ImGuiViewport *viewport) {
    auto data = (imgui_viewport_data *)viewport->RendererUserData;

    if (data) {
        viewport->RendererUserData = nullptr;
        free_view_id(data->viewId);
        bgfx::destroy(data->frameBufferHandle);
        data->frameBufferHandle.idx = bgfx::kInvalidHandle;
        delete data;
    }
}

static void ImguiBgfxOnSetWindowSize(ImGuiViewport *viewport, ImVec2 size) {
    ImguiBgfxOnDestroyWindow(viewport);
    ImguiBgfxOnCreateWindow(viewport);
}

static void ImguiBgfxOnRenderWindow(ImGuiViewport *viewport, void *) {
    auto data = (imgui_viewport_data *)viewport->RendererUserData;

    if (data) {

        bgfx::setViewFrameBuffer(data->viewId, data->frameBufferHandle);
        bgfx::setViewRect(data->viewId, 0, 0, data->width, data->height);

        bgfx::setViewClear(data->viewId, BGFX_CLEAR_COLOR, 0xff00ffff, 1.0f, 0);
        bgfx::setState(BGFX_STATE_DEFAULT);

        ImGui_Implbgfx_RenderDrawLists(data->viewId, viewport->DrawData);

        bgfx::touch(data->viewId);
    }
}

enum class BgfxTextureFlags : uint32_t {
    Opaque       = 1u << 31,
    PointSampler = 1u << 30,
    All          = Opaque | PointSampler,
};

// This is the main rendering function that you have to implement and call after
// ImGui::Render(). Pass ImGui::GetDrawData() to this function.
// Note: If text or lines are blurry when integrating ImGui into your engine,
// in your Render function, try translating your projection matrix by
// (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_Implbgfx_RenderDrawLists(bgfx::ViewId viewId, struct ImDrawData *draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays
    // (screen coordinates != framebuffer coordinates)
    int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    bgfx::setViewName(viewId, "ImGui");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);

    const bgfx::Caps *caps = bgfx::getCaps();
    {
        float ortho[16];
        float x      = draw_data->DisplayPos.x;
        float y      = draw_data->DisplayPos.y;
        float width  = draw_data->DisplaySize.x;
        float height = draw_data->DisplaySize.y;

        bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f,
                     caps->homogeneousDepth, bx::Handedness::Left);
        bgfx::setViewTransform(viewId, NULL, ortho);
        bgfx::setViewRect(viewId, 0, 0, uint16_t(width), uint16_t(height));
    }

    const ImVec2 clipPos   = draw_data->DisplayPos;       // (0,0) unless using multi-viewports
    const ImVec2 clipScale = draw_data->FramebufferScale; // (1,1) unless using retina display which
                                                          // are often (2,2)

    // Render command lists
    for (int32_t ii = 0, num = draw_data->CmdListsCount; ii < num; ++ii) {
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer  tib;

        const ImDrawList *drawList    = draw_data->CmdLists[ii];
        uint32_t          numVertices = (uint32_t)drawList->VtxBuffer.size();
        uint32_t          numIndices  = (uint32_t)drawList->IdxBuffer.size();

        if (!checkAvailTransientBuffers(numVertices, g_VertexLayout, numIndices)) {
            // not enough space in transient buffer just quit drawing the
            // rest...
            break;
        }

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, g_VertexLayout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

        ImDrawVert *verts = (ImDrawVert *)tvb.data;
        bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

        ImDrawIdx *indices = (ImDrawIdx *)tib.data;
        bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

        bgfx::Encoder *encoder = bgfx::begin();

        uint32_t offset = 0;
        for (const ImDrawCmd *cmd    = drawList->CmdBuffer.begin(),
                             *cmdEnd = drawList->CmdBuffer.end();
             cmd != cmdEnd; ++cmd) {
            if (cmd->UserCallback) {
                cmd->UserCallback(drawList, cmd);
            } else if (0 != cmd->ElemCount) {
                uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
                uint32_t sampler_state = 0;

                bgfx::TextureHandle th      = g_FontTexture;
                bgfx::ProgramHandle program = g_ShaderHandle;

                auto alphaBlend = true;
                if (cmd->TextureId != nullptr) {
                    auto textureInfo = (uintptr_t)cmd->TextureId;
                    if (textureInfo & (uint32_t)BgfxTextureFlags::Opaque) {
                        alphaBlend = false;
                    }
                    if (textureInfo & (uint32_t)BgfxTextureFlags::PointSampler) {
                        sampler_state = BGFX_SAMPLER_POINT;
                    }
                    textureInfo &= ~(uint32_t)BgfxTextureFlags::All;
                    th = {(uint16_t)textureInfo};
                }
                if (alphaBlend) {
                    state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                                   BGFX_STATE_BLEND_INV_SRC_ALPHA);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clipRect;
                clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                if (clipRect.x < fb_width && clipRect.y < fb_height && clipRect.z >= 0.0f &&
                    clipRect.w >= 0.0f) {
                    const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f));
                    const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f));
                    encoder->setScissor(xx, yy, uint16_t(bx::min(clipRect.z, 65535.0f) - xx),
                                        uint16_t(bx::min(clipRect.w, 65535.0f) - yy));

                    encoder->setState(state);
                    encoder->setTexture(0, g_AttribLocationTex, th, sampler_state);
                    encoder->setVertexBuffer(0, &tvb, 0, numVertices);
                    encoder->setIndexBuffer(&tib, offset, cmd->ElemCount);
                    encoder->submit(viewId, program);
                }
            }

            offset += cmd->ElemCount;
        }

        bgfx::end(encoder);
    }
}

bool ImGui_Implbgfx_CreateFontsTexture() {
    // Build texture atlas
    ImGuiIO       &io = ImGui::GetIO();
    unsigned char *pixels;
    int            width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    g_FontTexture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1,
                                          bgfx::TextureFormat::BGRA8, 0,
                                          bgfx::copy(pixels, width * height * 4));

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture.idx;

    return true;
}

static const bgfx::EmbeddedShader s_embeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui), BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image), BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END()};

bool ImGui_Implbgfx_CreateDeviceObjects() {
    bgfx::RendererType::Enum type = bgfx::getRendererType();
    g_ShaderHandle                = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"), true);

    g_VertexLayout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    m_imageProgram    = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image"), true);

    g_AttribLocationTex = bgfx::createUniform("g_AttribLocationTex", bgfx::UniformType::Sampler);

    ImGui_Implbgfx_CreateFontsTexture();

    return true;
}

void ImGui_Implbgfx_InvalidateDeviceObjects() {
    bgfx::destroy(g_AttribLocationTex);
    bgfx::destroy(g_ShaderHandle);
    bgfx::destroy(m_imageProgram);
    bgfx::destroy(u_imageLodEnabled);

    if (isValid(g_FontTexture)) {
        bgfx::destroy(g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture.idx           = bgfx::kInvalidHandle;
    }
}

void ImGui_Implbgfx_Init(int view) {
    g_View                             = (uint8_t)(view & 0xff);
    ImGuiPlatformIO &platform_io       = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow  = ImguiBgfxOnCreateWindow;
    platform_io.Renderer_DestroyWindow = ImguiBgfxOnDestroyWindow;
    platform_io.Renderer_SetWindowSize = ImguiBgfxOnSetWindowSize;
    platform_io.Renderer_RenderWindow  = ImguiBgfxOnRenderWindow;
}

void ImGui_Implbgfx_Shutdown() { ImGui_Implbgfx_InvalidateDeviceObjects(); }

void ImGui_Implbgfx_NewFrame() {
    if (!isValid(g_FontTexture)) {
        ImGui_Implbgfx_CreateDeviceObjects();
    }
}