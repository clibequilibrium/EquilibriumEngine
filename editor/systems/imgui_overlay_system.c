#include <assert.h>
#include <systems/imgui/cimgui_base.h>
#include "imgui_overlay_system.h"
#include "bgfx_components.h"
#include "utils/bgfx_utils.h"

static bool bar_draw(float width, float maxWidth, float height, const ImVec4 color, char *name) {
    ImGuiStyle *style = igGetStyle();

    ImVec4 hoveredColor = {color.x * 1.1f, color.y * 1.1f, color.z * 1.1f, color.w * 1.1f};

    igPushStyleColor_Vec4(ImGuiCol_Button, color);
    igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, hoveredColor);
    igPushStyleColor_Vec4(ImGuiCol_ButtonActive, color);
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 0.0f);
    igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){0.0f, style->ItemSpacing.y});

    bool itemHovered = false;

    igButton(name, (ImVec2){width, height});
    itemHovered = itemHovered || igIsItemHovered(0);

    igSameLine(0, 0);
    igInvisibleButton(name, (ImVec2){glm_max(1.0f, maxWidth - width), height}, 0);
    itemHovered = itemHovered || igIsItemHovered(0);

    igPopStyleVar(2);
    igPopStyleColor(3);

    return itemHovered;
}

static void DrawOverlay(ecs_iter_t *it) {

    // ecs_pipeline_stats_t stats = {0};
    // ecs_get_pipeline_stats(it->world, ecs_get_pipeline(it->world), &stats);

    // igText("Active system count: %d \n", stats.system_count);

    // ecs_query_t *system_query = ecs_query_new(it->world,
    // "flecs.system.System"); ecs_iter_t system_it = ecs_query_iter(it->world,
    // system_query); while (ecs_query_next(&system_it)) {
    //   for (int i = 0; i < system_it.count; i++) {
    //     ecs_system_stats_t stats = {0};
    //     ecs_get_system_stats(it->world, system_it.entities[i], &stats);

    //     printf("%s \n", ecs_get_name(it->world, system_it.entities[i]));
    //   }
    // }

    const float  GRAPH_FREQUENCY = 0.05f;
    const size_t GRAPH_HISTORY   = 100;

    static float mTime = 0.0f;
    mTime += igGetIO()->DeltaTime;

    // performance overlay
    if (true) {
        const float  overlayWidth = 150.0f;
        const ImVec2 padding      = {5.0f, 25.0f};

        // top left, transparent background
        igSetNextWindowPos(padding, ImGuiCond_Always, (ImVec2){});
        igSetNextWindowBgAlpha(0.5f);
        igBegin("Stats", NULL,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                    ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs);

        // title
        // igSeparator();

        // general data
        const bgfx_stats_t *stats   = bgfx_get_stats();
        const double        toCpuMs = 1000.0 / (double)(stats->cpuTimerFreq);
        const double        toGpuMs = 1000.0 / (double)(stats->gpuTimerFreq);

        igText("Backend: %s", bgfx_get_renderer_name(bgfx_get_renderer_type()));
        igText("Buffer size: %u x %u px", stats->width, stats->height);
        igText("Triangles: %u", stats->numPrims[BGFX_TOPOLOGY_TRI_LIST]);
        igText("Draw calls: %u", stats->numDraw);
        igText("Compute calls: %u", stats->numCompute);

        // plots
        static float  fpsValues[100]       = {0};
        static float  frameTimeValues[100] = {0};
        static float  gpuMemoryValues[100] = {0};
        static size_t offset               = 0;

        bool showFps       = true;
        bool showFrameTime = true;
        bool showProfiler  = true;
        bool showGpuMemory = true;

        if (showFps) {
            igSeparator();
            igText("FPS: %.0f", fpsValues[offset]);
        }
        if (showFrameTime) {
            igSeparator();
            igText("CPU: %.2f ms", (float)(stats->cpuTimeEnd - stats->cpuTimeBegin) * toCpuMs);
            igText("GPU: %.2f ms", (float)(stats->gpuTimeEnd - stats->gpuTimeBegin) * toGpuMs);
            igText("Total: %.2f ms", frameTimeValues[offset]);
        }

        if (showProfiler) {
            igSeparator();
            igText("View stats");
            if (stats->numViews > 0) {
                ImVec4 cpuColor = {0.5f, 1.0f, 0.5f, 1.0f};
                ImVec4 gpuColor = {0.5f, 0.5f, 1.0f, 1.0f};

                const float itemHeight            = igGetTextLineHeightWithSpacing();
                const float itemHeightWithSpacing = igGetFrameHeightWithSpacing();
                const float scale                 = 2.0f;

                if (igBeginListBox("##Stats", (ImVec2){overlayWidth,
                                                       stats->numViews * itemHeightWithSpacing})) {
                    ImGuiListClipper clipper = {stats->numViews, itemHeight};
                    ImGuiListClipper_Begin(&clipper, stats->numViews, itemHeight);

                    while (ImGuiListClipper_Step(&clipper)) {
                        for (int32_t pos = clipper.DisplayStart; pos < clipper.DisplayEnd; ++pos) {
                            bgfx_view_stats_t viewStats = stats->viewStats[pos];
                            float             cpuElapsed =
                                (float)((viewStats.cpuTimeEnd - viewStats.cpuTimeBegin) * toCpuMs);
                            float gpuElapsed =
                                (float)((viewStats.gpuTimeEnd - viewStats.gpuTimeBegin) * toGpuMs);

                            igText("%d", viewStats.view);

                            const float maxWidth = overlayWidth * 0.35f;
                            const float cpuWidth = glm_clamp(cpuElapsed * scale, 1.0f, maxWidth);
                            const float gpuWidth = glm_clamp(gpuElapsed * scale, 1.0f, maxWidth);

                            igSameLine(overlayWidth * 0.3f, 0);

                            char cpu[512] = "";
                            char gpu[512] = "";
                            snprintf(cpu, sizeof(cpu), "##%s%d%d", "cpu", viewStats.view, pos);
                            snprintf(gpu, sizeof(gpu), "##%s%d%d", "gpu", viewStats.view, pos);

                            if (bar_draw(cpuWidth, maxWidth, itemHeight, cpuColor, cpu)) {
                                igSetTooltip("%s -- CPU: %.2f ms", viewStats.name, cpuElapsed);
                            }

                            igSameLine(0, 0);

                            if (bar_draw(gpuWidth, maxWidth, itemHeight, gpuColor, gpu)) {
                                igSetTooltip("%s -- GPU: %.2f ms", viewStats.name, gpuElapsed);
                            }
                        }
                    }

                    igEndListBox();
                }
            }
        }

        if (showGpuMemory) {
            int64_t used = stats->gpuMemoryUsed;
            int64_t max  = stats->gpuMemoryMax;

            igSeparator();
            if (used > 0 && max > 0) {
                igText("GPU memory");
                char strUsed[64];
                bx_prettify(strUsed, BX_COUNT_OF(strUsed), stats->gpuMemoryUsed);
                char strMax[64];
                bx_prettify(strMax, BX_COUNT_OF(strMax), stats->gpuMemoryMax);
                igText("%s / %s", strUsed, strMax);
            }

            // update after drawing so offset is the current value
            static float oldTime = 0.0f;
            if (mTime - oldTime > GRAPH_FREQUENCY) {
                offset                  = (offset + 1) % GRAPH_HISTORY;
                ImGuiIO *io             = igGetIO();
                fpsValues[offset]       = io->Framerate;
                frameTimeValues[offset] = 1000.0f / io->Framerate;
                gpuMemoryValues[offset] = (float)(stats->gpuMemoryUsed) / 1024 / 1024;

                oldTime = mTime;
            }

            igEnd();
        }
    }
}

void ImguiOverlaySystemImport(world_t *world) {
    ECS_MODULE(world, ImguiOverlaySystem);
    ECS_IMPORT(world, GuiComponents);

    entity_create(world, "OverlaySystem", GuiSystem, {DrawOverlay});
}