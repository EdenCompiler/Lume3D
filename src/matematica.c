#include "lume_interno.h"

#include <float.h>
#include <math.h>

LumeVec3 lume_vec3_add(LumeVec3 a, LumeVec3 b)
{
    return (LumeVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
LumeVec3 lume_vec3_subtract(LumeVec3 a, LumeVec3 b)
{
    return (LumeVec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
LumeVec3 lume_vec3_scale(LumeVec3 v, float e)
{
    return (LumeVec3){v.x * e, v.y * e, v.z * e};
}
float lume_vec3_dot(LumeVec3 a, LumeVec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
LumeVec3 lume_vec3_cross(LumeVec3 a, LumeVec3 b)
{
    return (LumeVec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float lume_vec3_length(LumeVec3 v)
{
    return sqrtf(lume_vec3_dot(v, v));
}
LumeVec3 lume_vec3_normalize(LumeVec3 v)
{
    float comprimento = lume_vec3_length(v);
    return comprimento > 0.000001f ? lume_vec3_scale(v, 1.0f / comprimento) : (LumeVec3){0.0f, 0.0f, 0.0f};
}

LumeQuat lume_quat_identity(void)
{
    return (LumeQuat){0.0f, 0.0f, 0.0f, 1.0f};
}

static LumeQuat lume_quat_normalizar(LumeQuat q)
{
    float comprimento = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (comprimento <= 0.000001f)
        return lume_quat_identity();
    return (LumeQuat){q.x / comprimento, q.y / comprimento, q.z / comprimento, q.w / comprimento};
}

static LumeQuat lume_quat_multiplicar(LumeQuat a, LumeQuat b)
{
    return (LumeQuat){a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

LumeQuat lume_quat_from_euler(LumeVec3 r)
{
    float cx = cosf(r.x * 0.5f), sx = sinf(r.x * 0.5f);
    float cy = cosf(r.y * 0.5f), sy = sinf(r.y * 0.5f);
    float cz = cosf(r.z * 0.5f), sz = sinf(r.z * 0.5f);
    LumeQuat qx = {sx, 0, 0, cx}, qy = {0, sy, 0, cy}, qz = {0, 0, sz, cz};
    return lume_quat_normalizar(lume_quat_multiplicar(qz, lume_quat_multiplicar(qy, qx)));
}

LumeQuat lume_quat_slerp(LumeQuat a, LumeQuat b, float t)
{
    float produto = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    float escala_a, escala_b;
    if (produto < 0.0f)
    {
        produto = -produto;
        b = (LumeQuat){-b.x, -b.y, -b.z, -b.w};
    }
    if (produto > 0.9995f)
        return lume_quat_normalizar(
            (LumeQuat){a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z), a.w + t * (b.w - a.w)});
    {
        float angulo = acosf(fmaxf(-1.0f, fminf(1.0f, produto)));
        float seno = sinf(angulo);
        escala_a = sinf((1.0f - t) * angulo) / seno;
        escala_b = sinf(t * angulo) / seno;
    }
    return (LumeQuat){a.x * escala_a + b.x * escala_b, a.y * escala_a + b.y * escala_b, a.z * escala_a + b.z * escala_b,
                      a.w * escala_a + b.w * escala_b};
}

LumeMat4 lume_mat4_identity(void)
{
    LumeMat4 m = {{0}};
    m.values[0] = m.values[5] = m.values[10] = m.values[15] = 1.0f;
    return m;
}

LumeMat4 lume_mat4_multiply(LumeMat4 a, LumeMat4 b)
{
    LumeMat4 r = {{0}};
    int c, l, i;
    for (c = 0; c < 4; ++c)
        for (l = 0; l < 4; ++l)
            for (i = 0; i < 4; ++i)
                r.values[c * 4 + l] += a.values[i * 4 + l] * b.values[c * 4 + i];
    return r;
}

LumeMat4 lume_mat4_transform(LumeVec3 p, LumeQuat q, LumeVec3 s)
{
    LumeMat4 m = lume_mat4_identity();
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z, xy = q.x * q.y, xz = q.x * q.z;
    float yz = q.y * q.z, wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    q = lume_quat_normalizar(q);
    (void)q;
    m.values[0] = (1 - 2 * (yy + zz)) * s.x;
    m.values[1] = (2 * (xy + wz)) * s.x;
    m.values[2] = (2 * (xz - wy)) * s.x;
    m.values[4] = (2 * (xy - wz)) * s.y;
    m.values[5] = (1 - 2 * (xx + zz)) * s.y;
    m.values[6] = (2 * (yz + wx)) * s.y;
    m.values[8] = (2 * (xz + wy)) * s.z;
    m.values[9] = (2 * (yz - wx)) * s.z;
    m.values[10] = (1 - 2 * (xx + yy)) * s.z;
    m.values[12] = p.x;
    m.values[13] = p.y;
    m.values[14] = p.z;
    return m;
}

LumeMat4 lume_matriz_transformacao_euler(LumeVec3 p, LumeVec3 r, LumeVec3 s)
{
    return lume_mat4_transform(p, lume_quat_from_euler(r), s);
}

LumeMat4 lume_mat4_perspective(float fov, float aspect, float perto, float longe)
{
    LumeMat4 m = {{0}};
    float e = 1.0f / tanf(fov * 0.5f);
    m.values[0] = e / aspect;
    m.values[5] = e;
    m.values[10] = (longe + perto) / (perto - longe);
    m.values[11] = -1.0f;
    m.values[14] = (2.0f * longe * perto) / (perto - longe);
    return m;
}

LumeMat4 lume_mat4_orthographic(float e, float d, float b, float t, float p, float l)
{
    LumeMat4 m = lume_mat4_identity();
    m.values[0] = 2 / (d - e);
    m.values[5] = 2 / (t - b);
    m.values[10] = -2 / (l - p);
    m.values[12] = -(d + e) / (d - e);
    m.values[13] = -(t + b) / (t - b);
    m.values[14] = -(l + p) / (l - p);
    return m;
}

bool lume_mat4_inverse(LumeMat4 matriz, LumeMat4 *resultado)
{
    const float *m = matriz.values;
    float i[16], d;
    int n;
    if (!resultado)
        return false;
    i[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
           m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    i[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
           m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    i[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
           m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    i[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
            m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    i[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
           m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    i[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
           m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    i[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
           m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    i[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
            m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    i[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] -
           m[13] * m[3] * m[6];
    i[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
           m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    i[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
            m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    i[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
            m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    i[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] +
           m[9] * m[3] * m[6];
    i[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] -
           m[8] * m[3] * m[6];
    i[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] +
            m[8] * m[3] * m[5];
    i[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] -
            m[8] * m[2] * m[5];
    d = m[0] * i[0] + m[1] * i[4] + m[2] * i[8] + m[3] * i[12];
    if (fabsf(d) <= 0.000001f)
        return false;
    d = 1.0f / d;
    for (n = 0; n < 16; ++n)
        resultado->values[n] = i[n] * d;
    return true;
}

LumeVec3 lume_mat4_transform_point(LumeMat4 m, LumeVec3 p)
{
    return (LumeVec3){m.values[0] * p.x + m.values[4] * p.y + m.values[8] * p.z + m.values[12],
                      m.values[1] * p.x + m.values[5] * p.y + m.values[9] * p.z + m.values[13],
                      m.values[2] * p.x + m.values[6] * p.y + m.values[10] * p.z + m.values[14]};
}
LumeVec3 lume_matriz_transformar_direcao(LumeMat4 m, LumeVec3 v)
{
    return (LumeVec3){m.values[0] * v.x + m.values[4] * v.y + m.values[8] * v.z,
                      m.values[1] * v.x + m.values[5] * v.y + m.values[9] * v.z,
                      m.values[2] * v.x + m.values[6] * v.y + m.values[10] * v.z};
}

LumeAabb lume_aabb_empty(void)
{
    return (LumeAabb){{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
}
LumeAabb lume_aabb_expand_point(LumeAabb b, LumeVec3 p)
{
    b.min.x = fminf(b.min.x, p.x);
    b.min.y = fminf(b.min.y, p.y);
    b.min.z = fminf(b.min.z, p.z);
    b.max.x = fmaxf(b.max.x, p.x);
    b.max.y = fmaxf(b.max.y, p.y);
    b.max.z = fmaxf(b.max.z, p.z);
    return b;
}
LumeAabb lume_aabb_transform(LumeAabb b, LumeMat4 m)
{
    LumeAabb r = lume_aabb_empty();
    int i;
    for (i = 0; i < 8; ++i)
        r = lume_aabb_expand_point(
            r, lume_mat4_transform_point(m, (LumeVec3){(i & 1) ? b.max.x : b.min.x, (i & 2) ? b.max.y : b.min.y,
                                                       (i & 4) ? b.max.z : b.min.z}));
    return r;
}
bool lume_ray_intersect_aabb(LumeRay r, LumeAabb b, float *distancia)
{
    float tmin = 0.0f, tmax = FLT_MAX;
    int eixo;
    float o[3] = {r.origin.x, r.origin.y, r.origin.z}, d[3] = {r.direction.x, r.direction.y, r.direction.z};
    float mn[3] = {b.min.x, b.min.y, b.min.z}, mx[3] = {b.max.x, b.max.y, b.max.z};
    for (eixo = 0; eixo < 3; ++eixo)
    {
        if (fabsf(d[eixo]) < 0.000001f)
        {
            if (o[eixo] < mn[eixo] || o[eixo] > mx[eixo])
                return false;
        }
        else
        {
            float a = (mn[eixo] - o[eixo]) / d[eixo], c = (mx[eixo] - o[eixo]) / d[eixo], troca;
            if (a > c)
            {
                troca = a;
                a = c;
                c = troca;
            }
            tmin = fmaxf(tmin, a);
            tmax = fminf(tmax, c);
            if (tmin > tmax)
                return false;
        }
    }
    if (distancia)
        *distancia = tmin;
    return true;
}

LumeFrustum lume_frustum_from_matrix(LumeMat4 m)
{
    LumeFrustum f;
    int i;
    float *a = m.values;
    float p[6][4] = {{a[3] + a[0], a[7] + a[4], a[11] + a[8], a[15] + a[12]},
                     {a[3] - a[0], a[7] - a[4], a[11] - a[8], a[15] - a[12]},
                     {a[3] + a[1], a[7] + a[5], a[11] + a[9], a[15] + a[13]},
                     {a[3] - a[1], a[7] - a[5], a[11] - a[9], a[15] - a[13]},
                     {a[3] + a[2], a[7] + a[6], a[11] + a[10], a[15] + a[14]},
                     {a[3] - a[2], a[7] - a[6], a[11] - a[10], a[15] - a[14]}};
    for (i = 0; i < 6; ++i)
    {
        float l = sqrtf(p[i][0] * p[i][0] + p[i][1] * p[i][1] + p[i][2] * p[i][2]);
        f.planes[i].normal = (LumeVec3){p[i][0] / l, p[i][1] / l, p[i][2] / l};
        f.planes[i].distance = p[i][3] / l;
    }
    return f;
}
bool lume_frustum_intersects_aabb(LumeFrustum f, LumeAabb b)
{
    int i;
    for (i = 0; i < 6; ++i)
    {
        LumePlane p = f.planes[i];
        LumeVec3 v = {p.normal.x >= 0 ? b.max.x : b.min.x, p.normal.y >= 0 ? b.max.y : b.min.y,
                      p.normal.z >= 0 ? b.max.z : b.min.z};
        if (lume_vec3_dot(p.normal, v) + p.distance < 0)
            return false;
    }
    return true;
}
