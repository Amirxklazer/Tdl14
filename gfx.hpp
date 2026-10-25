// gfx.hpp - 2D helpers, bitmap font, and a tiny software 3D pipeline drawn through SDL_RenderGeometry
#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include <cstdarg>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "font_atlas.h"

static const int SCR_W = 1280, SCR_H = 720;
static const int PANEL_X = 840;           // right-hand 2D panel starts here
static const int PW_ = 440;
static SDL_Renderer* R = nullptr;
static SDL_Texture* fontTex = nullptr;

struct Col { Uint8 r, g, b, a; };
static inline Col C(int r, int g, int b, int a = 255) { return Col{(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a}; }
static const Col ORANGE = {249, 115, 22, 255};
static const Col BLACK0 = {10, 8, 0, 255};
static const Col WHITE  = {255, 255, 255, 255};
static inline Col mulc(Col c, float k) { return C(std::min(255.f, c.r * k), std::min(255.f, c.g * k), std::min(255.f, c.b * k), c.a); }
static inline Col mixc(Col a, Col b, float t) { return C(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t); }
static inline Col alpha(Col c, int a) { c.a = (Uint8)a; return c; }
static inline Col fromHex(Uint32 h) { return C((h >> 16) & 255, (h >> 8) & 255, h & 255); }

// ---------------------------------------------------------------- font
static void initFont() {
    std::vector<Uint8> px((size_t)FONT_COLS * FONT_CW * FONT_ROWS * FONT_CH);
    size_t o = 0;
    for (int i = 0; i + 1 < FONT_RLE_LEN && o < px.size(); i += 2)
        for (int k = 0; k < FONT_RLE[i + 1] && o < px.size(); k++) px[o++] = FONT_RLE[i];
    int W = FONT_COLS * FONT_CW, H = FONT_ROWS * FONT_CH;
    std::vector<Uint32> argb((size_t)W * H);
    for (size_t i = 0; i < argb.size(); i++) argb[i] = ((Uint32)px[i] << 24) | 0x00FFFFFF;
    fontTex = SDL_CreateTexture(R, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
    SDL_UpdateTexture(fontTex, nullptr, argb.data(), W * 4);
    SDL_SetTextureBlendMode(fontTex, SDL_BLENDMODE_BLEND);
}
static inline float advFor(float h) { return h * (float)FONT_ADV / (float)FONT_CH * 1.02f; }
static int decodeUtf8(const char*& s) {
    unsigned char c = (unsigned char)*s++;
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && *s) { int r = ((c & 0x1F) << 6) | (*s++ & 0x3F); return r; }
    if ((c & 0xF0) == 0xE0 && s[0] && s[1]) { s += 2; return '?'; }
    if ((c & 0xF8) == 0xF0 && s[0] && s[1] && s[2]) { s += 3; return '?'; }
    return '?';
}
static float textW(const char* s, float h) {
    int n = 0; while (*s) { decodeUtf8(s); n++; }
    return n * advFor(h);
}
// align: 0 left, 1 center, 2 right
static void text(const char* s, float x, float y, float h, Col c, int align = 0) {
    float w = textW(s, h);
    if (align == 1) x -= w / 2; else if (align == 2) x -= w;
    SDL_SetTextureColorMod(fontTex, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(fontTex, c.a);
    float adv = advFor(h);
    float sc = h / FONT_CH;
    while (*s) {
        int cp = decodeUtf8(s);
        if (cp < 32 || cp > 255) cp = '?';
        int i = cp - 32;
        SDL_Rect src = {(i % FONT_COLS) * FONT_CW, (i / FONT_COLS) * FONT_CH, FONT_CW, FONT_CH};
        SDL_FRect dst = {x, y, FONT_CW * sc, h};
        if (cp != ' ') SDL_RenderCopyF(R, fontTex, &src, &dst);
        x += adv;
    }
}
static void textf(float x, float y, float h, Col c, int align, const char* fmt, ...) __attribute__((format(printf, 6, 7)));
static void textf(float x, float y, float h, Col c, int align, const char* fmt, ...) {
    char buf[512]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    text(buf, x, y, h, c, align);
}
// word wrap; returns number of lines drawn
static int textWrap(const char* s, float x, float y, float w, float h, Col c, int maxLines = 99) {
    char line[256]; line[0] = 0; int ll = 0; int lines = 0; float adv = advFor(h);
    int maxc = std::max(1, (int)(w / adv));
    std::string word, cur;
    auto flush = [&]() { if (lines < maxLines) text(cur.c_str(), x, y + lines * (h + 3), h, c); lines++; cur.clear(); };
    (void)line; (void)ll;
    const char* p = s;
    while (true) {
        if (*p == ' ' || *p == 0 || *p == '\n') {
            int wl = 0; { const char* q = word.c_str(); while (*q) { decodeUtf8(q); wl++; } }
            int cl = 0; { const char* q = cur.c_str(); while (*q) { decodeUtf8(q); cl++; } }
            if (cl && cl + 1 + wl > maxc) flush();
            if (!cur.empty()) cur += ' ';
            cur += word; word.clear();
            if (*p == '\n') flush();
            if (*p == 0) break;
        } else word += *p;
        p++;
    }
    if (!cur.empty()) flush();
    return lines;
}

// ---------------------------------------------------------------- 2D primitives
static void setc(Col c) { SDL_SetRenderDrawColor(R, c.r, c.g, c.b, c.a); }
static void fillRect(float x, float y, float w, float h, Col c) {
    setc(c); SDL_FRect r = {x, y, w, h}; SDL_RenderFillRectF(R, &r);
}
static void strokeRect(float x, float y, float w, float h, Col c, int t = 2) {
    fillRect(x, y, w, t, c); fillRect(x, y + h - t, w, t, c); fillRect(x, y, t, h, c); fillRect(x + w - t, y, t, h, c);
}
static void lineD(float x1, float y1, float x2, float y2, Col c, float th = 2) {
    float dx = x2 - x1, dy = y2 - y1, l = std::sqrt(dx * dx + dy * dy); if (l < 0.001f) return;
    float nx = -dy / l * th / 2, ny = dx / l * th / 2;
    SDL_Vertex v[4] = {{{x1 + nx, y1 + ny}, {c.r, c.g, c.b, c.a}, {0, 0}}, {{x1 - nx, y1 - ny}, {c.r, c.g, c.b, c.a}, {0, 0}},
                       {{x2 - nx, y2 - ny}, {c.r, c.g, c.b, c.a}, {0, 0}}, {{x2 + nx, y2 + ny}, {c.r, c.g, c.b, c.a}, {0, 0}}};
    int idx[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(R, nullptr, v, 4, idx, 6);
}
static void tri2(float x1, float y1, float x2, float y2, float x3, float y3, Col c) {
    SDL_Vertex v[3] = {{{x1, y1}, {c.r, c.g, c.b, c.a}, {0, 0}}, {{x2, y2}, {c.r, c.g, c.b, c.a}, {0, 0}}, {{x3, y3}, {c.r, c.g, c.b, c.a}, {0, 0}}};
    SDL_RenderGeometry(R, nullptr, v, 3, nullptr, 0);
}
static void poly(const std::vector<SDL_FPoint>& p, Col c) {
    if (p.size() < 3) return;
    std::vector<SDL_Vertex> v; v.reserve((p.size() - 2) * 3);
    for (size_t i = 1; i + 1 < p.size(); i++) {
        for (const SDL_FPoint* q : {&p[0], &p[i], &p[i + 1]}) { SDL_Vertex vv; vv.position = *q; vv.color = {c.r, c.g, c.b, c.a}; vv.tex_coord = {0, 0}; v.push_back(vv); }
    }
    SDL_RenderGeometry(R, nullptr, v.data(), (int)v.size(), nullptr, 0);
}
static void disc(float cx, float cy, float rx, float ry, Col c, int n = 20) {
    std::vector<SDL_FPoint> p;
    for (int i = 0; i < n; i++) { float a = i * 6.2831853f / n; p.push_back({cx + std::cos(a) * rx, cy + std::sin(a) * ry}); }
    poly(p, c);
}
static void vgrad(float x, float y, float w, float h, Col top, Col bot) {
    SDL_Vertex v[4] = {{{x, y}, {top.r, top.g, top.b, top.a}, {0, 0}}, {{x + w, y}, {top.r, top.g, top.b, top.a}, {0, 0}},
                       {{x + w, y + h}, {bot.r, bot.g, bot.b, bot.a}, {0, 0}}, {{x, y + h}, {bot.r, bot.g, bot.b, bot.a}, {0, 0}}};
    int idx[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(R, nullptr, v, 4, idx, 6);
}
static void scanlines(float alphaV) {
    std::vector<SDL_FRect> rs;
    for (int y = 0; y < SCR_H; y += 4) rs.push_back({0, (float)y, (float)SCR_W, 1});
    SDL_SetRenderDrawColor(R, 0, 0, 0, (Uint8)alphaV);
    SDL_RenderFillRectsF(R, rs.data(), (int)rs.size());
}

// ---------------------------------------------------------------- 3D math
struct V3 { float x, y, z; };
static inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline V3 operator*(V3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
static inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static inline V3 norm(V3 a) { float l = std::sqrt(dot(a, a)); return l > 1e-6f ? a * (1.f / l) : V3{0, 1, 0}; }

struct M3 { float m[9]; };
static inline M3 mI() { return M3{{1, 0, 0, 0, 1, 0, 0, 0, 1}}; }
static inline M3 mul(const M3& a, const M3& b) {
    M3 r; for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.m[i * 3 + j] = a.m[i * 3] * b.m[j] + a.m[i * 3 + 1] * b.m[3 + j] + a.m[i * 3 + 2] * b.m[6 + j]; return r;
}
static inline V3 mulv(const M3& a, V3 v) { return {a.m[0] * v.x + a.m[1] * v.y + a.m[2] * v.z, a.m[3] * v.x + a.m[4] * v.y + a.m[5] * v.z, a.m[6] * v.x + a.m[7] * v.y + a.m[8] * v.z}; }
static inline M3 rotX(float a) { float c = std::cos(a), s = std::sin(a); return M3{{1, 0, 0, 0, c, -s, 0, s, c}}; }
static inline M3 rotY(float a) { float c = std::cos(a), s = std::sin(a); return M3{{c, 0, s, 0, 1, 0, -s, 0, c}}; }
static inline M3 rotZ(float a) { float c = std::cos(a), s = std::sin(a); return M3{{c, -s, 0, s, c, 0, 0, 0, 1}}; }
struct Xf { M3 r; V3 t; };
static inline Xf xf(V3 t, M3 r = mI()) { return Xf{r, t}; }
static inline Xf compose(const Xf& p, const Xf& c) { return Xf{mul(p.r, c.r), p.t + mulv(p.r, c.t)}; }
static inline V3 apply(const Xf& x, V3 v) { return x.t + mulv(x.r, v); }

struct Cam {
    V3 pos{0, 5, 8}, target{0, 1, 0};
    float fov = 0.9f;
    float vx = 0, vy = 0, vw = 840, vh = 720;
    V3 fwd, right, up; float f;
    void update() {
        fwd = norm(target - pos); right = norm(cross(fwd, V3{0, 1, 0})); up = cross(right, fwd);
        f = (vh * 0.5f) / std::tan(fov * 0.5f);
    }
    bool project(V3 w, float& sx, float& sy, float& depth) const {
        V3 d = w - pos; float z = dot(d, fwd); depth = z; if (z < 0.1f) return false;
        sx = vx + vw * 0.5f + f * dot(d, right) / z; sy = vy + vh * 0.5f - f * dot(d, up) / z; return true;
    }
    V3 rayDir(float sx, float sy) const { return norm(fwd + right * ((sx - vx - vw * 0.5f) / f) + up * (-(sy - vy - vh * 0.5f) / f)); }
    bool hitPlaneY(float sx, float sy, float y, V3& out) const {
        V3 d = rayDir(sx, sy); if (std::fabs(d.y) < 1e-5f) return false;
        float t = (y - pos.y) / d.y; if (t < 0) return false; out = pos + d * t; return true;
    }
};

struct Tri { V3 p[3]; Col c[3]; float depth; int layer; };
static SDL_Texture* ssaaTex = nullptr; static int ssaaW = 0, ssaaH = 0;
static const float SSAA = 1.5f;
struct Scene3D {
    std::vector<Tri> tris; float depthBias = 0; Cam cam; float light = 1.f; Col tint = {255, 255, 255, 255};
    Col hemiTop = {255, 255, 255, 255}, hemiBot = {150, 140, 135, 255};
    V3 L = norm(V3{-0.45f, 0.85f, 0.5f});
    void clear() { tris.clear(); depthBias = 0; }
    Col lit(Col col, V3 n, V3 mid, bool emissive) const {
        if (emissive) return col;
        float lam = std::max(0.f, dot(n, L));
        float hy = n.y * 0.5f + 0.5f;
        float hr = hemiBot.r + (hemiTop.r - hemiBot.r) * hy, hg = hemiBot.g + (hemiTop.g - hemiBot.g) * hy, hb = hemiBot.b + (hemiTop.b - hemiBot.b) * hy;
        float ndv = std::max(0.f, dot(n, norm(cam.pos - mid))); float rim = std::pow(1.f - ndv, 3.f) * 0.16f;
        float r = (col.r / 255.f) * (hr / 255.f * 0.64f + 0.52f * lam * 1.0f) + rim, g = (col.g / 255.f) * (hg / 255.f * 0.64f + 0.52f * lam * 0.96f) + rim, b = (col.b / 255.f) * (hb / 255.f * 0.64f + 0.52f * lam * 0.88f) + rim;
        float k = light * 255.f;
        return C(std::min(255.f, r * k * tint.r / 255.f), std::min(255.f, g * k * tint.g / 255.f), std::min(255.f, b * k * tint.b / 255.f), col.a);
    }
    void addTri(V3 a, V3 b, V3 c, Col col, V3 center, bool twoSided = false, int layer = 1, bool emissive = false, const V3* nrm = nullptr) {
        V3 fn = norm(cross(b - a, c - a));
        V3 mid = (a + b + c) * (1.f / 3.f);
        if (!twoSided) { if (dot(fn, mid - center) < 0) fn = fn * -1.f; }
        else if (dot(fn, cam.pos - mid) < 0) fn = fn * -1.f;
        if (dot(fn, cam.pos - mid) <= 0) return;                // back-face cull
        Tri t; t.p[0] = a; t.p[1] = b; t.p[2] = c;
        if (nrm) { V3 vs[3] = {a, b, c}; for (int i = 0; i < 3; i++) t.c[i] = lit(col, nrm[i], vs[i], emissive); }
        else { Col o = lit(col, fn, mid, emissive); t.c[0] = t.c[1] = t.c[2] = o; }
        t.depth = dot(mid - cam.pos, cam.fwd) + depthBias; t.layer = layer;
        tris.push_back(t);
    }
    void box(const Xf& x, V3 h, Col c, int layer = 1, bool emissive = false) {
        V3 v[8]; int k = 0;
        for (int i = 0; i < 8; i++) { V3 l = {(i & 1) ? h.x : -h.x, (i & 2) ? h.y : -h.y, (i & 4) ? h.z : -h.z}; v[k++] = apply(x, l); }
        static const int F[6][4] = {{0, 1, 3, 2}, {4, 5, 7, 6}, {0, 1, 5, 4}, {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 3, 7, 5}};
        for (auto& f : F) { addTri(v[f[0]], v[f[1]], v[f[2]], c, x.t, false, layer, emissive); addTri(v[f[0]], v[f[2]], v[f[3]], c, x.t, false, layer, emissive); }
    }
    void sphere(const Xf& x, V3 rad, Col c, int lon = 12, int lat = 8, int layer = 1, bool emissive = false) {
        for (int j = 0; j < lat; j++) {
            float a0 = M_PI * j / lat, a1 = M_PI * (j + 1) / lat;
            for (int i = 0; i < lon; i++) {
                float b0 = 6.2831853f * i / lon, b1 = 6.2831853f * (i + 1) / lon;
                auto N = [&](float a, float b) { return norm(mulv(x.r, V3{std::sin(a) * std::cos(b) / std::max(rad.x, 1e-4f), std::cos(a) / std::max(rad.y, 1e-4f), std::sin(a) * std::sin(b) / std::max(rad.z, 1e-4f)})); };
                auto P = [&](float a, float b) { return apply(x, V3{rad.x * std::sin(a) * std::cos(b), rad.y * std::cos(a), rad.z * std::sin(a) * std::sin(b)}); };
                V3 p00 = P(a0, b0), p01 = P(a0, b1), p10 = P(a1, b0), p11 = P(a1, b1);
                V3 n00 = N(a0, b0), n01 = N(a0, b1), n10 = N(a1, b0), n11 = N(a1, b1);
                if (j > 0) { V3 ns[3] = {n00, n01, n11}; addTri(p00, p01, p11, c, x.t, false, layer, emissive, ns); }
                if (j < lat - 1) { V3 ns[3] = {n00, n11, n10}; addTri(p00, p11, p10, c, x.t, false, layer, emissive, ns); }
            }
        }
    }
    void cyl(const Xf& x, float r, float hh, Col c, int n = 14, int layer = 1, float r2 = -1) {
        if (r2 < 0) r2 = r;
        for (int i = 0; i < n; i++) {
            float a0 = 6.2831853f * i / n, a1 = 6.2831853f * (i + 1) / n;
            V3 b0 = apply(x, {r * std::cos(a0), -hh, r * std::sin(a0)}), b1 = apply(x, {r * std::cos(a1), -hh, r * std::sin(a1)});
            V3 t0 = apply(x, {r2 * std::cos(a0), hh, r2 * std::sin(a0)}), t1 = apply(x, {r2 * std::cos(a1), hh, r2 * std::sin(a1)});
            V3 tc = apply(x, {0, hh, 0}), bc = apply(x, {0, -hh, 0});
            V3 n0 = norm(mulv(x.r, V3{std::cos(a0), (r - r2) / std::max(2.f * hh, 1e-3f), std::sin(a0)})), n1 = norm(mulv(x.r, V3{std::cos(a1), (r - r2) / std::max(2.f * hh, 1e-3f), std::sin(a1)}));
            V3 s1[3] = {n0, n1, n1}, s2[3] = {n0, n1, n0};
            addTri(b0, b1, t1, c, x.t, false, layer, false, s1); addTri(b0, t1, t0, c, x.t, false, layer, false, s2);
            if (r2 > 0.001f) addTri(tc, t0, t1, c, x.t, false, layer);
            addTri(bc, b1, b0, c, x.t, false, layer);
        }
    }
    void quad(V3 a, V3 b, V3 c, V3 d, Col col, int layer = 0, bool emissive = false) {
        addTri(a, b, c, col, a, true, layer, emissive); addTri(a, c, d, col, a, true, layer, emissive);
    }
    void shadow(float x, float z, float r, float y = 0.012f) {            // soft blob shadow on the floor
        float sb = depthBias; depthBias = 10;
        for (int ring = 0; ring < 2; ring++) {
            float rr_ = r * (ring ? 0.6f : 1.f); int a = ring ? 70 : 55; int n = 14;
            for (int i = 0; i < n; i++) { float a0 = i * 6.2831853f / n, a1 = (i + 1) * 6.2831853f / n;
                addTri({x, y + ring * 0.001f, z}, {x + std::cos(a0) * rr_, y + ring * 0.001f, z + std::sin(a0) * rr_ * 0.75f}, {x + std::cos(a1) * rr_, y + ring * 0.001f, z + std::sin(a1) * rr_ * 0.75f}, C(0, 0, 10, a), {x, -5, z}, true, 0, true); }
        }
        depthBias = sb;
    }
    void render() {
        std::stable_sort(tris.begin(), tris.end(), [](const Tri& a, const Tri& b) { if (a.layer != b.layer) return a.layer < b.layer; return a.depth > b.depth; });
        int tw = (int)(cam.vw * SSAA), th = (int)(cam.vh * SSAA);
        bool useSS = true;
        if (!ssaaTex || ssaaW != tw || ssaaH != th) {
            if (ssaaTex) SDL_DestroyTexture(ssaaTex);
            ssaaTex = SDL_CreateTexture(R, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, tw, th); ssaaW = tw; ssaaH = th;
            if (ssaaTex) SDL_SetTextureBlendMode(ssaaTex, SDL_BLENDMODE_BLEND);
        }
        useSS = ssaaTex != nullptr;
        float k = useSS ? SSAA : 1.f;
        std::vector<SDL_Vertex> vs; vs.reserve(tris.size() * 3);
        for (auto& t : tris) {
            float sx[3], sy[3], d; bool ok = true;
            for (int i = 0; i < 3; i++) if (!cam.project(t.p[i], sx[i], sy[i], d)) { ok = false; break; }
            if (!ok) continue;
            for (int i = 0; i < 3; i++) { SDL_Vertex v; v.position = {(sx[i] - (useSS ? cam.vx : 0)) * k, (sy[i] - (useSS ? cam.vy : 0)) * k}; v.color = {t.c[i].r, t.c[i].g, t.c[i].b, t.c[i].a}; v.tex_coord = {0, 0}; vs.push_back(v); }
        }
        if (useSS) {
            SDL_SetRenderTarget(R, ssaaTex); SDL_SetRenderDrawColor(R, 0, 0, 0, 0); SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_NONE); SDL_RenderClear(R); SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND);
            if (!vs.empty()) SDL_RenderGeometry(R, nullptr, vs.data(), (int)vs.size(), nullptr, 0);
            SDL_SetRenderTarget(R, nullptr);
            SDL_Rect dst = {(int)cam.vx, (int)cam.vy, (int)cam.vw, (int)cam.vh}; SDL_RenderCopy(R, ssaaTex, nullptr, &dst);
        } else {
            SDL_Rect clip = {(int)cam.vx, (int)cam.vy, (int)cam.vw, (int)cam.vh};
            SDL_RenderSetClipRect(R, &clip);
            if (!vs.empty()) SDL_RenderGeometry(R, nullptr, vs.data(), (int)vs.size(), nullptr, 0);
            SDL_RenderSetClipRect(R, nullptr);
        }
    }
};
