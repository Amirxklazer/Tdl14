// scene3d.hpp - island panorama, room interiors, and low-poly characters
#pragma once
#include "world.hpp"

static Scene3D S3;
static const float PI_ = 3.14159265f;

// ---------------------------------------------------------------- sky
static void skyColors(Col& top, Col& bot, float& light) {
    float h = G.minute / 60.f;
    struct K { float h; Col t, b; float l; };
    static const K ks[] = {{0, C(8, 8, 30), C(24, 18, 56), 0.48f}, {5.5f, C(30, 24, 70), C(210, 110, 70), 0.6f}, {7.5f, C(90, 150, 235), C(250, 200, 160), 0.95f}, {11, C(70, 150, 240), C(190, 225, 250), 1.f},
                           {16, C(80, 150, 235), C(200, 225, 250), 1.f}, {18.5f, C(70, 70, 150), C(250, 130, 60), 0.8f}, {20.5f, C(20, 16, 55), C(90, 40, 80), 0.55f}, {24, C(8, 8, 30), C(24, 18, 56), 0.48f}};
    for (int i = 0; i < 7; i++) if (h >= ks[i].h && h <= ks[i + 1].h) {
        float t = (h - ks[i].h) / (ks[i + 1].h - ks[i].h);
        top = mixc(ks[i].t, ks[i + 1].t, t); bot = mixc(ks[i].b, ks[i + 1].b, t); light = ks[i].l + (ks[i + 1].l - ks[i].l) * t; return;
    }
    top = ks[0].t; bot = ks[0].b; light = 1.f;
}
static void drawSkyBackdrop(float x, float y, float w, float h, float tAnim) {
    Col t, b; float l; skyColors(t, b, l);
    vgrad(x, y, w, h, t, b);
    SDL_Rect clip = {(int)x, (int)y, (int)w, (int)h}; SDL_RenderSetClipRect(R, &clip);
    float hr = G.minute / 60.f;
    if (isNight()) {
        for (int i = 0; i < 60; i++) { float sx = x + std::fmod(i * 197.31f, w), sy = y + std::fmod(i * 83.7f, h * 0.6f); float tw = 0.5f + 0.5f * std::sin(tAnim * 2 + i); fillRect(sx, sy, 2, 2, C(255, 255, 230, (int)(90 + 140 * tw))); }
        float mx = x + w * ((hr < 12 ? hr + 24 : hr) - 19.f) / 12.f; float my = y + h * 0.35f - std::sin(((hr < 12 ? hr + 24 : hr) - 19.f) / 12.f * PI_) * h * 0.22f;
        disc(mx, my, 34, 34, C(240, 235, 210)); disc(mx + 12, my - 6, 30, 30, mixc(t, b, 0.1f));
    } else {
        float f = (hr - 6.f) / 13.f; float sx = x + w * f, sy = y + h * 0.4f - std::sin(f * PI_) * h * 0.3f;
        disc(sx, sy, 40, 40, C(255, 224, 140, 90)); disc(sx, sy, 28, 28, C(255, 235, 170));
    }
    SDL_RenderSetClipRect(R, nullptr);
}

// ---------------------------------------------------------------- characters
struct CharPose {
    float phase = 0, amp = 0; int expr = 0; bool lying = false; float bounce = 0;
    bool talk = false, eat = false, held = false, sad = false, cheer = false; float sway = 0, lookYaw = 0, scaleMul = 1, t = 0;
};
static void drawHair(Scene3D& S, const Xf& head, float rh, int style, Col hc, bool hasHat, float swing) {
    Col hd = mulc(hc, 0.82f);
    S.sphere(compose(head, xf({0, 0.2f * rh, -0.2f * rh})), {1.06f * rh, 0.9f * rh, 1.0f * rh}, hc, 12, 6);
    if (style != 4) S.sphere(compose(head, xf({0, 0.66f * rh, 0.5f * rh})), {0.9f * rh, 0.34f * rh, 0.5f * rh}, hd, 10, 4);     // fringe
    switch (style) {
        case 1: for (int i = 0; i < 6; i++) { float a = (i - 2.5f) * 0.5f; S.cyl(compose(head, xf({std::sin(a) * 0.75f * rh, 1.0f * rh, -0.05f * rh}, rotZ(-a))), 0.2f * rh, 0.34f * rh, hc, 6, 1, 0.01f); } break;
        case 2: { Xf bh = compose(head, xf({0, 0.2f * rh, -0.55f * rh}, rotX(swing))); S.sphere(compose(bh, xf({0, -0.75f * rh, -0.05f * rh})), {0.95f * rh, 1.05f * rh, 0.34f * rh}, hc, 10, 5); } break;
        case 3: if (!hasHat) { Xf bh = compose(head, xf({0, 1.1f * rh, -0.25f * rh}, rotX(swing * 0.6f))); S.sphere(compose(bh, xf({0, 0.15f * rh, 0})), {0.4f * rh, 0.4f * rh, 0.4f * rh}, hc, 8, 5); } break;
        case 5: for (int i = 0; i < 8; i++) { float a = i * 0.785f; S.sphere(compose(head, xf({std::cos(a) * 0.82f * rh, 0.55f * rh + 0.12f * std::sin(a * 2), std::sin(a) * 0.82f * rh - 0.1f * rh})), {0.32f * rh, 0.32f * rh, 0.32f * rh}, hc, 6, 4); } break;
        default: break;
    }
}
static inline float posf(float v) { return v > 0 ? v : 0; }
static void drawCharacter(Scene3D& S, int ci, V3 pos, float yaw, const CharPose& P, bool mini = false) {
    const CDef& D = ROSTER[ci]; Char& c = G.ch[ci];
    float s = ageScale(D.age) * P.scaleMul; float hs = std::max(1.0f, 1.42f - D.age * 0.022f);
    Col skin = fromHex(SKINS[D.skin]), hair = fromHex(HAIRS[D.hair]);
    uint32_t topC = c.outfit[SL_TOP] >= 0 ? ITEMS[c.outfit[SL_TOP]].color : D.top, botC = c.outfit[SL_BOTTOM] >= 0 ? ITEMS[c.outfit[SL_BOTTOM]].color : D.bottom;
    Col top = fromHex(topC), bot = fromHex(botC);
    if (c.glitch > 0 && (SDL_GetTicks() / 70) % 3 == 0) { top = C(255 - top.r, 255 - top.g, 255 - top.b); }
    float t = P.t; float a = P.amp; float ph = P.phase;
    if (!P.lying) S.shadow(pos.x, pos.z, 0.42f * s + 0.08f, 0.013f);
    auto sc = [&](V3 v) { return V3{v.x * s, v.y * s, v.z * s}; };
    if (mini) {
        Xf root = xf(pos + V3{0, std::fabs(std::sin(t * 4 + ci)) * 0.03f, 0}, rotY(yaw));
        S.cyl(compose(root, xf(sc({0, 0.55f, 0}))), 0.2f * s, 0.55f * s, top, 8, 1, 0.16f * s);
        S.sphere(compose(root, xf(sc({0, 1.3f, 0}))), sc({0.28f, 0.28f, 0.28f}), skin, 8, 5);
        S.sphere(compose(root, xf(sc({0, 1.38f, -0.04f}))), sc({0.3f, 0.23f, 0.29f}), hair, 8, 4);
        return;
    }
    bool longSleeve = c.outfit[SL_TOP] == 16 || c.outfit[SL_TOP] == 17 || ci % 3 == 0;
    bool shorts = c.outfit[SL_BOTTOM] < 0 && (ci % 4 == 1 || D.age < 9);
    Col shoe = C(34, 34, 44); Col shirtDark = mulc(top, 0.8f);
    float sinp = std::sin(ph), cosp = std::cos(ph);
    float idleW = 1.f - a;
    // ---------------- body motion
    float lean = 0.07f * a + (P.sad ? 0.17f : 0.f) - (P.held ? 0.04f : 0.f);
    float bob = -0.04f * a * std::fabs(sinp) + std::sin(t * 1.9f + ci) * 0.006f + P.bounce;
    if (P.lying) bob = 0;
    float breath = 0.5f + 0.5f * std::sin(t * (P.lying ? 1.2f : 1.9f) + ci);
    Xf root = xf(pos, mul(rotY(yaw), P.lying ? rotZ(PI_ / 2) : mI()));
    float sway = std::sin(t * 0.7f + ci) * 0.02f * idleW;
    Xf pelvis = compose(root, xf(sc({sway + 0.018f * a * sinp, 0.63f + bob, 0}), mul(rotY(-0.13f * a * sinp), rotZ(-0.04f * a * sinp + sway * 1.5f))));
    Xf spine = compose(pelvis, xf(sc({0, 0.06f, 0}), mul(rotX(lean), rotY(0.2f * a * sinp))));
    // ---------------- legs (hip -> knee -> ankle)
    for (int k = 0; k < 2; k++) {
        float sd = k ? 1.f : -1.f; float psi = ph + (k ? PI_ : 0.f);
        float th = -0.62f * a * std::sin(psi), kn = 0.06f + 0.9f * a * posf(std::cos(psi));
        th += idleW * 0.03f * sd * std::sin(t * 0.7f); kn += idleW * 0.04f * (k ? posf(std::sin(t * 0.7f)) : posf(-std::sin(t * 0.7f)));
        if (P.held) { th = -0.12f + (P.sway * 0.55f + std::sin(t * 9.f + k * 1.7f) * 0.14f) * sd; kn = 0.2f + 0.12f * std::sin(t * 9.f + k * 1.7f + 1.f); }
        if (P.cheer) { th = -0.5f; kn = 0.9f; }
        if (P.lying) { th = -0.04f; kn = 0.12f + 0.1f * k; }
        Xf hip = compose(pelvis, xf(sc({0.13f * sd, -0.05f, 0}), mul(rotX(th), rotZ(0.03f * sd))));
        Col legC = bot;
        S.sphere(hip, sc({0.115f, 0.115f, 0.115f}), bot, 7, 4);
        S.cyl(compose(hip, xf(sc({0, -0.15f, 0}))), 0.092f * s, 0.15f * s, bot, 8, 1, 0.115f * s);                // thigh
        Xf knee = compose(hip, xf(sc({0, -0.3f, 0}), rotX(kn)));
        S.sphere(knee, sc({0.09f, 0.09f, 0.09f}), shorts ? skin : legC, 6, 4);
        S.cyl(compose(knee, xf(sc({0, -0.14f, 0}))), 0.07f * s, 0.14f * s, shorts ? skin : legC, 8, 1, 0.09f * s);  // shin
        Xf ank = compose(knee, xf(sc({0, -0.28f, 0}), rotX(-(th + kn) * 0.75f)));
        S.sphere(compose(ank, xf(sc({0, -0.035f, 0.055f}))), sc({0.11f, 0.065f, 0.18f}), shoe, 8, 4);          // shoe
        S.box(compose(ank, xf(sc({0, -0.075f, 0.06f}))), sc({0.095f, 0.012f, 0.17f}), C(235, 235, 240));        // sole
    }
    // ---------------- torso
    S.sphere(compose(pelvis, xf(sc({0, -0.02f, 0}))), sc({0.255f, 0.15f, 0.17f}), bot, 8, 5);
    S.sphere(compose(pelvis, xf(sc({0, 0.1f, 0}))), sc({0.235f, 0.18f, 0.165f}), top, 8, 5);
    float chestK = 1.f + 0.022f * breath;
    S.sphere(compose(spine, xf(sc({0, 0.25f, 0}))), sc({0.285f * chestK, 0.31f * chestK, 0.19f * chestK}), top, 12, 7);
    if (c.outfit[SL_TOP] == 16) S.sphere(compose(spine, xf(sc({0, 0.5f, -0.13f}))), sc({0.24f, 0.13f, 0.13f}), top, 8, 4);      // hood
    S.cyl(compose(spine, xf(sc({0, 0.52f, 0}))), 0.07f * s, 0.07f * s, skin, 8);                                             // neck
    // ---------------- arms (shoulder -> elbow -> wrist)
    for (int k = 0; k < 2; k++) {
        float sd = k ? 1.f : -1.f; float psi = ph + (k ? PI_ : 0.f);
        float sh = 0.5f * a * std::sin(psi) + idleW * (0.06f + 0.05f * std::sin(t * 1.3f + k * 2.f)); float el = -(0.2f + 0.55f * a * posf(-std::sin(psi)) + idleW * 0.05f * std::sin(t * 1.3f + k));
        float outZ = 0.12f * sd;
        if (P.sad) { sh = 0.15f; el = -0.1f; }
        if (P.talk && k == 1) { sh = -0.75f + 0.35f * std::sin(t * 5.2f); el = -1.1f + 0.4f * std::sin(t * 6.3f); outZ = 0.3f; }
        if (P.eat && k == 1) { sh = -1.5f + 0.12f * std::sin(t * 10.f); el = -1.7f; }
        if (P.cheer) { sh = -2.75f + 0.2f * std::sin(t * 14.f + k); el = -0.25f; outZ = 0.28f * sd; }
        if (P.held) { sh = -2.3f + 0.35f * std::sin(t * 11.f + k * 2.f); el = -0.3f; outZ = 0.32f * sd; }
        if (P.lying) { sh = 0.1f; el = -0.3f; }
        Xf shj = compose(spine, xf(sc({0.35f * sd, 0.44f, 0}), mul(rotX(sh), rotZ(outZ))));
        S.sphere(shj, sc({0.105f, 0.105f, 0.105f}), top, 7, 4);
        S.cyl(compose(shj, xf(sc({0, -0.12f, 0}))), 0.07f * s, 0.12f * s, top, 8, 1, 0.085f * s);                    // upper arm
        Xf elj = compose(shj, xf(sc({0, -0.24f, 0}), rotX(el)));
        S.sphere(elj, sc({0.07f, 0.07f, 0.07f}), longSleeve ? top : skin, 6, 4);
        S.cyl(compose(elj, xf(sc({0, -0.11f, 0}))), 0.055f * s, 0.11f * s, longSleeve ? shirtDark : skin, 8, 1, 0.07f * s);       // forearm
        Xf wr = compose(elj, xf(sc({0, -0.23f, 0})));
        S.sphere(wr, sc({0.085f, 0.09f, 0.075f}), skin, 6, 4);
        S.sphere(compose(wr, xf(sc({0.06f * sd, 0.0f, 0.03f}))), sc({0.035f, 0.05f, 0.035f}), skin, 4, 3);        // thumb
        if (k == 1 && c.held >= 0) S.box(compose(wr, xf(sc({0, -0.07f, 0.1f}))), sc({0.1f, 0.1f, 0.06f}), fromHex(ITEMS[c.held].color), 1);
    }
    // ---------------- head
    float rh = 0.3f * hs;
    float pitch = 0.04f * std::sin(t * 1.1f) + (P.sad ? 0.34f : 0.f) - lean * 0.85f + (P.eat ? -0.22f : 0.f) + (P.talk ? 0.06f * std::sin(t * 6.f) : 0.f) + 0.03f * a * std::cos(2 * ph);
    float tilt = P.lying ? 0.f : std::sin(t * 1.3f + ci * 2.f) * 0.05f + (P.sad ? 0.06f : 0.f);
    Xf head = compose(spine, xf(sc({0, 0.52f + rh * 0.95f + 0.02f, 0}), mul(rotY(P.lookYaw), mul(rotX(pitch), rotZ(tilt)))));
    S.sphere(head, sc({rh, rh * 0.96f, rh * 0.97f}), skin, 14, 9);
    float e = rh * s; int hat = c.outfit[SL_HAT];
    for (int k = 0; k < 2; k++) { float sd = k ? 1.f : -1.f; S.sphere(compose(head, xf({0.97f * e * sd, -0.02f * e, 0})), {0.14f * e, 0.2f * e, 0.12f * e}, mulc(skin, 0.95f), 6, 4); }
    drawHair(S, head, e, D.hairStyle, hair, hat >= 0, 0.12f * a * sinp - 0.05f - (P.held ? 0.3f : 0.f));
    Col ink = C(26, 22, 34); bool blink = std::fmod(t + ci * 1.37f, 4.2f) < 0.13f;
    bool glitchEye = c.glitch > 0 && (SDL_GetTicks() / 90) % 2 == 0;
    Col pupilC = glitchEye ? C(249, 115, 22) : C(36, 28, 46);
    int expr = P.held ? 5 : P.expr; bool mouthOpen = P.talk && std::sin(t * 15.f) > 0.1f;
    for (int k = 0; k < 2; k++) {
        float sd = k ? 1.f : -1.f; Xf ex = compose(head, xf({0.37f * e * sd, 0.09f * e, 0.92f * e}));
        bool closed = expr == 4 || blink;
        if (closed) S.box(ex, {0.17f * e, 0.025f * e, 0.03f * e}, ink);
        else if (expr == 1) { S.box(compose(ex, xf({-0.065f * e, 0.0f, 0}, rotZ(0.5f))), {0.085f * e, 0.03f * e, 0.03f * e}, ink); S.box(compose(ex, xf({0.065f * e, 0.0f, 0}, rotZ(-0.5f))), {0.085f * e, 0.03f * e, 0.03f * e}, ink); }
        else {
            float w = expr == 5 ? 1.2f : 1.f;
            S.box(ex, {0.125f * e * w, 0.15f * e * w, 0.028f * e}, C(250, 250, 255));
            S.box(compose(ex, xf({0.0f, -0.01f * e, 0.014f * e})), {0.075f * e, 0.11f * e, 0.02f * e}, pupilC);
            S.box(compose(ex, xf({0.03f * e, 0.045f * e, 0.03f * e})), {0.026f * e, 0.03f * e, 0.01f * e}, C(255, 255, 255), 1, true);
        }
        S.box(compose(ex, xf({0, (expr == 5 ? 0.3f : 0.24f) * e, 0.0f}, rotZ((expr == 2 ? 0.5f : expr == 3 ? -0.5f : 0.f) * sd))), {0.14f * e, 0.022f * e, 0.026f * e}, mulc(hair, 0.9f));
        if (expr == 1) S.box(compose(head, xf({0.6f * e * sd, -0.2f * e, 0.8f * e})), {0.12f * e, 0.07f * e, 0.02f * e}, C(255, 130, 150), 1, true);
    }
    S.sphere(compose(head, xf({0, -0.12f * e, 0.97f * e})), {0.07f * e, 0.06f * e, 0.06f * e}, mulc(skin, 0.9f), 6, 4);
    Xf mo = compose(head, xf({0, -0.38f * e, 0.88f * e}));
    if (expr == 5 || mouthOpen || P.eat) S.sphere(mo, {0.09f * e, (mouthOpen || P.eat ? 0.06f + 0.03f * std::sin(t * 15.f) : 0.1f) * e, 0.04f * e}, C(110, 26, 38), 8, 4);
    else if (expr == 1) { S.box(mo, {0.17f * e, 0.075f * e, 0.03f * e}, C(110, 26, 38)); S.box(compose(mo, xf({0, 0.04f * e, 0.012f * e})), {0.13f * e, 0.02f * e, 0.02f * e}, C(250, 250, 250)); }
    else if (expr == 2) S.box(compose(mo, xf({0, -0.04f * e, 0})), {0.1f * e, 0.022f * e, 0.03f * e}, ink);
    else if (expr == 3) S.box(mo, {0.13f * e, 0.022f * e, 0.03f * e}, ink);
    else if (expr == 4) S.sphere(mo, {0.05f * e, 0.04f * e, 0.03f * e}, C(120, 40, 50), 6, 4);
    else S.box(mo, {0.085f * e, 0.02f * e, 0.03f * e}, ink);
    int gl = c.outfit[SL_GLASSES];
    if (gl >= 0) {
        bool sun = gl == 15; Col gc = sun ? C(5, 5, 8) : C(40, 40, 60);
        for (int k = 0; k < 2; k++) { float sd = k ? 1.f : -1.f; S.box(compose(head, xf({0.37f * e * sd, 0.09f * e, 1.0f * e})), {0.22f * e, sun ? 0.16f * e : 0.2f * e, 0.02f * e}, gc);
            if (!sun) S.box(compose(head, xf({0.37f * e * sd, 0.09f * e, 1.018f * e})), {0.17f * e, 0.15f * e, 0.01f * e}, C(190, 225, 250, 120), 1, true); }
        S.box(compose(head, xf({0, 0.12f * e, 0.99f * e})), {0.18f * e, 0.02f * e, 0.02f * e}, gc);
    }
    if (hat >= 0) {
        Col hc = fromHex(ITEMS[hat].color);
        if (hat == 11) { S.sphere(compose(head, xf({0, 0.46f * e, -0.05f * e})), {1.09f * e, 0.62f * e, 1.09f * e}, hc, 12, 5); S.box(compose(head, xf({0, 0.42f * e, 0.96f * e})), {0.72f * e, 0.04f * e, 0.44f * e}, mulc(hc, 0.85f)); }
        else if (hat == 12) S.sphere(compose(head, xf({0, 0.42f * e, -0.02f * e})), {1.12f * e, 0.78f * e, 1.12f * e}, hc, 12, 6);
        else if (hat == 13) { S.cyl(compose(head, xf({0, 0.97f * e, 0})), 0.8f * e, 0.18f * e, hc, 10, 1, 0.95f * e); for (int i = 0; i < 5; i++) { float an = i * 1.2566f; S.cyl(compose(head, xf({std::cos(an) * 0.9f * e, 1.27f * e, std::sin(an) * 0.9f * e})), 0.12f * e, 0.18f * e, hc, 6, 1, 0.01f); S.sphere(compose(head, xf({std::cos(an) * 0.9f * e, 1.46f * e, std::sin(an) * 0.9f * e})), {0.05f * e, 0.05f * e, 0.05f * e}, C(236, 72, 153), 5, 3, 1, true); } }
    }
}
static float charHeight(int ci) { float hs = std::max(1.0f, 1.42f - ROSTER[ci].age * 0.022f); return ageScale(ROSTER[ci].age) * (1.2f + 0.3f * hs * 1.95f) + 0.12f; }

static void drawBot(Scene3D& S, int ci, V3 pos, float t) {
    Col body = mixc(fromHex(ROSTER[ci].top), C(200, 205, 215), 0.6f);
    float hov = 0.22f + 0.04f * std::sin(t * 3 + ci);
    S.box(xf(pos + V3{0, hov, 0}), {0.17f, 0.09f, 0.17f}, body);
    S.sphere(xf(pos + V3{0, hov + 0.1f, 0}), {0.13f, 0.1f, 0.13f}, mulc(body, 0.85f), 6, 3);
    S.box(xf(pos + V3{0, hov + 0.08f, 0.14f}), {0.06f, 0.025f, 0.03f}, C(249, 115, 22), 1, true);
}

// ---------------------------------------------------------------- room interiors
static float roomTheme(int room, Col& wall, Col& floorA, Col& floorB) {
    if (room == ROOM_LOBBY) { wall = C(70, 70, 82); floorA = C(150, 150, 160); floorB = C(125, 125, 138); return 1.f; }
    if (room == ROOM_ICE) { wall = C(250, 190, 210); floorA = C(255, 245, 250); floorB = C(250, 205, 225); return 1.f; }
    if (room == ROOM_PARK) { wall = C(60, 60, 66); floorA = C(70, 70, 76); floorB = C(64, 64, 70); return 1.f; }
    Col t = fromHex(ROSTER[room].top); wall = mixc(t, C(235, 230, 220), 0.72f); floorA = C(176, 132, 90); floorB = C(160, 118, 80); return 1.f;
}
static void drawPropMesh(Scene3D& S, int item, V3 p) {
    Col c = fromHex(ITEMS[item].color); int cat = ITEMS[item].cat;
    if (cat == CAT_FOOD) { S.cyl(xf(p + V3{0, 0.04f, 0}), 0.22f, 0.04f, C(240, 240, 240), 8); S.sphere(xf(p + V3{0, 0.14f, 0}), {0.14f, 0.1f, 0.14f}, c, 6, 3); }
    else if (cat == CAT_CLOTH) { S.box(xf(p + V3{0, 0.06f, 0}), {0.2f, 0.06f, 0.16f}, c); }
    else if (item == 26) { S.cyl(xf(p + V3{0, 0.5f, 0}), 0.01f, 0.5f, C(220, 220, 220), 3); S.sphere(xf(p + V3{0, 1.1f, 0}), {0.22f, 0.28f, 0.22f}, c, 7, 4); }
    else if (item == 27) S.sphere(xf(p + V3{0, 0.16f, 0}), {0.16f, 0.16f, 0.16f}, c, 7, 4);
    else S.box(xf(p + V3{0, 0.12f, 0}), {0.16f, 0.12f, 0.16f}, c);
}
static void drawRoom(Scene3D& S, int room, float tAnim) {
    Col wall, fA, fB; roomTheme(room, wall, fA, fB);
    const float W = 4.f, D = 3.f, H = 3.3f;
    bool outside = room == ROOM_PARK;
    S.hemiTop = outside ? C(215, 228, 255) : C(255, 246, 228); S.hemiBot = outside ? C(120, 122, 135) : C(176, 154, 134);
    S.depthBias = 30;
    if (!outside) {
        for (int i = 0; i < 8; i++) for (int j = 0; j < 6; j++) {
            V3 a = {-W + i, 0, -D + j}, b = {-W + i + 1, 0, -D + j}, c = {-W + i + 1, 0, -D + j + 1}, d = {-W + i, 0, -D + j + 1};
            S.quad(a, b, c, d, ((i + j) & 1) ? fA : fB, 0);
        }
        S.quad({-W, 0, -D}, {W, 0, -D}, {W, H, -D}, {-W, H, -D}, wall, 0);
        S.quad({-W, 0, D}, {-W, 0, -D}, {-W, H, -D}, {-W, H, D}, mulc(wall, 0.88f), 0);
        S.box(xf({0, 0.1f, -D + 0.03f}), {W, 0.1f, 0.03f}, mulc(wall, 0.6f), 0);
        S.depthBias = 28;
        for (int i = 0; i < 16; i += 2) S.quad({-W + i * 0.5f + 0.5f, 0.2f, -D + 0.006f}, {-W + i * 0.5f + 1.0f, 0.2f, -D + 0.006f}, {-W + i * 0.5f + 1.0f, H, -D + 0.006f}, {-W + i * 0.5f + 0.5f, H, -D + 0.006f}, mulc(wall, 0.93f), 0);
        S.depthBias = 0;
        // window showing the sky
        Col t, b; float l; skyColors(t, b, l);
        S.quad({-1.0f, 1.3f, -D + 0.02f}, {1.0f, 1.3f, -D + 0.02f}, {1.0f, 2.6f, -D + 0.02f}, {-1.0f, 2.6f, -D + 0.02f}, mixc(t, b, 0.5f), 0, true);
        S.box(xf({0, 1.95f, -D + 0.05f}), {1.08f, 0.04f, 0.04f}, C(235, 235, 235), 0, false);
        S.box(xf({0, 1.3f, -D + 0.05f}), {1.08f, 0.04f, 0.05f}, C(235, 235, 235), 0, false);
        S.box(xf({0, 1.95f, -D + 0.05f}), {0.03f, 0.68f, 0.04f}, C(235, 235, 235), 0, false);
    } else {
        for (int i = 0; i < 8; i++) for (int j = 0; j < 6; j++) {
            V3 a = {-W + i, 0, -D + j}, b = {-W + i + 1, 0, -D + j}, c = {-W + i + 1, 0, -D + j + 1}, d = {-W + i, 0, -D + j + 1};
            S.quad(a, b, c, d, ((i + j) & 1) ? fA : fB, 0);
        }
        S.depthBias = 0;
        for (int i = 0; i < 4; i++) S.box(xf({-3.0f + i * 2.0f, 0.01f, 0.2f}), {0.05f, 0.005f, 1.3f}, C(230, 230, 230), 0);
        for (int i = 0; i < 9; i++) S.box(xf({-4.f + i, 0.55f, -D}), {0.04f, 0.55f, 0.04f}, C(150, 150, 155), 0);
        S.box(xf({0, 0.9f, -D}), {W, 0.04f, 0.04f}, C(150, 150, 155), 0); S.box(xf({0, 0.5f, -D}), {W, 0.03f, 0.03f}, C(150, 150, 155), 0);
    }
    if (room < NCHAR) {
        Col th = fromHex(ROSTER[room].top);
        S.box(xf({-2.4f, 0.18f, -1.9f}), {1.05f, 0.18f, 0.75f}, C(120, 90, 60));
        S.box(xf({-2.4f, 0.42f, -1.9f}), {1.0f, 0.1f, 0.7f}, th);
        S.box(xf({-3.15f, 0.55f, -1.9f}), {0.3f, 0.07f, 0.4f}, C(245, 245, 245));
        S.box(xf({0.4f, 0.012f, 0.9f}), {1.3f, 0.012f, 0.95f}, mixc(th, C(255, 255, 255), 0.25f), 0);
        S.box(xf({2.5f, 0.78f, -2.4f}), {0.9f, 0.04f, 0.45f}, C(130, 96, 66));
        for (int k = 0; k < 2; k++) S.box(xf({2.5f + (k ? 0.8f : -0.8f), 0.38f, -2.4f}), {0.05f, 0.38f, 0.4f}, C(100, 74, 52));
        S.box(xf({2.5f, 1.1f, -2.7f}), {0.4f, 0.27f, 0.04f}, C(60, 150, 230), 1, true);
        S.box(xf({2.5f, 0.55f, -1.6f}), {0.28f, 0.04f, 0.28f}, C(60, 60, 70)); S.box(xf({2.5f, 0.3f, -1.6f}), {0.04f, 0.27f, 0.04f}, C(60, 60, 70));
        S.box(xf({-0.2f, 0.9f, -2.78f}), {0.8f, 0.9f, 0.18f}, C(110, 80, 56));
        for (int k = 0; k < 6; k++) S.box(xf({-0.8f + k * 0.28f, 1.55f, -2.62f}), {0.09f, 0.18f, 0.08f}, fromHex(ITEMS[(room + k * 5) % 11 + 11].color));
        S.quad({-W + 0.02f, 1.2f, -0.6f}, {-W + 0.02f, 1.2f, 0.6f}, {-W + 0.02f, 2.4f, 0.6f}, {-W + 0.02f, 2.4f, -0.6f}, mixc(th, C(20, 20, 30), 0.4f), 0);
        S.quad({-W + 0.03f, 1.5f, -0.3f}, {-W + 0.03f, 1.5f, 0.3f}, {-W + 0.03f, 2.1f, 0.3f}, {-W + 0.03f, 2.1f, -0.3f}, C(249, 115, 22), 0, true);
        if (room == I_DALIA) {                                           // the door next to Dalia's bed leads outside
            S.box(xf({3.3f, 1.0f, -D + 0.06f}), {0.55f, 1.05f, 0.05f}, C(249, 115, 22), 0, true);
            S.box(xf({3.3f, 1.0f, -D + 0.1f}), {0.45f, 0.95f, 0.05f}, C(12, 8, 6), 0);
        }
        drawBot(S, room, {-1.2f, 0, 1.9f}, tAnim);
        S.box(xf({-3.55f, 1.1f, 0.9f}), {0.42f, 1.1f, 0.72f}, mixc(th, C(240, 230, 215), 0.55f)); S.box(xf({-3.12f, 1.1f, 0.55f}), {0.012f, 1.0f, 0.3f}, mulc(th, 0.7f)); S.box(xf({-3.12f, 1.1f, 1.25f}), {0.012f, 1.0f, 0.3f}, mulc(th, 0.7f));
        S.box(xf({-3.1f, 1.15f, 0.7f}), {0.03f, 0.1f, 0.03f}, C(230, 200, 90)); S.box(xf({-3.1f, 1.15f, 1.1f}), {0.03f, 0.1f, 0.03f}, C(230, 200, 90));
        S.cyl(xf({-3.4f, 0.8f, -0.9f}), 0.025f, 0.8f, C(70, 70, 80), 6); bool lampOn = isNight() || G.minute < 7 * 60;
        S.cyl(xf({-3.4f, 1.72f, -0.9f}), 0.28f, 0.2f, lampOn ? C(255, 226, 150) : C(240, 225, 190), 10, 1, 0.18f); if (lampOn) S.sphere(xf({-3.4f, 1.65f, -0.9f}), {0.12f, 0.12f, 0.12f}, C(255, 240, 190), 6, 4, 1, true);
        S.quad({-1.8f, 1.5f, -D + 0.02f}, {-1.3f, 1.5f, -D + 0.02f}, {-1.3f, 2.2f, -D + 0.02f}, {-1.8f, 2.2f, -D + 0.02f}, mixc(th, C(255, 255, 255), 0.5f), 0, true);
        S.quad({1.4f, 1.9f, -D + 0.02f}, {2.2f, 1.9f, -D + 0.02f}, {2.2f, 2.5f, -D + 0.02f}, {1.4f, 2.5f, -D + 0.02f}, mixc(th, C(30, 30, 60), 0.35f), 0, true);
        for (int k = 0; k < 2; k++) S.box(xf({k ? 1.22f : -1.22f, 1.95f, -D + 0.16f}), {0.22f, 0.85f, 0.07f}, mulc(th, 0.85f), 0);
        S.cyl(xf({3.45f, 0.22f, 0.3f}), 0.2f, 0.22f, C(160, 100, 70), 10); for (int k = 0; k < 5; k++) S.sphere(xf({3.45f + std::sin(k * 1.3f) * 0.22f, 0.6f + 0.1f * k, 0.3f + std::cos(k * 1.3f) * 0.2f}), {0.2f, 0.28f, 0.2f}, C(60 + k * 12, 160, 90), 7, 4);
    } else if (room == ROOM_LOBBY) {
        S.box(xf({2.1f, 0.5f, -2.2f}), {1.4f, 0.5f, 0.4f}, C(249, 115, 22)); S.box(xf({2.1f, 1.02f, -2.2f}), {1.45f, 0.04f, 0.45f}, C(30, 26, 20));
        S.box(xf({-2.6f, 0.3f, -2.2f}), {1.15f, 0.3f, 0.5f}, C(70, 90, 140)); S.box(xf({-2.6f, 0.85f, -2.6f}), {1.15f, 0.35f, 0.12f}, C(60, 80, 130));
        S.box(xf({0, 1.2f, -D + 0.05f}), {0.8f, 1.2f, 0.05f}, C(170, 175, 185)); S.box(xf({0, 1.2f, -D + 0.08f}), {0.03f, 1.15f, 0.03f}, C(60, 60, 66));
        S.box(xf({0, 2.75f, -D + 0.06f}), {0.35f, 0.18f, 0.03f}, C(249, 115, 22), 0, true);
        S.cyl(xf({3.5f, 0.25f, 0.4f}), 0.25f, 0.25f, C(150, 90, 60), 8); S.sphere(xf({3.5f, 0.8f, 0.4f}), {0.4f, 0.45f, 0.4f}, C(50, 150, 80), 7, 4);
    } else if (room == ROOM_ICE) {
        S.box(xf({0, 0.55f, -1.9f}), {2.6f, 0.55f, 0.45f}, C(250, 140, 190)); S.box(xf({0, 1.12f, -1.9f}), {2.7f, 0.04f, 0.5f}, C(255, 255, 255));
        for (int k = 0; k < 5; k++) { S.cyl(xf({-1.6f + k * 0.8f, 1.35f, -1.9f}), 0.14f, 0.17f, C(230, 180, 110), 6, 1, 0.01f); S.sphere(xf({-1.6f + k * 0.8f, 1.62f, -1.9f}), {0.17f, 0.17f, 0.17f}, fromHex(ITEMS[6].color + k * 0x101000), 6, 3); }
        S.quad({-1.4f, 1.8f, -D + 0.02f}, {1.4f, 1.8f, -D + 0.02f}, {1.4f, 2.8f, -D + 0.02f}, {-1.4f, 2.8f, -D + 0.02f}, C(40, 28, 38), 0);
    } else {
        auto car = [&](float x, float z, Col c) { S.box(xf({x, 0.35f, z}), {0.85f, 0.25f, 0.45f}, c); S.box(xf({x - 0.1f, 0.7f, z}), {0.45f, 0.18f, 0.4f}, mulc(c, 0.85f)); S.box(xf({x - 0.1f, 0.7f, z + 0.01f}), {0.4f, 0.12f, 0.41f}, C(150, 200, 235), 1, true); };
        car(-2.6f, -2.3f, C(200, 60, 50)); car(0.2f, -2.3f, C(60, 90, 190)); car(2.8f, -2.3f, C(220, 220, 225));
    }
    for (auto& p : G.props) if (p.room == room) drawPropMesh(S, p.item, {p.x, 0, p.z});
}

// ---------------------------------------------------------------- the island panorama
static V3 spotFor(int room) {
    if (room == ROOM_LOBBY) return {-1.6f, 0, 0.9f}; if (room == ROOM_ICE) return {3.0f, 0, 1.4f}; if (room == ROOM_PARK) return {-3.8f, 0, 2.0f}; return {-1.5f, 0, -0.1f};
}
static void drawIsland(Scene3D& S, float tAnim, float yaw, float dist = 19.f, float ht = 10.5f, float tgtY = 1.2f) {
    Col t, b; float l; skyColors(t, b, l); S.light = l; S.hemiTop = mixc(t, C(255, 255, 255), 0.55f); S.hemiBot = C(100, 128, 88);
    S.cam.pos = {std::sin(yaw) * dist, ht, std::cos(yaw) * dist}; S.cam.target = {0, tgtY, 0}; S.cam.fov = 0.82f; S.cam.update();
    // water
    S.depthBias = 0;
    int N = 28; Col wc = mixc(C(30, 90, 170), t, 0.25f);
    for (int i = 0; i < N; i++) { float a0 = i * 6.2831853f / N, a1 = (i + 1) * 6.2831853f / N; float R_ = 40;
        S.quad({0, -0.55f, 0}, {std::cos(a0) * R_, -0.55f, std::sin(a0) * R_}, {std::cos(a1) * R_, -0.55f, std::sin(a1) * R_}, {0, -0.55f, 0}, wc, -1); }
    for (int k = 0; k < 3; k++) { float rad = 8.5f + k * 1.6f + std::sin(tAnim + k) * 0.3f; for (int i = 0; i < 24; i += 2) { float a = i * 0.2618f + k; S.box(xf({std::cos(a) * rad, -0.5f, std::sin(a) * rad}, rotY(-a)), {0.5f, 0.01f, 0.05f}, mixc(wc, C(255, 255, 255), 0.35f), -1); } }
    S.depthBias = 80; S.cyl(xf({0, -0.4f, 0}), 8.2f, 0.12f, C(214, 190, 130), 36, 0);
    S.depthBias = 40; S.cyl(xf({0, -0.25f, 0}), 7.4f, 0.25f, C(76, 160, 80), 36, 0);
    S.depthBias = 0;
    // road
    S.box(xf({0, 0.02f, 1.7f}), {7.0f, 0.02f, 0.38f}, C(70, 72, 80), 0);
    for (int i = 0; i < 12; i++) S.box(xf({-6.2f + i * 1.1f, 0.045f, 1.7f}), {0.25f, 0.005f, 0.03f}, C(240, 220, 120), 0);
    // 14 Tower
    S.box(xf({-1.5f, 2.2f, -1.6f}), {0.85f, 2.2f, 0.85f}, C(60, 64, 82));
    S.box(xf({-1.5f, 4.35f, -1.6f}), {0.9f, 0.12f, 0.9f}, ORANGE, 1, true);
    S.box(xf({-1.5f, 4.7f, -1.6f}), {0.05f, 0.3f, 0.05f}, C(200, 200, 200));
    for (int fl = 0; fl < 6; fl++) for (int k = 0; k < 2; k++) for (int side = 0; side < 4; side++) {
        float ox = side == 0 ? 0.86f : side == 1 ? -0.86f : (k ? 0.35f : -0.35f), oz = side == 2 ? 0.86f : side == 3 ? -0.86f : (k ? 0.35f : -0.35f);
        bool lit = isNight() ? ((fl * 7 + k * 3 + side) % 3 != 0) : false;
        S.box(xf({-1.5f + ox, 0.9f + fl * 0.62f, -1.6f + oz}), {side < 2 ? 0.02f : 0.18f, 0.16f, side < 2 ? 0.18f : 0.02f}, lit ? C(255, 220, 120) : C(110, 150, 190), 1, lit);
    }
    S.box(xf({-1.5f, 0.4f, -0.45f}), {1.2f, 0.4f, 0.55f}, C(120, 124, 140));
    S.box(xf({-1.5f, 0.82f, -0.45f}), {1.25f, 0.04f, 0.6f}, ORANGE);
    // ice cream shop
    S.box(xf({3.1f, 0.45f, 0.45f}), {0.65f, 0.45f, 0.6f}, C(250, 200, 220)); S.cyl(xf({3.1f, 1.2f, 0.45f}), 0.95f, 0.3f, C(240, 110, 160), 8, 1, 0.01f);
    S.cyl(xf({3.1f, 1.95f, 0.45f}), 0.12f, 0.25f, C(230, 180, 110), 6, 1, 0.01f); S.sphere(xf({3.1f, 2.3f, 0.45f}), {0.22f, 0.22f, 0.22f}, C(249, 168, 212), 6, 3);
    // shop
    S.box(xf({3.3f, 0.55f, -2.5f}), {0.95f, 0.55f, 0.75f}, C(200, 210, 230)); S.box(xf({3.3f, 1.2f, -1.75f}), {1.0f, 0.06f, 0.35f}, ORANGE);
    // parking
    S.box(xf({-4.3f, 0.03f, 3.0f}), {1.7f, 0.03f, 1.0f}, C(90, 90, 98), 0);
    for (int k = 0; k < 3; k++) { S.box(xf({-5.2f + k * 1.0f, 0.2f, 3.1f}), {0.35f, 0.15f, 0.2f}, fromHex(0xC83C32 + k * 0x203040)); S.box(xf({-5.2f + k * 1.0f, 0.4f, 3.1f}), {0.18f, 0.1f, 0.18f}, C(150, 200, 235)); }
    // park trees
    for (int k = 0; k < 6; k++) { float tx = -0.4f + std::fmod(k * 1.37f, 3.4f), tz = 3.0f + std::fmod(k * 0.83f, 1.6f); S.cyl(xf({tx, 0.3f, tz}), 0.08f, 0.3f, C(110, 74, 40), 5); S.cyl(xf({tx, 0.95f, tz}), 0.42f, 0.4f, C(46, 140, 70), 7, 1, 0.01f); }
    // extra island life: hills, flowers, dock + boat, lamps, birds, fireflies, foam
    S.sphere(xf({1.5f, -0.3f, -4.2f}), {3.2f, 1.1f, 2.2f}, C(70, 150, 78), 12, 6); S.sphere(xf({-4.4f, -0.3f, -2.2f}), {2.4f, 0.9f, 2.0f}, C(82, 164, 84), 12, 6);
    for (int k = 0; k < 30; k++) { float fx = std::sin(k * 2.399f) * 5.2f, fz = std::cos(k * 2.399f) * 4.6f + 0.6f; if (std::fabs(fz - 1.7f) < 0.7f) continue; S.sphere(xf({fx, 0.07f, fz}), {0.06f, 0.06f, 0.06f}, fromHex(0xF472B6 + (k % 4) * 0x40C000 - (k % 3) * 0x300000), 5, 3); }
    for (int k = 0; k < 7; k++) S.box(xf({5.4f + k * 0.38f, -0.15f, 4.2f - k * 0.2f}, rotY(-0.45f)), {0.17f, 0.03f, 0.26f}, C(150 + (k % 2) * 14, 112, 72));
    S.box(xf({7.6f, -0.35f, 2.9f}, rotY(0.5f)), {0.45f, 0.14f, 0.18f}, C(240, 240, 240)); S.box(xf({7.6f, 0.15f, 2.9f}, rotY(0.5f)), {0.02f, 0.4f, 0.02f}, C(120, 90, 60)); S.box(xf({7.6f, 0.25f, 2.95f}, rotY(0.5f)), {0.2f, 0.25f, 0.01f}, C(236, 72, 153));
    for (int k = 0; k < 5; k++) { float lx = -5.5f + k * 2.7f; S.cyl(xf({lx, 0.4f, 1.15f}), 0.03f, 0.4f, C(60, 62, 70), 6); S.sphere(xf({lx, 0.85f, 1.15f}), {0.11f, 0.11f, 0.11f}, isNight() ? C(255, 232, 160) : C(230, 230, 220), 6, 4, 1, isNight()); }
    for (int k = 0; k < 4; k++) { float a = tAnim * 0.35f + k * 1.6f; float fl = std::sin(tAnim * 9 + k) * 0.25f; V3 bp = {std::cos(a) * 6.f, 5.5f + k * 0.4f, std::sin(a) * 6.f}; Xf bx = xf(bp, rotY(-a)); S.box(compose(bx, xf({0.2f, fl * 0.5f, 0}, rotZ(0.3f + fl))), {0.22f, 0.015f, 0.07f}, C(245, 245, 250)); S.box(compose(bx, xf({-0.2f, fl * 0.5f, 0}, rotZ(-0.3f - fl))), {0.22f, 0.015f, 0.07f}, C(245, 245, 250)); }
    if (isNight()) for (int k = 0; k < 14; k++) S.box(xf({std::sin(tAnim * 0.4f + k * 1.7f) * 3.5f + 0.5f, 0.5f + 0.4f * std::sin(tAnim * 1.1f + k), 3.2f + std::cos(tAnim * 0.3f + k * 2.1f) * 1.4f}), {0.025f, 0.025f, 0.025f}, C(230, 255, 140), 1, true);
    S.depthBias = 120; for (int i = 0; i < 40; i++) { float a0 = i * 0.157f, a1 = (i + 1) * 0.157f; float r0 = 8.9f + std::sin(tAnim * 1.3f + i) * 0.12f, r1 = 8.0f; S.quad({std::cos(a0) * r1, -0.38f, std::sin(a0) * r1}, {std::cos(a0) * r0, -0.38f, std::sin(a0) * r0}, {std::cos(a1) * r0, -0.38f, std::sin(a1) * r0}, {std::cos(a1) * r1, -0.38f, std::sin(a1) * r1}, C(255, 255, 255, 120), 0, true); } S.depthBias = 0;
    // citizens in shared spots
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i]; if (c.room < NCHAR) continue; V3 sp = spotFor(c.room);
        V3 p = sp + V3{std::sin(tAnim * 0.6f + i * 1.7f) * 0.6f, 0.0f, std::cos(tAnim * 0.5f + i * 2.3f) * 0.4f};
        CharPose P; S.shadow(p.x, p.z, 0.3f, 0.03f); drawCharacter(S, i, p + V3{0, 0.02f, 0}, tAnim * 0.4f + i, P, true);
    }
    // clouds
    for (int k = 0; k < 4; k++) { float a = tAnim * 0.02f + k * 1.6f; S.sphere(xf({std::cos(a) * 16.f, 8.5f + k * 0.6f, std::sin(a) * 16.f}), {1.8f, 0.5f, 1.2f}, C(255, 255, 255, 255), 6, 3, -1); }
}
