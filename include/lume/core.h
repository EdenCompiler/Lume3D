#ifndef LUME_CORE_H
#define LUME_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(LUME_SHARED)
#if defined(LUME_BUILDING_LIBRARY)
#define LUME_API __declspec(dllexport)
#else
#define LUME_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define LUME_API __attribute__((visibility("default")))
#else
#define LUME_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define LUME_VERSION_MAJOR 1
#define LUME_VERSION_MINOR 5
#define LUME_VERSION_PATCH 1
#define LUME_VERSION_STRING "1.5.1"

typedef struct LumeApp LumeApp;
typedef struct LumeRenderer LumeRenderer;

typedef enum LumeResult
{
    LUME_SUCCESS = 0,
    LUME_ERROR_INVALID_ARGUMENT,
    LUME_ERROR_OUT_OF_MEMORY,
    LUME_ERROR_IO,
    LUME_ERROR_PARSE,
    LUME_ERROR_UNSUPPORTED,
    LUME_ERROR_GPU,
    LUME_ERROR_CANCELLED,
    LUME_ERROR_NOT_READY,
    LUME_ERROR_INTERNAL
} LumeResult;

typedef struct LumeError
{
    LumeResult code;
    const char *operation;
    const char *path;
    int line;
    int column;
    char message[1024];
} LumeError;

typedef enum LumeLogLevel
{
    LUME_LOG_INFO,
    LUME_LOG_WARNING,
    LUME_LOG_ERROR
} LumeLogLevel;

typedef void (*LumeLogCallback)(LumeLogLevel level, const char *message, void *user_data);

typedef struct LumeColor
{
    float r, g, b, a;
} LumeColor;

typedef struct LumeAppConfig
{
    const char *title;
    int width;
    int height;
    bool resizable;
    bool visible;
    bool vsync;
    LumeColor clear_color;
    uint32_t worker_count;
    float hot_reload_interval_seconds;
    LumeLogCallback log_callback;
    void *log_user_data;
} LumeAppConfig;

typedef enum LumeKey
{
    LUME_KEY_UNKNOWN = -1,
    LUME_KEY_SPACE = 32,
    LUME_KEY_APOSTROPHE = 39,
    LUME_KEY_COMMA = 44,
    LUME_KEY_MINUS = 45,
    LUME_KEY_PERIOD = 46,
    LUME_KEY_SLASH = 47,
    LUME_KEY_0 = 48,
    LUME_KEY_1 = 49,
    LUME_KEY_2 = 50,
    LUME_KEY_3 = 51,
    LUME_KEY_4 = 52,
    LUME_KEY_5 = 53,
    LUME_KEY_6 = 54,
    LUME_KEY_7 = 55,
    LUME_KEY_8 = 56,
    LUME_KEY_9 = 57,
    LUME_KEY_A = 65,
    LUME_KEY_B = 66,
    LUME_KEY_C = 67,
    LUME_KEY_D = 68,
    LUME_KEY_E = 69,
    LUME_KEY_F = 70,
    LUME_KEY_G = 71,
    LUME_KEY_H = 72,
    LUME_KEY_I = 73,
    LUME_KEY_J = 74,
    LUME_KEY_K = 75,
    LUME_KEY_L = 76,
    LUME_KEY_M = 77,
    LUME_KEY_N = 78,
    LUME_KEY_O = 79,
    LUME_KEY_P = 80,
    LUME_KEY_Q = 81,
    LUME_KEY_R = 82,
    LUME_KEY_S = 83,
    LUME_KEY_T = 84,
    LUME_KEY_U = 85,
    LUME_KEY_V = 86,
    LUME_KEY_W = 87,
    LUME_KEY_X = 88,
    LUME_KEY_Y = 89,
    LUME_KEY_Z = 90,
    LUME_KEY_ESCAPE = 256,
    LUME_KEY_ENTER = 257,
    LUME_KEY_TAB = 258,
    LUME_KEY_BACKSPACE = 259,
    LUME_KEY_INSERT = 260,
    LUME_KEY_DELETE = 261,
    LUME_KEY_RIGHT = 262,
    LUME_KEY_LEFT = 263,
    LUME_KEY_DOWN = 264,
    LUME_KEY_UP = 265,
    LUME_KEY_PAGE_UP = 266,
    LUME_KEY_PAGE_DOWN = 267,
    LUME_KEY_HOME = 268,
    LUME_KEY_END = 269,
    LUME_KEY_LEFT_SHIFT = 340,
    LUME_KEY_LEFT_CONTROL = 341,
    LUME_KEY_LEFT_ALT = 342,
    LUME_KEY_RIGHT_SHIFT = 344,
    LUME_KEY_RIGHT_CONTROL = 345,
    LUME_KEY_RIGHT_ALT = 346
} LumeKey;

typedef enum LumeMouseButton
{
    LUME_MOUSE_BUTTON_LEFT = 0,
    LUME_MOUSE_BUTTON_RIGHT = 1,
    LUME_MOUSE_BUTTON_MIDDLE = 2
} LumeMouseButton;

LUME_API const char *lume_version_string(void);
LUME_API const LumeError *lume_error_last(void);
LUME_API const char *lume_result_string(LumeResult result);
LUME_API void lume_error_clear(void);
LUME_API LumeAppConfig lume_app_config_default(void);
LUME_API LumeResult lume_app_create(const LumeAppConfig *config, LumeApp **out_app);
LUME_API void lume_app_destroy(LumeApp *app);
LUME_API LumeRenderer *lume_app_renderer(LumeApp *app);
LUME_API bool lume_app_should_close(const LumeApp *app);
LUME_API void lume_app_request_close(LumeApp *app);
LUME_API float lume_app_begin_frame(LumeApp *app);
LUME_API void lume_app_end_frame(LumeApp *app);
LUME_API void lume_app_set_clear_color(LumeApp *app, LumeColor color);
LUME_API void lume_app_get_framebuffer_size(const LumeApp *app, int *width, int *height);
LUME_API bool lume_key_is_down(const LumeApp *app, LumeKey key);
LUME_API bool lume_key_was_pressed(const LumeApp *app, LumeKey key);
LUME_API bool lume_key_was_released(const LumeApp *app, LumeKey key);
LUME_API bool lume_mouse_button_is_down(const LumeApp *app, LumeMouseButton button);
LUME_API bool lume_mouse_button_was_pressed(const LumeApp *app, LumeMouseButton button);
LUME_API bool lume_mouse_button_was_released(const LumeApp *app, LumeMouseButton button);
LUME_API void lume_mouse_get_position(const LumeApp *app, float *out_x, float *out_y);
LUME_API void lume_mouse_get_delta(const LumeApp *app, float *out_x, float *out_y);
LUME_API void lume_mouse_get_scroll(const LumeApp *app, float *out_x, float *out_y);

#ifdef __cplusplus
}
#endif
#endif
