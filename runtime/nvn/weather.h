// Weather and snow drawn over the game (Cosmetics "Weather"). LAS drew them over Link's Awakening's overworld
// (runtime/nvn/weather*.c and vulkan/weather_vk.c / snow_vk.c, removed with its features); DKCR has none, so there is
// never a weather frame or snow cover and the passes are never reached. The types and calls stay in the renderer
// (runtime/nvn/vulkan/exec.c) until its weather paths are taken out.
#pragma once
#include <stdbool.h>
#include <stdint.h>

enum { WEATHER_PROBES = 5 };

typedef struct {
    float vp[16], inv_vp[16];
    float origin[4], box[4], shape[4], color[4], screen[4], flags[4], fill[4], zr[4], cam[4], wind[4], leaf[4];
    uint32_t count, splashes;
    int kind;
} WeatherFrame;

typedef struct {
    float vp[16], inv_vp[16];
    float screen[4], flags[4], zr[4], trail[4], tone[4], shake[8], lvp[16], shd[4], shz[4], shade[4];
} SnowFrame;

static inline void weather_scene_camera(const float *u, uint64_t size, uint64_t frame) { (void)u, (void)size, (void)frame; }
static inline void weather_scene_vp(const float *vp, uint64_t frame) { (void)vp, (void)frame; }
static inline bool weather_frame(WeatherFrame *f, uint64_t frame, const float probe[WEATHER_PROBES]) {
    (void)f, (void)frame, (void)probe;
    return false;
}
static inline void weather_cycle_scene(void) {}
static inline bool weather_snow_cover(SnowFrame *s, const float vp[16]) { (void)s, (void)vp; return false; }
static inline bool weather_snow_on(void) { return false; }
static inline int weather_snow_tree(const float pos[3]) { (void)pos; return 0; }
static inline bool weather_snow_animated(uint64_t key, const void *vtx, uint64_t size, uint64_t frame) {
    (void)key, (void)vtx, (void)size, (void)frame;
    return false;
}

#ifdef VK_VERSION_1_0  // (the Vulkan passes: never reached without a weather frame or a snow cover)
static inline bool vk_weather_probes(float out[WEATHER_PROBES]) { (void)out; return false; }
static inline void vk_weather_pass(VkImageView color, VkFormat fmt, uint32_t w, uint32_t h, VkImageView depth,
                                   const WeatherFrame *wf, int parts, bool dbg) {
    (void)color, (void)fmt, (void)w, (void)h, (void)depth, (void)wf, (void)parts, (void)dbg;
}
static inline bool vk_snow_pass(VkImage color_img, VkImageView color, VkFormat fmt, uint32_t cw, uint32_t ch,
                                VkImageView depth, VkImageView stencil, VkImageView shadow, SnowFrame *sf) {
    (void)color_img, (void)color, (void)fmt, (void)cw, (void)ch, (void)depth, (void)stencil, (void)shadow, (void)sf;
    return false;
}
#endif
