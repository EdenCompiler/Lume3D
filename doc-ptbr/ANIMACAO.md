# Animação

Animações de transformação importadas do glTF são expostas como objetos imutáveis `LumeAnimationClip` pertencentes ao modelo. Um `LumeAnimationPlayer` controla uma instância do modelo.

```c
LumeAnimationPlayer *player = NULL;
LumeAnimationClip *clip = lume_model_animation(modelo, 0);

if (lume_animation_player_create(instancia, &player) == LUME_SUCCESS)
    lume_animation_player_play(player, clip, LUME_LOOP_REPEAT, 0.0f);

while (!lume_app_should_close(aplicativo)) {
    float delta = lume_app_begin_frame(aplicativo);
    lume_animation_player_update(player, delta);
    lume_app_render(aplicativo, cena, camera);
    lume_app_end_frame(aplicativo);
}

lume_animation_player_destroy(player);
```

O player amostra canais de translação, rotação quaternion e escala. Ele oferece loops once, repeat e ping-pong; pausa; seek; velocidade com sinal; e crossfade temporizado. `lume_animation_player_time` informa o tempo do clip ativo em segundos.

Players não retêm a instância. Destrua cada player antes da sua instância. Clips continuam válidos enquanto o modelo de origem estiver vivo.
