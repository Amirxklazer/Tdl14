// ui.hpp - input, widgets, icons, speech/status bubbles
#pragma once
#include "scene3d.hpp"

enum { BTN_A = 1, BTN_B = 2, BTN_X = 4, BTN_Y = 8, BTN_L = 16, BTN_R = 32, BTN_ZL = 64, BTN_ZR = 128, BTN_PLUS = 256, BTN_MINUS = 512, BTN_LEFT = 1024, BTN_UP = 2048, BTN_RIGHT = 4096, BTN_DOWN = 8192 };
struct PadIn {
    float px = 0, py = 0, sx = 0, sy = 0; bool down = false, pressed = false, released = false;
    float lx = 0, ly = 0; Uint32 held = 0, hit = 0; bool padMode = false; float lastTouch = -10;
    float cx = 640, cy = 360;                               // virtual cursor for gamepad play
    float pressT = 0;
};
static PadIn in;
static Col UIA = {34, 211, 238, 255};      // accent colour of the current screen (changes per screen)
static const Col BGA = {16, 14, 40, 255}, BGB = {30, 40, 84, 255};
static bool inside(float x, float y, float w, float h, float px, float py) { return px >= x && px < x + w && py >= y && py < y + h; }
static bool hover(float x, float y, float w, float h) { return inside(x, y, w, h, in.px, in.py); }
static bool pressedIn(float x, float y, float w, float h) { return in.down && inside(x, y, w, h, in.sx, in.sy) && inside(x, y, w, h, in.px, in.py); }
static bool clickedIn(float x, float y, float w, float h) { return in.released && inside(x, y, w, h, in.sx, in.sy) && inside(x, y, w, h, in.px, in.py); }
static bool tappedIn(float x, float y, float w, float h) { return in.pressed && inside(x, y, w, h, in.px, in.py); }

static bool btn(float x, float y, float w, float h, const char* label, float ts = 22, Col fg = UIA, bool on = true, bool filled = false) {
    bool pr = on && pressedIn(x, y, w, h);
    Col bg = filled ? (pr ? mulc(UIA, 0.75f) : UIA) : (pr ? C(64, 60, 140) : C(30, 28, 70));
    fillRect(x, y, w, h, bg); strokeRect(x, y, w, h, on ? fg : C(90, 80, 70), 2);
    text(label, x + w / 2, y + (h - ts) / 2, ts, filled ? BLACK0 : (on ? fg : C(120, 130, 180)), 1);
    bool c = on && clickedIn(x, y, w, h); if (c) sfx(SFX_TAP); return c;
}
static std::string fmtMoney(int m) { std::string s = std::to_string(m), o; int n = (int)s.size(); for (int i = 0; i < n; i++) { o += s[i]; if ((n - i - 1) % 3 == 0 && i < n - 1) o += ','; } return o + " DA"; }
static void bar(float x, float y, float w, float h, float v, Col c, const char* label = nullptr) {
    fillRect(x, y, w, h, C(30, 26, 18)); fillRect(x, y, w * std::max(0.f, std::min(1.f, v / 100.f)), h, c); strokeRect(x, y, w, h, C(80, 82, 140), 1);
    if (label) text(label, x + 4, y + (h - 12) / 2, 12, WHITE);
}
static void panelBG() {
    vgrad(PANEL_X, 0, SCR_W - PANEL_X, SCR_H, BGA, BGB);
    for (int y = 0; y < SCR_H; y += 40) fillRect(PANEL_X, y, SCR_W - PANEL_X, 1, C(36, 34, 78));
    for (int x = PANEL_X; x < SCR_W; x += 40) fillRect(x, 0, 1, SCR_H, C(36, 34, 78));
    fillRect(PANEL_X - 3, 0, 3, SCR_H, UIA); fillRect(PANEL_X - 3, 0, 3, 240, C(244, 114, 182)); fillRect(PANEL_X - 3, 240, 3, 240, C(250, 204, 21));
}
static void header(const char* title = nullptr) {
    fillRect(PANEL_X, 0, SCR_W - PANEL_X, 58, C(24, 22, 58)); fillRect(PANEL_X, 58, SCR_W - PANEL_X, 3, UIA);
    if (G.realClock) { text(dateStr().c_str(), PANEL_X + 14, 7, 17, UIA); text(clockStr().c_str(), PANEL_X + 14, 26, 28, WHITE); }
    else { textf(PANEL_X + 14, 8, 20, UIA, 0, "DAY %d (fast clock)", G.day); text(clockStr().c_str(), PANEL_X + 14, 30, 24, WHITE); }
    text(fmtMoney(G.money).c_str(), SCR_W - 14, 20, 24, C(250, 204, 21), 2);
    if (title) text(title, PANEL_X + PW_ / 2 + 0, 18, 22, WHITE, 1);
}

// ---------------------------------------------------------------- pictograms (original, drawn from primitives)
static void drawIcon(int id, float cx, float cy, float s, Col c) {
    float h = s / 2;
    switch (id) {
        case 0: fillRect(cx - h * 0.45f, cy - h, h * 0.9f, h * 2, c); for (int r = 0; r < 4; r++) for (int k = 0; k < 2; k++) fillRect(cx - h * 0.3f + k * h * 0.35f, cy - h * 0.8f + r * h * 0.4f, h * 0.2f, h * 0.2f, BLACK0); break;   // tower
        case 1: fillRect(cx - h, cy - h * 0.3f, h * 2, h * 1.3f, c); fillRect(cx - h * 0.2f, cy + h * 0.2f, h * 0.4f, h * 0.8f, BLACK0); tri2(cx - h * 1.1f, cy - h * 0.3f, cx + h * 1.1f, cy - h * 0.3f, cx, cy - h, c); break;   // lobby
        case 2: tri2(cx - h * 0.45f, cy - h * 0.1f, cx + h * 0.45f, cy - h * 0.1f, cx, cy + h, c); disc(cx, cy - h * 0.35f, h * 0.55f, h * 0.55f, c); disc(cx, cy - h * 0.7f, h * 0.3f, h * 0.3f, mulc(c, 0.7f)); break;   // ice cream
        case 3: fillRect(cx - h, cy - h * 0.1f, h * 2, h * 0.8f, c); fillRect(cx - h * 0.6f, cy - h * 0.7f, h * 1.2f, h * 0.6f, c); disc(cx - h * 0.55f, cy + h * 0.75f, h * 0.25f, h * 0.25f, BLACK0); disc(cx + h * 0.55f, cy + h * 0.75f, h * 0.25f, h * 0.25f, BLACK0); break;   // parking (car)
        case 4: fillRect(cx - h * 0.8f, cy - h * 0.4f, h * 1.6f, h * 1.3f, c); strokeRect(cx - h * 0.4f, cy - h * 0.9f, h * 0.8f, h * 0.7f, c, 3); break;    // shop bag
        case 5: disc(cx - h * 0.4f, cy - h * 0.3f, h * 0.35f, h * 0.35f, c); disc(cx + h * 0.4f, cy - h * 0.3f, h * 0.35f, h * 0.35f, c); fillRect(cx - h * 0.9f, cy + h * 0.1f, h * 0.9f, h * 0.7f, c); fillRect(cx, cy + h * 0.1f, h * 0.9f, h * 0.7f, c); break;   // people
        case 6: fillRect(cx - h * 0.45f, cy - h, h * 0.9f, h * 2, c); fillRect(cx - h * 0.35f, cy - h * 0.8f, h * 0.7f, h * 1.3f, BLACK0); disc(cx, cy + h * 0.75f, h * 0.12f, h * 0.12f, BLACK0); break;   // phone
        case 7: fillRect(cx - h, cy - h * 0.55f, h * 2, h * 1.4f, c); disc(cx, cy + h * 0.1f, h * 0.45f, h * 0.45f, BLACK0); disc(cx, cy + h * 0.1f, h * 0.25f, h * 0.25f, c); fillRect(cx - h * 0.4f, cy - h * 0.8f, h * 0.5f, h * 0.3f, c); break;   // camera
        case 8: disc(cx, cy, h * 0.7f, h * 0.7f, c, 12); for (int i = 0; i < 8; i++) { float a = i * 0.7854f; fillRect(cx + std::cos(a) * h * 0.85f - h * 0.15f, cy + std::sin(a) * h * 0.85f - h * 0.15f, h * 0.3f, h * 0.3f, c); } disc(cx, cy, h * 0.3f, h * 0.3f, BLACK0); break;   // gear
        case 9: fillRect(cx - h, cy - h, h * 2, h * 2, c); fillRect(cx - h * 0.6f, cy - h, h * 1.2f, h * 0.7f, BLACK0); fillRect(cx - h * 0.5f, cy + h * 0.1f, h, h * 0.9f, BLACK0); break;   // save
        case 10: fillRect(cx - h, cy - h * 0.7f, h * 2, h * 1.6f, c); lineD(cx - h * 0.7f, cy - h * 0.2f, cx + h * 0.7f, cy - h * 0.2f, BLACK0, 3); lineD(cx - h * 0.7f, cy + h * 0.3f, cx + h * 0.3f, cy + h * 0.3f, BLACK0, 3); break;  // notes
    }
}
static void itemIcon(int item, float x, float y, float s, bool dim = false) {
    const ItemDef& it = ITEMS[item]; Col c = fromHex(it.color); if (dim) c = mulc(c, 0.4f);
    fillRect(x, y, s, s, C(30, 28, 70)); strokeRect(x, y, s, s, UIA, 2);
    float m = s * 0.18f;
    if (it.cat == CAT_FOOD) { disc(x + s / 2, y + s * 0.55f, s * 0.32f, s * 0.22f, C(235, 235, 235)); disc(x + s / 2, y + s * 0.45f, s * 0.24f, s * 0.2f, c); }
    else if (it.cat == CAT_CLOTH) { fillRect(x + m, y + m * 1.4f, s - 2 * m, s - 3 * m, c); if (it.slot == SL_GLASSES) { disc(x + s * 0.33f, y + s * 0.5f, s * 0.14f, s * 0.14f, C(10, 10, 14)); disc(x + s * 0.67f, y + s * 0.5f, s * 0.14f, s * 0.14f, C(10, 10, 14)); } }
    else { fillRect(x + m, y + m, s - 2 * m, s - 2 * m, c); }
    text(it.tag, x + s / 2, y + s * 0.34f, s * 0.26f, it.cat == CAT_FOOD ? BLACK0 : C(255, 255, 255, 230), 1);
}

// ---------------------------------------------------------------- bubbles over heads
static void drawStatusBubble(float sx, float sy, int type, int arg, float t) {
    float bob = std::sin(t * 5.f) * 3.f; sy += bob; Col col; const char* glyph = "";
    switch (type) {
        case B_WANT: col = C(250, 204, 21); break; case B_SAD: col = C(59, 130, 246); break; case B_FRIEND: col = C(249, 115, 22); break;
        case B_STUCK: col = C(168, 85, 247); break; case B_GAME: col = C(34, 197, 94); break; case B_DREAM: col = C(226, 232, 240); break; default: col = C(244, 114, 182); break;
    }
    if (type == B_SAD) for (int i = -1; i <= 1; i++) lineD(sx + i * 12, sy - 66, sx + i * 12, sy - 40 + std::fmod(t * 40.f + i * 9, 10.f), col, 3);
    if (type == B_STUCK) for (int i = -1; i <= 1; i++) tri2(sx + i * 17 - 8, sy - 62, sx + i * 17 + 8, sy - 62, sx + i * 17, sy - 84 - (i == 0 ? 8 : 0), col);
    if (type == B_GAME) for (int i = -1; i <= 1; i++) fillRect(sx + i * 12 - 3, sy - 84 + (i == 0 ? 0 : 6), 6, 20, col);
    disc(sx, sy - 34, 27, 27, BLACK0, 22); disc(sx, sy - 34, 23, 23, col, 22);
    tri2(sx - 8, sy - 12, sx + 8, sy - 12, sx, sy - 2, BLACK0);
    switch (type) {
        case B_WANT: text(arg >= 0 ? ITEMS[arg].tag : "!", sx, sy - 46, 22, BLACK0, 1); break;
        case B_SAD: text("...", sx, sy - 52, 26, BLACK0, 1); break;
        case B_FRIEND: text("+", sx, sy - 48, 30, BLACK0, 1); break;
        case B_STUCK: text("!", sx, sy - 48, 30, BLACK0, 1); break;
        case B_GAME: tri2(sx - 8, sy - 46, sx - 8, sy - 22, sx + 12, sy - 34, BLACK0); break;
        case B_DREAM: text("Zz", sx, sy - 47, 24, C(40, 60, 120), 1); break;
        default: disc(sx - 6, sy - 40, 7, 7, BLACK0, 10); disc(sx + 6, sy - 40, 7, 7, BLACK0, 10); tri2(sx - 12, sy - 37, sx + 12, sy - 37, sx, sy - 22, BLACK0); break;
    }
    (void)glyph;
}
static void drawSpeech(float sx, float sy, const std::string& s, bool thought, const std::string& who, float vx0, float vx1, Col accent = {34, 211, 238, 255}) {
    float tw = 260, th = 17; int lines = 0;
    { std::string w2, cur; int maxc = (int)(tw / advFor(th)); const char* p = s.c_str(); lines = 1; int cl = 0; std::string word; while (true) { if (*p == ' ' || *p == 0) { int wl = (int)word.size(); if (cl && cl + 1 + wl > maxc) { lines++; cl = 0; } cl += (cl ? 1 : 0) + wl; word.clear(); if (!*p) break; } else word += *p; p++; } }
    float bh = lines * (th + 3) + 30, bw = tw + 20; float bx = std::max(vx0 + 6, std::min(vx1 - bw - 6, sx - bw / 2)); float by = std::max(6.f, sy - bh - 6);
    fillRect(bx, by, bw, bh, C(16, 14, 40, 235)); strokeRect(bx, by, bw, bh, thought ? C(148, 163, 184) : accent, 2);
    if (thought) { disc(sx, sy + 2, 6, 6, C(16, 14, 40)); disc(sx + 4, sy + 14, 4, 4, C(16, 14, 40)); } else tri2(sx - 8, by + bh, sx + 8, by + bh, sx, by + bh + 12, accent);
    text(who.c_str(), bx + 8, by + 4, 13, thought ? C(148, 163, 184) : accent);
    textWrap(s.c_str(), bx + 10, by + 22, tw, th, thought ? C(200, 210, 225) : WHITE);
}
