#ifndef LUME_DEBUG_H
#define LUME_DEBUG_H
#include <lume/render.h>

#ifdef __cplusplus
extern "C"
{
#endif

LUME_API void lume_debug_line(LumeRenderer *renderer, LumeVec3 start, LumeVec3 end, LumeColor color);
LUME_API void lume_debug_axes(LumeRenderer *renderer, LumeMat4 transform, float size);
LUME_API void lume_debug_aabb(LumeRenderer *renderer, LumeAabb bounds, LumeColor color);
LUME_API void lume_debug_sphere(LumeRenderer *renderer, LumeSphere sphere, LumeColor color);
LUME_API void lume_debug_ray(LumeRenderer *renderer, LumeRay ray, float length, LumeColor color);
LUME_API void lume_debug_clear(LumeRenderer *renderer);

#ifdef __cplusplus
}
#endif
#endif
