#ifndef LUME_ANIMATION_H
#define LUME_ANIMATION_H
#include <lume/assets.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LumeAnimationPlayer LumeAnimationPlayer;
typedef enum LumeLoopMode
{
    LUME_LOOP_ONCE,
    LUME_LOOP_REPEAT,
    LUME_LOOP_PING_PONG
} LumeLoopMode;

LUME_API const char *lume_animation_clip_name(const LumeAnimationClip *clip);
LUME_API float lume_animation_clip_duration(const LumeAnimationClip *clip);
LUME_API LumeResult lume_animation_player_create(LumeModelInstance *instance, LumeAnimationPlayer **out_player);
LUME_API void lume_animation_player_destroy(LumeAnimationPlayer *player);
LUME_API LumeResult lume_animation_player_play(LumeAnimationPlayer *player, LumeAnimationClip *clip,
                                               LumeLoopMode loop_mode);
LUME_API LumeResult lume_animation_player_crossfade(LumeAnimationPlayer *player, LumeAnimationClip *clip,
                                                    float duration);
LUME_API void lume_animation_player_update(LumeAnimationPlayer *player, float delta_seconds);
LUME_API void lume_animation_player_pause(LumeAnimationPlayer *player, bool paused);
LUME_API void lume_animation_player_seek(LumeAnimationPlayer *player, float seconds);
LUME_API void lume_animation_player_set_speed(LumeAnimationPlayer *player, float speed);
LUME_API float lume_animation_player_time(const LumeAnimationPlayer *player);

#ifdef __cplusplus
}
#endif
#endif
