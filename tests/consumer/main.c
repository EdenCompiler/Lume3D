#include <lume/lume.h>

int main(void)
{
    LumeAppConfig config = lume_app_config_default();
    return config.width > 0 && config.height > 0 ? 0 : 1;
}
