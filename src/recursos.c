#include "lume_interno.h"

#include <stb_image.h>

#include <stdlib.h>

LumeTextureConfig lume_texture_config_default(void)
{
    return (LumeTextureConfig){LUME_TEXTURE_FILTER_LINEAR,
                               LUME_TEXTURE_FILTER_LINEAR,
                               LUME_TEXTURE_WRAP_REPEAT,
                               LUME_TEXTURE_WRAP_REPEAT,
                               true,
                               true};
}

LumeTexture *lume_texture_create(LumeApp *aplicativo, const uint8_t *pixels, int largura, int altura,
                                 const LumeTextureConfig *config)
{
    LumeTextureConfig configuracao = config ? *config : lume_texture_config_default();
    LumeTexture *textura;
    GLint filtro_minimo;

    if (!aplicativo || !pixels || largura <= 0 || altura <= 0)
    {
        lume_definir_erro("Texture creation requires an application, RGBA pixels, and positive dimensions.");
        return NULL;
    }
    textura = calloc(1, sizeof(*textura));
    if (!textura)
    {
        lume_definir_erro("Out of memory while creating a texture.");
        return NULL;
    }
    textura->aplicativo = aplicativo;
    textura->largura = largura;
    textura->altura = altura;
    filtro_minimo = configuracao.min_filter == LUME_TEXTURE_FILTER_NEAREST
                        ? (configuracao.generate_mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST)
                        : (configuracao.generate_mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    glfwMakeContextCurrent(aplicativo->janela);
    glGenTextures(1, &textura->identificador);
    glBindTexture(GL_TEXTURE_2D, textura->identificador);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtro_minimo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    configuracao.mag_filter == LUME_TEXTURE_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    configuracao.wrap_u == LUME_TEXTURE_WRAP_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    configuracao.wrap_v == LUME_TEXTURE_WRAP_REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, largura, altura, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (configuracao.generate_mipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!lume_adicionar_ponteiro((void ***)&aplicativo->texturas, &aplicativo->quantidade_texturas,
                                 &aplicativo->capacidade_texturas, textura))
    {
        glDeleteTextures(1, &textura->identificador);
        free(textura);
        return NULL;
    }
    return textura;
}

LumeTexture *lume_texture_load(LumeApp *aplicativo, const char *caminho, const LumeTextureConfig *config)
{
    LumeTextureConfig configuracao = config ? *config : lume_texture_config_default();
    unsigned char *pixels;
    int largura;
    int altura;
    int canais;
    LumeTexture *textura;

    if (!caminho)
    {
        lume_definir_erro("A file path is required to load a texture.");
        return NULL;
    }
    stbi_set_flip_vertically_on_load(configuracao.flip_y ? 1 : 0);
    pixels = stbi_load(caminho, &largura, &altura, &canais, 4);
    if (!pixels)
    {
        lume_definir_erro("Could not load texture '%s': %s", caminho, stbi_failure_reason());
        return NULL;
    }
    textura = lume_texture_create(aplicativo, pixels, largura, altura, &configuracao);
    stbi_image_free(pixels);
    return textura;
}

LumeBasicMaterialConfig lume_basic_material_config_default(void)
{
    return (LumeBasicMaterialConfig){{1.0f, 1.0f, 1.0f, 1.0f}, NULL, false};
}

LumeLambertMaterialConfig lume_lambert_material_config_default(void)
{
    return (LumeLambertMaterialConfig){{1.0f, 1.0f, 1.0f, 1.0f}, NULL, false};
}

static LumeMaterial *lume_criar_material(LumeApp *aplicativo, LumeTipoMaterial tipo, LumeColor cor,
                                         LumeTexture *textura, bool aramado)
{
    LumeMaterial *material;
    if (!aplicativo)
    {
        lume_definir_erro("A valid application is required to create a material.");
        return NULL;
    }
    if (textura && textura->aplicativo != aplicativo)
    {
        lume_definir_erro("A material texture must belong to the same application.");
        return NULL;
    }
    material = calloc(1, sizeof(*material));
    if (!material)
    {
        lume_definir_erro("Out of memory while creating a material.");
        return NULL;
    }
    material->aplicativo = aplicativo;
    material->tipo = tipo;
    material->cor = cor;
    material->textura = textura;
    material->aramado = aramado;
    if (!lume_adicionar_ponteiro((void ***)&aplicativo->materiais, &aplicativo->quantidade_materiais,
                                 &aplicativo->capacidade_materiais, material))
    {
        free(material);
        return NULL;
    }
    return material;
}

LumeMaterial *lume_material_create_basic(LumeApp *aplicativo, const LumeBasicMaterialConfig *config)
{
    LumeBasicMaterialConfig configuracao = config ? *config : lume_basic_material_config_default();
    return lume_criar_material(aplicativo, LUME_MATERIAL_BASICO, configuracao.color, configuracao.texture,
                               configuracao.wireframe);
}

LumeMaterial *lume_material_create_lambert(LumeApp *aplicativo, const LumeLambertMaterialConfig *config)
{
    LumeLambertMaterialConfig configuracao = config ? *config : lume_lambert_material_config_default();
    return lume_criar_material(aplicativo, LUME_MATERIAL_LAMBERT, configuracao.color, configuracao.texture,
                               configuracao.wireframe);
}

void lume_material_set_color(LumeMaterial *material, LumeColor cor)
{
    if (material)
    {
        material->cor = cor;
    }
}

void lume_material_set_texture(LumeMaterial *material, LumeTexture *textura)
{
    if (!material)
    {
        return;
    }
    if (textura && textura->aplicativo != material->aplicativo)
    {
        lume_definir_erro("A material texture must belong to the same application.");
        return;
    }
    material->textura = textura;
}
