# Animation

Imported glTF transform animations are exposed as immutable `LumeAnimationClip` objects owned by a model. A `LumeAnimationPlayer` controls one instantiated model.

```c
LumeAnimationPlayer *player = NULL;
LumeAnimationClip *clip = lume_model_animation(model, 0);

if (lume_animation_player_create(instance, &player) == LUME_SUCCESS)
    lume_animation_player_play(player, clip, LUME_LOOP_REPEAT, 0.0f);

while (!lume_app_should_close(app)) {
    float dt = lume_app_begin_frame(app);
    lume_animation_player_update(player, dt);
    lume_app_render(app, scene, camera);
    lume_app_end_frame(app);
}

lume_animation_player_destroy(player);
```

The player samples translation, quaternion rotation, and scale channels. It supports once, repeat, and ping-pong loop modes; pause; seek; signed playback speed; and timed crossfades. `lume_animation_player_time` reports the active clip time in seconds.

Players do not retain the model instance. Destroy each player before its instance. Clips remain valid while the source model is alive.
