// Îlot14 Island - a cursed island life sim for Nintendo Switch homebrew (C++ / SDL2 / libnx)
#include "minigames.hpp"
#ifdef __SWITCH__
#include <switch.h>
#endif

static const char* VERSION_STR = "v0.4.0";
enum UiMode { M_TITLE, M_MAP, M_ROOM, M_SHOP, M_PEOPLE, M_PROFILE, M_PHONE, M_SETTINGS, M_GAME, M_SETUP };
static UiMode mode = M_TITLE;
static void startSetup();
static float tAnim = 0, camYaw = 0.6f;
static int tab = 0, bagCat = 0, shopCat = 0, peoplePage = 0, profileIdx = 0, phoneTab = 0;
static std::string toast; static float toastT = 0;
static bool running = true, photoReq = false; static float photoFlash = 0; static float autosaveT = 0;
static bool saveExists = false;
static float talkCd = 0;

struct Dlg { bool open = false; std::string who, body, b1, b2; int kind = 0, a = 0, b = 0; };
static Dlg dlg;
static void say(const std::string& s) { toast = s; toastT = 2.6f; }
static void openDlg(int kind, int a, int b, const std::string& who, const std::string& body, const std::string& b1, const std::string& b2 = "") { dlg = {true, who, body, b1, b2, kind, a, b}; sfx(SFX_BELL); }

// ---------------------------------------------------------------- helpers
static V3 charPos(int i) {
    Char& c = G.ch[i];
    if (c.state == S_SLEEP) return {-1.7f, 0.62f, -1.9f};
    return {c.x, 0, c.z};
}
static bool charScreen(int i, float& sx, float& sy, float& r, float& depth, float yOff = 0.9f) {
    V3 p = charPos(i); float s = ageScale(ROSTER[i].age);
    if (G.ch[i].state == S_HELD) p.y += 0.6f;
    if (G.ch[i].state == S_SLEEP) { p = p + V3{-0.9f * s, 0.3f, 0}; }
    float x, y, d; if (!S3.cam.project(p + V3{0, yOff * s, 0}, x, y, d)) return false;
    sx = x; sy = y; depth = d; r = S3.cam.f / d * 0.55f * s + 10; return true;
}
static int pickChar(float px, float py, int ignore = -1) {
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < NCHAR; i++) {
        if (i == ignore || G.ch[i].room != G.viewRoom) continue;
        float sx, sy, r, d; if (!charScreen(i, sx, sy, r, d)) continue;
        if ((px - sx) * (px - sx) + (py - sy) * (py - sy) < r * r && d < bd) { bd = d; best = i; }
    }
    return best;
}
static void headScreen(int i, float& sx, float& sy) {
    V3 p = charPos(i); if (G.ch[i].state == S_HELD) p.y += 0.6f; if (G.ch[i].state == S_SLEEP) p = p + V3{-1.5f, 0.2f, 0};
    float d; S3.cam.project(p + V3{0, charHeight(i) + 0.05f, 0}, sx, sy, d);
}
static void setView(int room) { G.viewRoom = room; tab = tab; }
static void goRoom(int room) { G.viewRoom = room; mode = M_ROOM; curTrack = isNight() ? TR_NIGHT : TR_DAY; }
static std::string charLine(int i, const char* const* pool, int n) { (void)n; return voiced(i, pool[ri(n)]); }
#define LINE(i, arr) voiced(i, ROSTER[i].pers == P_TINY ? std::string(pick(L_TINY_GREET)) : std::string(pick(arr)))

// ---------------------------------------------------------------- interactions
static void tapChar(int i) {
    Char& c = G.ch[i]; G.selected = i; sfx(SFX_TAP);
    if (c.state == S_SLEEP) { speak(i, "...zzz...", 2.f); return; }
    addBond(i, 0.4f); addMood(i, 0.5f);
    if (rf() < 0.25f && ROSTER[i].pers != P_TINY) speak(i, voiced(i, std::string("Hi, ") + G.playerName + "!"), 3.0f); else speak(i, LINE(i, L_GREET), 3.0f);
    c.bounce = 0.6f;
}
static void tapBubble(int i) {
    Char& c = G.ch[i]; sfx(SFX_POP);
    switch (c.bubble) {
        case B_WANT: speak(i, voiced(i, std::string("I really want ") + ITEMS[c.bArg].name + "!"), 3.6f); break;
        case B_SAD: speak(i, voiced(i, pick(L_SAD)), 3.4f); say("Try giving them food or a gift."); break;
        case B_FRIEND: openDlg(1, i, c.bArg, nm(i), std::string("I'd like to be friends with ") + nm(c.bArg) + ". What do you think?", "Go for it!", "Not yet"); break;
        case B_STUCK: speak(i, voiced(i, fmtName("{n} and I are stuck...", nm(c.bArg))), 3.6f); say("Drag another friend onto them to help."); break;
        case B_GAME: { int g = ri(MG_COUNT); openDlg(2, i, g, nm(i), std::string("Wanna play ") + MG_NAME[g] + " with me?", "Let's play!", "Later"); break; }
        case B_DREAM: { int d = c.dream < 0 ? 0 : c.dream; speak(i, std::string("dreaming of ") + DREAMS[d].text + "...", 4.5f, true);
            if (DREAMS[d].item >= 0) { G.bag[DREAMS[d].item]++; say(std::string("The dream left you: ") + ITEMS[DREAMS[d].item].name); notice(std::string(nm(i)) + "'s dream manifested a " + ITEMS[DREAMS[d].item].name + "."); sfx(SFX_GLITCH); }
            c.bubble = B_NONE; c.dream = -1; break; }
        case B_CRUSH: openDlg(3, i, c.bArg, "Gabriel", "Dalia... I... uh. Do you want to... hang out? Like, as people? In a room?", "...", ""); break;
        default: break;
    }
}
static void onDialog(int choice) {
    Dlg d = dlg; dlg.open = false;
    if (d.kind == 1) {                                                 // friendship proposal
        int a = d.a, b = d.b;
        if (choice == 1) {
            if (isBeef(a, b)) { speak(a, voiced(a, "...no way. I'm not doing that."), 3.6f); addMood(a, -4); G.ch[a].bubble = B_NONE; G.ch[a].bCool = 90; sfx(SFX_BAD); }
            else { addAff(a, b, 20); G.ch[a].friends[b] = G.ch[b].friends[a] = 1; speak(a, voiced(a, "Yay, we're friends now!"), 3.6f); addMood(a, 10); addMood(b, 8); G.ch[a].bubble = B_NONE; G.ch[a].bCool = 60; sfx(SFX_GOOD); notice(std::string(nm(a)) + " and " + nm(b) + " became friends."); }
        } else { addAff(a, b, -4); G.ch[a].bubble = B_NONE; G.ch[a].bCool = 80; speak(a, "Okay. Maybe later.", 3.f); }
    } else if (d.kind == 2) {                                          // minigame request
        if (choice == 1) mgStart(d.b, d.a); else { G.ch[d.a].bubble = B_NONE; G.ch[d.a].bCool = 50; speak(d.a, "Aw, okay...", 2.8f); addMood(d.a, -3); }
    } else if (d.kind == 3) {                                          // Gabriel's crush scene
        speak(I_DALIA, "No.", 2.6f); addMood(I_GABRIEL, -6); G.ch[I_GABRIEL].bubble = B_NONE; G.ch[I_GABRIEL].bCool = 120; addAff(I_GABRIEL, I_DALIA, -0.5f); sfx(SFX_BAD);
        speak(I_GABRIEL, "...That went well. I think.", 4.f);
    } else if (d.kind == 4) { if (choice == 1) startSetup(); }
    else if (d.kind == 5) { if (choice == 1) running = false; }
}
static void dropOn(int held, int target) {                                // character dragged onto another character
    Char& T = G.ch[target];
    if (T.state == S_SLEEP) { speak(held, "Shh, they're sleeping.", 2.4f); return; }
    if (T.bubble == B_STUCK && T.bArg != held) {
        float p = 0.55f + (ROSTER[held].pers == P_CHEERFUL || ROSTER[held].pers == P_CALM || ROSTER[held].pers == P_CHILL ? 0.2f : 0.f) - (isHateCrush(target, T.bArg) ? 0.45f : 0.f);
        if (rf() < p) { addAff(target, T.bArg, 14); T.bubble = B_NONE; T.bCool = 60; speak(held, voiced(held, "Come on, you two. Talk it out."), 3.2f); speak(target, "...fine. Sorry.", 3.4f); sfx(SFX_GOOD); addMood(target, 8); notice(std::string(nm(held)) + " helped " + nm(target) + " make up."); }
        else { speak(held, "Hmm, that didn't work.", 2.8f); addMood(target, -3); sfx(SFX_BAD); }
        return;
    }
    addAff(held, target, 2.5f); addMood(held, 2); addMood(target, 2);
    speak(held, voiced(held, fmtName("Hi, {n}!", nm(target))), 2.6f); speak(target, voiced(target, "Oh, hi!"), 2.8f); sfx(SFX_POP);
}

// ---------------------------------------------------------------- drag state
struct Drag { int kind = 0, id = -1; bool grabbed = false; float t = 0; float sx = 0, sy = 0; };
static Drag drag;

static void roomCell(int r, float& x, float& y) { x = PANEL_X + 20 + (r % 5) * 80; y = 150 + (r / 5) * 70; }
static const char* roomShort(int r, char* buf) {
    if (r == ROOM_LOBBY) return "LOB"; if (r == ROOM_ICE) return "ICE"; if (r == ROOM_PARK) return "P1";
    snprintf(buf, 8, "%.3s", ROSTER[r].name); return buf;
}
static const char* roomName(int r) { return r < NCHAR ? (std::string(ROSTER[r].name) + "'s Apartment").c_str() : sharedRoomName(r); }
static std::string roomNameS(int r) { return r < NCHAR ? std::string(ROSTER[r].name) + "'s Apartment" : std::string(sharedRoomName(r)); }
static std::vector<int> ownedOfCat(int cat) { std::vector<int> v; for (int i = 0; i < NITEMS; i++) if (ITEMS[i].cat == cat && G.bag[i] > 0) v.push_back(i); return v; }
static void bagCell(int k, float& x, float& y) { x = PANEL_X + 22 + (k % 4) * 100; y = 178 + (k / 4) * 108; }

// ---------------------------------------------------------------- simulation per frame
static void updateWorld(float dt) {
    static float ltTimer = 0; ltTimer -= dt; if (ltTimer <= 0) { ltTimer = 1.f; refreshLocalTime(); }
    if (G.realClock) {
        int target = LT.h * 60 + LT.mi; int guard = 0;
        while (G.minute != target && guard++ < 1500) tickMinute();
    } else { G.clockAcc += dt / 1.5f; while (G.clockAcc >= 1.f) { G.clockAcc -= 1.f; tickMinute(); } }
    G.curseFlash = std::max(0.f, G.curseFlash - dt);
    curseAmt = 0.25f + 0.35f * G.glitchLevel;
    static int lastTrack = -1; int want = mode == M_TITLE ? TR_TITLE : mode == M_GAME ? TR_GAME : (isNight() ? TR_NIGHT : TR_DAY);
    if (want != lastTrack) { curTrack = want; lastTrack = want; }
    musicVol = G.musicV; sfxVol = G.sfxV;
    float& cd = G.convoCd[G.viewRoom]; cd -= dt; if (cd <= 0) { cd = rr(7, 14); if (!G.convo[G.viewRoom].on) startConvo(G.viewRoom); }
    updateConvo(G.viewRoom, dt);
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i];
        c.sayT -= dt; if (c.sayT <= 0) { c.say.clear(); } c.glitch = std::max(0.f, c.glitch - dt); c.bounce = std::max(0.f, c.bounce - dt * 1.5f);
        if (c.room != G.viewRoom) continue;
        { float tgt = c.state == S_WALK ? 1.f : 0.f; c.walkAmp += (tgt - c.walkAmp) * std::min(1.f, dt * 7.f);
          float dA = c.faceT - c.face; while (dA > 3.14159f) dA -= 6.28318f; while (dA < -3.14159f) dA += 6.28318f; c.face += dA * std::min(1.f, dt * 9.f);
          float lt = (c.state == S_IDLE) ? 0.5f * std::sin(tAnim * 0.45f + i * 1.7f) : 0.f; c.lookYaw += (lt - c.lookYaw) * std::min(1.f, dt * 3.f);
          if (c.state != S_HELD) c.sway *= std::max(0.f, 1.f - dt * 4.f); }
        if (c.state == S_HELD || c.state == S_SLEEP) continue;
        if (c.state == S_EAT) { c.stateT -= dt; if (c.stateT <= 0) c.state = S_IDLE; continue; }
        if (c.state == S_WALK) {
            float dx = c.tx - c.x, dz = c.tz - c.z, d = std::sqrt(dx * dx + dz * dz); float sp = 1.2f * (0.7f + 0.3f * ageScale(ROSTER[i].age));
            if (d < 0.08f) { c.state = S_IDLE; c.stateT = rr(1.5f, 5.f); c.anim = 0; }
            else { c.x += dx / d * sp * dt; c.z += dz / d * sp * dt; c.faceT = std::atan2(dx, dz); c.anim += dt * (6.5f + sp * 3.f); }
        } else if (c.state == S_IDLE) {
            c.stateT -= dt;
            if (c.stateT <= 0 && !(G.convo[c.room].on && (G.convo[c.room].a == i || G.convo[c.room].b == i))) { randomSpot(c.room, c.tx, c.tz); c.state = S_WALK; }
        }
    }
    autosaveT += dt; if (autosaveT > 90 && mode != M_GAME && mode != M_TITLE) { saveGame(); autosaveT = 0; }
    talkCd = std::max(0.f, talkCd - dt);
}

// ---------------------------------------------------------------- the 3D viewport
static void renderViewport() {
    const float VW = PANEL_X;
    if (mode == M_TITLE || mode == M_SETUP || mode == M_MAP || mode == M_SHOP || mode == M_PEOPLE || mode == M_PROFILE || mode == M_PHONE || mode == M_SETTINGS) {
        float w = (mode == M_TITLE || mode == M_SETUP) ? SCR_W : VW;
        drawSkyBackdrop(0, 0, w, SCR_H, tAnim);
        S3.clear(); S3.cam.vx = 0; S3.cam.vy = 0; S3.cam.vw = w; S3.cam.vh = SCR_H; drawIsland(S3, tAnim, camYaw); S3.render();
        // building labels
        struct L { const char* t; V3 p; }; L ls[] = {{"14 TOWER", {-1.5f, 5.2f, -1.6f}}, {"LOBBY", {-1.5f, 1.3f, -0.45f}}, {"ICE CREAM", {3.1f, 2.9f, 0.45f}}, {"SHOP", {3.3f, 1.9f, -2.5f}}, {"PARKING 1", {-4.3f, 1.0f, 3.0f}}};
        for (auto& l : ls) { float sx, sy, d; if (S3.cam.project(l.p, sx, sy, d)) { float tw = textW(l.t, 15) + 12; fillRect(sx - tw / 2, sy - 12, tw, 24, C(16, 14, 40, 200)); strokeRect(sx - tw / 2, sy - 12, tw, 24, UIA, 1); text(l.t, sx, sy - 8, 15, WHITE, 1); } }
        if (mode != M_TITLE && in.down && in.px < VW && in.sx < VW && mode == M_MAP) camYaw -= (in.px - in.sx) * 0.0; // (swipe handled in input)
        return;
    }
    // room
    bool outside = G.viewRoom == ROOM_PARK;
    if (outside) drawSkyBackdrop(0, 0, VW, SCR_H, tAnim); else fillRect(0, 0, VW, SCR_H, C(18, 14, 10));
    Col t, b; float l; skyColors(t, b, l);
    S3.clear(); S3.light = outside ? l : std::max(0.8f, l * 0.55f + 0.5f); S3.cam.vx = 0; S3.cam.vy = 0; S3.cam.vw = VW; S3.cam.vh = SCR_H;
    S3.cam.pos = {std::sin(tAnim * 0.15f) * 0.5f, 4.9f, 8.3f}; S3.cam.target = {0, 1.0f, -0.2f}; S3.cam.fov = 0.95f; S3.cam.update();
    drawRoom(S3, G.viewRoom, tAnim);
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i]; if (c.room != G.viewRoom) continue;
        CharPose P; V3 p = charPos(i); float yaw = c.face; P.t = tAnim; P.phase = c.anim; P.amp = c.walkAmp; P.lookYaw = c.lookYaw; P.sway = c.sway;
        P.bounce = c.bounce > 0 ? 0.16f * std::fabs(std::sin(tAnim * 12.f)) * std::min(1.f, c.bounce) : 0.f; P.cheer = c.bounce > 0.35f && c.state != S_SLEEP;
        P.talk = !c.say.empty() && !c.thought && c.state != S_SLEEP; P.sad = c.mood < 28 && c.state != S_SLEEP;
        if (c.state == S_SLEEP) { P.lying = true; P.expr = 4; yaw = 0; }
        else if (c.state == S_HELD) { p.y += 0.6f; P.held = true; P.expr = 1; }
        else if (c.state == S_EAT) { P.expr = 1; P.eat = true; }
        else P.expr = c.mood < 28 ? 2 : c.mood > 72 ? 1 : 0;
        if (c.state == S_CHAT) { P.expr = c.mood < 28 ? 2 : 1; }
        drawCharacter(S3, i, p, yaw, P);
        if (c.state == S_SLEEP) { Col bc = mixc(fromHex(ROSTER[i].top), C(255, 255, 255), 0.35f); S3.box(xf({-2.2f, 0.78f, -1.9f}), {0.78f, 0.09f, 0.52f}, bc); S3.sphere(xf({-2.9f, 0.8f, -1.9f}), {0.2f, 0.1f, 0.5f}, bc, 8, 4); }
        if (c.room >= 0 && c.room < NCHAR && c.state != S_SLEEP) {}
    }
    for (int i = 0; i < NCHAR; i++) { Char& c = G.ch[i]; if (c.room != G.viewRoom) continue; // each owner's cleaning bot tags along
        float tx = c.x + 0.75f, tz = c.z + 0.55f; c.botX += (tx - c.botX) * 0.04f; c.botZ += (tz - c.botZ) * 0.04f; if (std::fabs(c.botX) > 9) { c.botX = tx; c.botZ = tz; }
        drawBot(S3, i, {std::max(-3.7f, std::min(3.7f, c.botX)), 0, std::max(-2.6f, std::min(2.7f, c.botZ))}, tAnim); }
    S3.render();
    if (photoReq) {
        photoReq = false; photoFlash = 0.25f; ensureDirs();
        SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, (int)VW, SCR_H, 32, SDL_PIXELFORMAT_ARGB8888); SDL_Rect rc = {0, 0, (int)VW, SCR_H};
        if (s && SDL_RenderReadPixels(R, &rc, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch) == 0) { char fn[256]; snprintf(fn, sizeof fn, "%sphotos/ilot14_%d_%04d.bmp", SAVE_DIR, G.day, (int)(SDL_GetTicks() % 10000)); SDL_SaveBMP(s, fn); say("Photo saved to the photos folder."); sfx(SFX_BELL); }
        if (s) SDL_FreeSurface(s);
    }
    // ---- overlays
    if (G.viewRoom == I_DALIA) { float sx, sy, d; if (S3.cam.project({3.3f, 2.25f, -3.0f}, sx, sy, d)) text("STAIRWAY", sx, sy, 14, UIA, 1); }
    float labelX, labelY, dd;
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i]; if (c.room != G.viewRoom) continue; float hx, hy; headScreen(i, hx, hy);
        if (c.bubble != B_NONE && c.state != S_HELD) drawStatusBubble(hx, hy - 6, c.bubble, c.bArg, tAnim + i);
        if (G.selected == i || c.state == S_HELD) { float w = textW(nm(i), 16) + 12; fillRect(hx - w / 2, hy + 10, w, 22, C(16, 14, 40, 220)); strokeRect(hx - w / 2, hy + 10, w, 22, UIA, 1); text(nm(i), hx, hy + 13, 16, WHITE, 1); }
    }
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i]; if (c.room != G.viewRoom || c.say.empty()) continue; float hx, hy; headScreen(i, hx, hy);
        drawSpeech(hx, hy - (c.bubble != B_NONE ? 92 : 8), c.say, c.thought, nm(i), 0, VW, fromHex(ROSTER[i].top));
    }
    (void)labelX; (void)labelY; (void)dd;
    // selected card
    if (G.selected >= 0 && G.ch[G.selected].room == G.viewRoom) {
        Char& c = G.ch[G.selected]; float x = 14, y = SCR_H - 108;
        fillRect(x, y, 290, 94, C(16, 14, 40, 225)); strokeRect(x, y, 290, 94, UIA, 2);
        textf(x + 10, y + 6, 20, WHITE, 0, "%s  (%s)", ROSTER[G.selected].name, ROSTER[G.selected].code);
        bar(x + 10, y + 34, 180, 14, c.mood, C(250, 204, 21), "MOOD"); bar(x + 10, y + 54, 180, 14, c.hunger, C(74, 222, 128), "FULL"); bar(x + 10, y + 74, 180, 14, c.bond, C(249, 115, 22), "BOND");
        text(PERS_NAME[ROSTER[G.selected].pers], x + 200, y + 34, 14, C(148, 163, 184));
    }
    { char buf[128]; snprintf(buf, sizeof buf, "%s", roomNameS(G.viewRoom).c_str()); float w = textW(buf, 18) + 20; fillRect(14, 14, w, 30, C(16, 14, 40, 225)); strokeRect(14, 14, w, 30, UIA, 2); text(buf, 24, 19, 18, UIA); }
    if (G.curseFlash > 0) { fillRect(0, 0, VW, SCR_H, C(168, 85, 247, (int)(G.curseFlash * 60 * G.glitchLevel))); for (int k = 0; k < 6 * G.glitchLevel; k++) fillRect(0, (float)(rnd() % SCR_H), VW, 2 + rnd() % 4, C(249, 115, 22, (int)(G.curseFlash * 140))); }
    if (photoFlash > 0) fillRect(0, 0, VW, SCR_H, C(255, 255, 255, (int)(photoFlash * 900)));
    if (dlg.open) {
        fillRect(0, 0, VW, SCR_H, C(0, 0, 0, 140)); float x = 120, y = 220, w = 600, h = 230; UIA = C(244, 114, 182);
        vgrad(x, y, w, h, BGA, BGB); strokeRect(x, y, w, h, UIA, 3); text(dlg.who.c_str(), x + 18, y + 12, 22, UIA);
        textWrap(dlg.body.c_str(), x + 18, y + 48, w - 36, 22, WHITE);
        if (!dlg.b2.empty()) { if (btn(x + 40, y + h - 66, 240, 50, dlg.b1.c_str(), 22, UIA, true, true)) onDialog(1); if (btn(x + w - 280, y + h - 66, 240, 50, dlg.b2.c_str(), 22)) onDialog(2); }
        else if (btn(x + w / 2 - 120, y + h - 66, 240, 50, dlg.b1.c_str(), 22, UIA, true, true)) onDialog(1);
    }
}

// ---------------------------------------------------------------- right-hand 2D screens
static void footerBack(const char* label = "< BACK") { if (btn(PANEL_X + 20, 652, 150, 50, label, 22)) { mode = M_MAP; } }
static const Col TABC[3] = {{34, 211, 238, 255}, {74, 222, 128, 255}, {192, 132, 252, 255}};
static void setAccent() {
    switch (mode) { case M_MAP: UIA = C(34, 211, 238); break; case M_SHOP: UIA = C(74, 222, 128); break; case M_PEOPLE: UIA = C(192, 132, 252); break; case M_PROFILE: UIA = C(244, 114, 182); break;
        case M_PHONE: UIA = C(45, 212, 191); break; case M_SETTINGS: UIA = C(96, 165, 250); break; case M_ROOM: UIA = TABC[tab]; break; default: UIA = C(249, 115, 22); break; }
}
static void drawMapScreen() {
    header("MAP");
    // stylized 2D island chart
    float ox = PANEL_X, oy = 70;
    fillRect(ox + 14, oy, 412, 560, C(10, 24, 50)); strokeRect(ox + 14, oy, 412, 560, C(40, 70, 120), 2);
    for (int k = 0; k < 9; k++) lineD(ox + 14, oy + 60 * k, ox + 426, oy + 60 * k, C(18, 40, 80), 1);
    std::vector<SDL_FPoint> isl = {{ox + 60, oy + 120}, {ox + 160, oy + 40}, {ox + 300, oy + 50}, {ox + 390, oy + 130}, {ox + 410, oy + 280}, {ox + 380, oy + 430}, {ox + 280, oy + 520}, {ox + 130, oy + 500}, {ox + 50, oy + 400}, {ox + 40, oy + 250}};
    poly(isl, C(160, 135, 85)); std::vector<SDL_FPoint> in2; for (auto p : isl) { p.x = ox + 220 + (p.x - ox - 220) * 0.93f; p.y = oy + 280 + (p.y - oy - 280) * 0.93f; in2.push_back(p); } poly(in2, C(36, 100, 58));
    lineD(ox + 60, oy + 262, ox + 400, oy + 262, C(60, 62, 70), 18); for (int i = 0; i < 12; i++) fillRect(ox + 70 + i * 28, oy + 260, 14, 3, C(240, 220, 120));
    static const Col mcol[10] = {{34, 211, 238, 255}, {251, 146, 60, 255}, {244, 114, 182, 255}, {96, 165, 250, 255}, {74, 222, 128, 255}, {192, 132, 252, 255}, {45, 212, 191, 255}, {248, 113, 113, 255}, {148, 163, 184, 255}, {250, 204, 21, 255}};
    struct MB { int id; const char* label; float x, y; int icon; };
    static const MB mbs[] = {{0, "14 TOWER", 120, 140, 0}, {1, "LOBBY", 250, 200, 1}, {2, "ICE CREAM", 340, 120, 2}, {3, "PARKING 1", 90, 340, 3}, {4, "SHOP", 340, 340, 4},
                             {5, "PEOPLE", 110, 450, 5}, {6, "PHONE", 225, 450, 6}, {7, "PHOTO", 340, 450, 7}, {8, "SETTINGS", 110, 560, 8}, {9, "SAVE", 340, 560, 9}};
    for (auto& m : mbs) {
        float cx = ox + m.x, cy = oy + m.y; float sz = m.id == 0 ? 92 : 78; float x = cx - sz / 2, y = cy - sz / 2; bool pr = pressedIn(x, y, sz, sz);
        fillRect(x, y, sz, sz, pr ? mixc(C(30, 28, 70), mcol[m.id], 0.45f) : mixc(C(30, 28, 70), mcol[m.id], 0.12f)); strokeRect(x, y, sz, sz, mcol[m.id], 3);
        drawIcon(m.icon, cx, cy - 4, sz * 0.5f, mcol[m.id]);
        text(m.label, cx, y + sz + 3, 14, WHITE, 1);
        if (clickedIn(x, y, sz, sz)) {
            sfx(SFX_TAP);
            switch (m.id) {
                case 0: goRoom(G.selected >= 0 && G.selected < NCHAR ? G.selected : 0); tab = 0; break;
                case 1: goRoom(ROOM_LOBBY); tab = 0; break; case 2: goRoom(ROOM_ICE); tab = 0; break; case 3: goRoom(ROOM_PARK); tab = 0; break;
                case 4: mode = M_SHOP; break; case 5: mode = M_PEOPLE; break; case 6: mode = M_PHONE; break;
                case 7: photoReq = true; say("Click! (photos appear when you are in a room)"); break;
                case 8: mode = M_SETTINGS; break; case 9: say(saveGame() ? "Saved to SD card." : "Save failed!"); sfx(SFX_BELL); break;
            }
        }
    }
    text("drag the island view to spin it", ox + PW_ / 2, 688, 13, C(130, 140, 190), 1);
}
static void drawShop() {
    header("SHOP");
    static const char* cats[3] = {"FOOD", "CLOTHES", "GOODS"};
    for (int i = 0; i < 3; i++) if (btn(PANEL_X + 14 + i * 142, 70, 136, 40, cats[i], 20, UIA, true, shopCat == i)) shopCat = i;
    int row = 0;
    for (int i = 0; i < NITEMS; i++) {
        if (ITEMS[i].cat != shopCat || ITEMS[i].price == 0) continue;
        float y = 120 + row * 46; row++;
        itemIcon(i, PANEL_X + 16, y, 40);
        text(ITEMS[i].name, PANEL_X + 66, y + 3, 19, WHITE); textf(PANEL_X + 66, y + 24, 14, C(250, 204, 21), 0, "%d DA   (have %d)", ITEMS[i].price, G.bag[i]);
        bool can = G.money >= ITEMS[i].price;
        if (btn(PANEL_X + 330, y, 96, 40, "BUY", 20, UIA, can, can)) { G.money -= ITEMS[i].price; G.bag[i]++; sfx(SFX_COIN); say(std::string("Bought ") + ITEMS[i].name); }
        else if (!can && clickedIn(PANEL_X + 330, y, 96, 40)) say("Not enough money.");
    }
    footerBack();
}
static void drawPeople() {
    header("PEOPLE");
    for (int i = 0; i < NCHAR; i++) {
        float x = PANEL_X + 12 + (i % 2) * 212, y = 72 + (i / 2) * 44; bool pr = pressedIn(x, y, 206, 40);
        fillRect(x, y, 206, 40, pr ? C(64, 60, 140) : C(30, 28, 70)); strokeRect(x, y, 206, 40, C(76, 78, 150), 1);
        fillRect(x + 6, y + 8, 24, 24, fromHex(ROSTER[i].top)); disc(x + 18, y + 12, 6, 6, fromHex(SKINS[ROSTER[i].skin]));
        text(ROSTER[i].name, x + 38, y + 2, 17, WHITE); textf(x + 38, y + 22, 12, C(148, 163, 184), 0, "%s  %s", ROSTER[i].code, G.ch[i].bubble == B_NONE ? "" : "!");
        disc(x + 190, y + 20, 7, 7, G.ch[i].mood < 28 ? C(59, 130, 246) : G.ch[i].mood > 70 ? C(74, 222, 128) : C(250, 204, 21), 10);
        if (clickedIn(x, y, 206, 40)) { profileIdx = i; mode = M_PROFILE; sfx(SFX_TAP); }
    }
    footerBack();
}
static void drawProfile() {
    header("PROFILE"); int i = profileIdx; Char& c = G.ch[i];
    fillRect(PANEL_X + 14, 72, 412, 100, C(30, 28, 70)); strokeRect(PANEL_X + 14, 72, 412, 100, UIA, 2);
    fillRect(PANEL_X + 26, 84, 76, 76, fromHex(ROSTER[i].top)); disc(PANEL_X + 64, 104, 22, 22, fromHex(SKINS[ROSTER[i].skin])); disc(PANEL_X + 64, 98, 24, 15, fromHex(HAIRS[ROSTER[i].hair]));
    text(ROSTER[i].full, PANEL_X + 116, 80, 22, WHITE); textf(PANEL_X + 116, 108, 16, UIA, 0, "%s - age %d", ROSTER[i].code, ROSTER[i].age);
    textf(PANEL_X + 116, 130, 16, C(148, 163, 184), 0, "%s  |  loves %s", PERS_NAME[ROSTER[i].pers], ITEMS[favFood(i)].name);
    textf(PANEL_X + 116, 150, 14, C(148, 163, 184), 0, "fave thing: %s%s", ITEMS[favGood(i)].name, c.hasPhone ? "  (has phone)" : "");
    bar(PANEL_X + 20, 184, 400, 16, c.mood, C(250, 204, 21), "MOOD"); bar(PANEL_X + 20, 206, 400, 16, c.hunger, C(74, 222, 128), "FULL"); bar(PANEL_X + 20, 228, 400, 16, c.energy, C(96, 165, 250), "ENERGY"); bar(PANEL_X + 20, 250, 400, 16, c.bond, C(249, 115, 22), "BOND WITH YOU");
    text("RELATIONSHIPS", PANEL_X + 20, 280, 18, UIA);
    std::vector<int> order; for (int j = 0; j < NCHAR; j++) if (j != i) order.push_back(j);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return c.aff[a] > c.aff[b]; });
    for (int k = 0; k < 8; k++) { int j = order[k]; float y = 306 + k * 36; text(ROSTER[j].name, PANEL_X + 20, y, 17, WHITE); bar(PANEL_X + 150, y + 2, 140, 14, c.aff[j], tierCol(i, j)); text(tierName(i, j), PANEL_X + 300, y + 1, 15, tierCol(i, j)); }
    if (btn(PANEL_X + 20, 652, 190, 50, "VISIT ROOM", 22, UIA, true, true)) { G.selected = i; goRoom(c.room); tab = 2; }
    if (btn(PANEL_X + 230, 652, 190, 50, "< LIST", 22)) mode = M_PEOPLE;
}
static void drawPhone() {
    header("PHONE");
    if (btn(PANEL_X + 14, 70, 200, 40, "TEXTS", 20, UIA, true, phoneTab == 0)) phoneTab = 0;
    if (btn(PANEL_X + 226, 70, 200, 40, "NOTICES", 20, UIA, true, phoneTab == 1)) phoneTab = 1;
    auto& L = phoneTab == 0 ? G.texts : G.notices; float y = 124;
    if (L.empty()) text(phoneTab == 0 ? "No texts yet. Give someone a Phone!" : "Nothing to report. Yet.", PANEL_X + 24, y, 16, C(148, 163, 184));
    for (auto& s : L) { if (y > 620) break; int n = textWrap(s.c_str(), PANEL_X + 24, y, 392, 16, WHITE); y += n * 19 + 10; fillRect(PANEL_X + 20, y - 6, 400, 1, C(50, 52, 100)); }
    footerBack();
}
static void drawSettings() {
    header("SETTINGS");
    auto row = [&](int k, const char* label, float v, float* tgt, float step) {
        float y = 84 + k * 62; text(label, PANEL_X + 20, y + 8, 20, WHITE); bar(PANEL_X + 200, y + 10, 120, 18, v * 100, UIA);
        if (btn(PANEL_X + 330, y, 44, 42, "-", 24)) { *tgt = std::max(0.f, *tgt - step); } if (btn(PANEL_X + 380, y, 44, 42, "+", 24)) { *tgt = std::min(1.f, *tgt + step); }
    };
    row(0, "Music", G.musicV, &G.musicV, 0.1f); row(1, "Effects", G.sfxV, &G.sfxV, 0.1f);
    float y = 84 + 2 * 62; text("Curse level", PANEL_X + 20, y + 8, 20, WHITE); textf(PANEL_X + 200, y + 8, 20, UIA, 0, "%s", G.glitchLevel == 0 ? "calm" : G.glitchLevel == 1 ? "normal" : "cursed!");
    if (btn(PANEL_X + 330, y, 44, 42, "-", 24)) G.glitchLevel = std::max(0, G.glitchLevel - 1); if (btn(PANEL_X + 380, y, 44, 42, "+", 24)) G.glitchLevel = std::min(2, G.glitchLevel + 1);
    y = 84 + 3 * 62; text("Clock", PANEL_X + 20, y + 8, 20, WHITE); if (btn(PANEL_X + 200, y, 224, 42, G.realClock ? "REAL TIME (device)" : "FAST (36 min/day)", 18)) G.realClock ^= 1;
    y = 84 + 4 * 62; if (btn(PANEL_X + 20, y, 404, 46, "SKIP AHEAD 1 HOUR", 20)) { for (int k = 0; k < 60; k++) tickMinute(); say("An hour passed. Probably."); }
    y = 84 + 5 * 62; if (btn(PANEL_X + 20, y, 404, 46, "SAVE NOW", 20)) say(saveGame() ? "Saved to SD card." : "Save failed!");
    y = 84 + 6 * 62; if (btn(PANEL_X + 20, y, 404, 46, "NEW GAME (erase island)", 20, C(239, 68, 68))) openDlg(4, 0, 0, "Are you sure?", "This will erase the current island and start over.", "Erase it", "Cancel");
    y = 84 + 7 * 62; if (btn(PANEL_X + 20, y, 404, 46, "EXIT TO HOME MENU", 20)) openDlg(5, 0, 0, "Leave the island?", "Progress is saved automatically.", "Exit", "Stay");
    textf(PANEL_X + 20, 604, 13, C(130, 140, 190), 0, "Music: put menu/day/night/minigame .wav in");
    text("sdmc:/switch/ilot14_island/music/ to replace it.", PANEL_X + 20, 622, 13, C(130, 140, 190));
    footerBack();
}

static void drawRoomPanel() {
    header();
    static const char* tabs[3] = {"ROOMS", "BAG", "INFO"};
    for (int i = 0; i < 3; i++) if (btn(PANEL_X + 14 + i * 142, 68, 136, 40, tabs[i], 20, TABC[i], true, tab == i)) tab = i;
    if (tab == 0) {
        text("drag someone onto a room to move them", PANEL_X + PW_ / 2, 116, 14, C(148, 163, 184), 1);
        for (int r = 0; r < NROOMS; r++) {
            float x, y; roomCell(r, x, y); char buf[8]; bool cur = r == G.viewRoom; int pop = roomPop(r);
            bool hot = drag.grabbed && drag.kind == 1 && inside(x, y, 76, 66, in.px, in.py);
            fillRect(x, y, 76, 66, hot ? C(84, 80, 176) : cur ? C(48, 46, 110) : C(30, 28, 70)); strokeRect(x, y, 76, 66, cur ? UIA : C(76, 78, 150), cur ? 3 : 1);
            text(roomShort(r, buf), x + 38, y + 8, 18, r >= NCHAR ? C(250, 204, 21) : WHITE, 1);
            for (int k = 0; k < std::min(pop, 8); k++) fillRect(x + 8 + (k % 4) * 15, y + 36 + (k / 4) * 13, 10, 10, C(249, 115, 22));
            bool hasBubble = false; for (int i = 0; i < NCHAR; i++) if (G.ch[i].room == r && G.ch[i].bubble != B_NONE && G.ch[i].bubble != B_DREAM) hasBubble = true;
            if (hasBubble) disc(x + 64, y + 10, 5, 5, C(250, 204, 21), 8);
            if (clickedIn(x, y, 76, 66) && !drag.grabbed && drag.kind != 2) { G.viewRoom = r; sfx(SFX_TAP); }
        }
        text(roomNameS(G.viewRoom).c_str(), PANEL_X + PW_ / 2, 580, 18, UIA, 1);
    } else if (tab == 1) {
        static const char* cats[3] = {"FOOD", "CLOTHES", "GOODS"};
        for (int i = 0; i < 3; i++) if (btn(PANEL_X + 14 + i * 142, 118, 136, 36, cats[i], 18, TABC[1], true, bagCat == i)) bagCat = i;
        std::vector<int> v = ownedOfCat(bagCat);
        if (v.empty()) text("Nothing here. Visit the shop!", PANEL_X + PW_ / 2, 240, 16, C(148, 163, 184), 1);
        for (size_t k = 0; k < v.size() && k < 16; k++) {
            float x, y; bagCell((int)k, x, y); itemIcon(v[k], x, y, 90);
            fillRect(x + 56, y + 62, 34, 26, BLACK0); textf(x + 73, y + 64, 20, C(250, 204, 21), 1, "x%d", G.bag[v[k]]);
            text(ITEMS[v[k]].name, x + 45, y + 90, 12, WHITE, 1);
        }
        text("Drag an item onto someone in the room", PANEL_X + PW_ / 2, 604, 14, C(148, 163, 184), 1);
    } else {
        int i = G.selected;
        if (i < 0) text("Tap someone in the room.", PANEL_X + PW_ / 2, 200, 18, C(148, 163, 184), 1);
        else {
            Char& c = G.ch[i];
            text(ROSTER[i].full, PANEL_X + 20, 124, 24, WHITE); textf(PANEL_X + 20, 154, 16, UIA, 0, "%s  age %d  %s", ROSTER[i].code, ROSTER[i].age, PERS_NAME[ROSTER[i].pers]);
            bar(PANEL_X + 20, 184, 400, 16, c.mood, C(250, 204, 21), "MOOD"); bar(PANEL_X + 20, 206, 400, 16, c.hunger, C(74, 222, 128), "FULL"); bar(PANEL_X + 20, 228, 400, 16, c.energy, C(96, 165, 250), "ENERGY"); bar(PANEL_X + 20, 250, 400, 16, c.bond, C(249, 115, 22), "BOND");
            textf(PANEL_X + 20, 276, 15, C(148, 163, 184), 0, "loves %s | fave: %s", ITEMS[favFood(i)].name, ITEMS[favGood(i)].name);
            std::string wear; for (int k = 0; k < 4; k++) if (c.outfit[k] >= 0) wear += std::string(wear.empty() ? "" : ", ") + ITEMS[c.outfit[k]].name; textf(PANEL_X + 20, 298, 14, C(148, 163, 184), 0, "wearing: %s", wear.empty() ? "default look" : wear.c_str());
            if (btn(PANEL_X + 20, 330, 126, 46, "TALK", 20, UIA, talkCd <= 0 && c.state != S_SLEEP)) { talkCd = 4; int in2[NCHAR]; int n = roomPop(c.room, in2, NCHAR); int o = in2[ri(n)]; if (o == i && n > 1) o = in2[(ri(n - 1) + 1) % n]; speak(i, o == i ? LINE(i, L_BORED) : chatLine(i, o), 4.f); addBond(i, 1.5f); addMood(i, 2); }
            if (btn(PANEL_X + 157, 330, 126, 46, "THINK", 20, UIA, c.state != S_SLEEP)) { int o = ri(NCHAR); if (o == i) o = (i + 1) % NCHAR; float a = c.aff[o]; std::string t = a >= 55 ? fmtName("I really like hanging out with {n}.", nm(o)) : a >= 30 ? fmtName("{n} is actually pretty cool.", nm(o)) : a < 10 ? fmtName("{n} gives me weird vibes.", nm(o)) : fmtName("I wonder what {n} is up to.", nm(o)); if (isHateCrush(i, o) && i == I_DALIA) t = "Gabriel. Again. Why."; speak(i, t, 4.4f, true); }
            if (btn(PANEL_X + 294, 330, 126, 46, "PROFILE", 20)) { profileIdx = i; mode = M_PROFILE; }
            text("TOP RELATIONSHIPS", PANEL_X + 20, 396, 17, UIA);
            std::vector<int> order; for (int j = 0; j < NCHAR; j++) if (j != i) order.push_back(j);
            std::sort(order.begin(), order.end(), [&](int a, int b) { return c.aff[a] > c.aff[b]; });
            for (int k = 0; k < 5; k++) { int j = order[k]; float y = 424 + k * 34; text(ROSTER[j].name, PANEL_X + 20, y, 17, WHITE); bar(PANEL_X + 150, y + 2, 130, 14, c.aff[j], tierCol(i, j)); text(tierName(i, j), PANEL_X + 292, y + 1, 15, tierCol(i, j)); }
        }
    }
    if (btn(PANEL_X + 14, 652, 120, 50, "MAP", 22)) { mode = M_MAP; curTrack = isNight() ? TR_NIGHT : TR_DAY; }
    if (btn(PANEL_X + 146, 652, 130, 50, "PHOTO", 22)) photoReq = true;
    if (btn(PANEL_X + 288, 652, 138, 50, "SAVE", 22, C(250, 204, 21))) { say(saveGame() ? "Saved to SD card." : "Save failed!"); sfx(SFX_BELL); }
}

// ---------------------------------------------------------------- room input: tap, hold-and-drag, give
static void roomInput(float dt) {
    if (dlg.open || mode != M_ROOM) { drag = Drag(); return; }
    // begin
    if (in.pressed) {
        drag = Drag(); drag.sx = in.px; drag.sy = in.py;
        if (in.px < PANEL_X) {
            int hit = -1;
            for (int i = 0; i < NCHAR; i++) { Char& c = G.ch[i]; if (c.room != G.viewRoom || c.bubble == B_NONE || c.state == S_HELD) continue; float hx, hy; headScreen(i, hx, hy); hy += -6 - 34 + (std::sin(tAnim * 5 + i) * 3); if ((in.px - hx) * (in.px - hx) + (in.py - hy) * (in.py - hy) < 32 * 32) hit = i; }
            if (hit >= 0) { tapBubble(hit); G.selected = hit; drag = Drag(); drag.id = -2; }
            else {
                int ch = pickChar(in.px, in.py);
                if (ch >= 0) { drag.kind = 1; drag.id = ch; }
                else {
                    // props
                    for (size_t k = 0; k < G.props.size(); k++) if (G.props[k].room == G.viewRoom) { float sx, sy, d; if (S3.cam.project({G.props[k].x, 0.15f, G.props[k].z}, sx, sy, d) && std::fabs(in.px - sx) < 34 && std::fabs(in.py - sy) < 34) { G.bag[G.props[k].item]++; G.props.erase(G.props.begin() + k); sfx(SFX_POP); say("Picked it back up."); drag.id = -2; break; } }
                    if (drag.id != -2 && G.viewRoom == I_DALIA) { float sx, sy, d; if (S3.cam.project({3.3f, 1.0f, -3.0f}, sx, sy, d) && std::fabs(in.px - sx) < 60 && std::fabs(in.py - sy) < 100) {
                        if (G.selected >= 0 && G.ch[G.selected].room == I_DALIA) { int w = G.selected; setRoom(w, ROOM_PARK, false); say(std::string(nm(w)) + " went through the stairway door. It led to the parking lot."); sfx(SFX_GLITCH); } else say("The stairway door. Select someone first. Don't open it yourself."); drag.id = -2; } }
                    if (drag.id != -2) G.selected = -1;
                }
            }
        } else if (tab == 1) {
            std::vector<int> v = ownedOfCat(bagCat);
            for (size_t k = 0; k < v.size() && k < 16; k++) { float x, y; bagCell((int)k, x, y); if (inside(x, y, 90, 90, in.px, in.py)) { drag.kind = 2; drag.id = v[k]; drag.grabbed = true; sfx(SFX_POP); } }
        }
    }
    if (drag.kind == 1 && in.down) {
        drag.t += dt;
        if (!drag.grabbed && G.ch[drag.id].state != S_SLEEP && (std::hypot(in.px - drag.sx, in.py - drag.sy) > 14 || drag.t > 0.35f)) {
            drag.grabbed = true; G.ch[drag.id].state = S_HELD; G.selected = drag.id; sfx(SFX_POP); if (G.convo[G.viewRoom].a == drag.id || G.convo[G.viewRoom].b == drag.id) G.convo[G.viewRoom].on = false;
        }
        if (drag.grabbed && G.ch[drag.id].state == S_HELD) { V3 h; if (S3.cam.hitPlaneY(in.px, in.py, 0.9f, h)) { float ox = G.ch[drag.id].x; float nx = std::max(-3.7f, std::min(3.7f, h.x)); G.ch[drag.id].sway += (std::max(-1.f, std::min(1.f, (nx - ox) / std::max(dt, 0.001f) * 0.2f)) - G.ch[drag.id].sway) * 0.25f; G.ch[drag.id].x = nx; G.ch[drag.id].z = std::max(-2.6f, std::min(2.8f, h.z)); } }
    }
    if (in.released && drag.id >= 0) {
        if (drag.kind == 1) {
            int i = drag.id; Char& c = G.ch[i];
            if (!drag.grabbed) { if (c.state != S_SLEEP || true) tapChar(i); }
            else {
                c.state = S_IDLE; c.stateT = 1.f; sfx(SFX_DROP);
                if (in.px >= PANEL_X) {
                    bool moved = false;
                    if (tab == 0) for (int r = 0; r < NROOMS; r++) { float x, y; roomCell(r, x, y); if (inside(x, y, 76, 66, in.px, in.py)) { if (r != c.room) { setRoom(i, r, false); say(std::string(nm(i)) + " -> " + roomNameS(r)); G.selected = -1; } moved = true; } }
                    (void)moved;
                } else { int t = pickChar(in.px, in.py, i); if (t >= 0) dropOn(i, t); }
                c.tx = c.x; c.tz = c.z; if (c.hunger < 1) c.hunger = 1;
            }
        } else if (drag.kind == 2) {
            int item = drag.id;
            if (in.px < PANEL_X) {
                int t = pickChar(in.px, in.py);
                if (t >= 0) { if (G.ch[t].state == S_SLEEP) say("They're asleep!"); else { std::string r; giveItem(t, item, r); G.selected = t; } }
                else if (ITEMS[item].cat != CAT_FOOD && ITEMS[item].cat != CAT_CLOTH) { V3 h; int cnt = 0; for (auto& p : G.props) if (p.room == G.viewRoom) cnt++; if (cnt < 10 && S3.cam.hitPlaneY(in.px, in.py, 0.f, h) && std::fabs(h.x) < 3.8f && h.z > -2.7f && h.z < 2.8f) { G.bag[item]--; G.props.push_back({G.viewRoom, item, h.x, h.z}); sfx(SFX_DROP); } }
                else say("Give it to someone!");
            }
        }
        drag = Drag();
    } else if (in.released) drag = Drag();
}


// ---------------------------------------------------------------- first-time setup
struct Setup { int step = 0; std::string name; bool shift = true; int clock = 1, curse = 1, pack = 0; float music = 0.5f, sfxv = 0.7f; };
static Setup setup; static std::string typedText;
static const int SETUP_STEPS = 8;
static void finishSetup() {
    initNewGame();
    snprintf(G.playerName, sizeof G.playerName, "%s", setup.name.empty() ? "Player" : setup.name.c_str());
    G.realClock = setup.clock; G.glitchLevel = setup.curse; G.musicV = setup.music; G.sfxV = setup.sfxv;
    if (G.realClock) { refreshLocalTime(); G.minute = LT.h * 60 + LT.mi; }
    for (int i = 0; i < NITEMS; i++) G.bag[i] = 0;
    G.bag[0] = 1; G.bag[6] = 1; G.bag[8] = 1; G.bag[26] = 1;                          // everybody gets a small base kit
    if (setup.pack == 0) { G.bag[0] += 3; G.bag[2] += 2; G.bag[8] += 3; G.bag[6] += 2; G.bag[3] += 1; G.bag[9] += 2; }
    else if (setup.pack == 1) { G.bag[11] = 1; G.bag[12] = 1; G.bag[14] = 1; G.bag[18] = 1; G.bag[20] = 1; G.bag[16] = 1; G.bag[13] = 1; }
    else { G.bag[IT_PHONE] += 2; G.bag[27] += 1; G.bag[25] += 1; G.bag[24] += 1; G.bag[26] += 2; G.bag[29] += 1; }
    G.notices.clear(); notice(std::string("Welcome to Îlot14 Island, ") + G.playerName + ". Everyone moved in. Nobody remembers signing anything.");
    G.curseFlash = 0; saveGame(); saveExists = true; mode = M_MAP; say(std::string("Welcome, ") + G.playerName + "!"); sfx(SFX_BELL);
}
static void startSetup() { setup = Setup(); refreshLocalTime(); mode = M_SETUP; sfx(SFX_TAP); }
static void drawSetup() {
    float cx = SCR_W / 2.f; fillRect(0, 0, SCR_W, SCR_H, C(0, 0, 0, 120));
    float px = 170, py = 50, pw = 940, ph = 620;
    vgrad(px, py, pw, ph, C(20, 16, 52, 245), C(14, 40, 80, 245)); UIA = C(34, 211, 238); strokeRect(px, py, pw, ph, UIA, 3); fillRect(px, py, pw, 6, C(244, 114, 182));
    static const char* titles[SETUP_STEPS] = {"WELCOME", "WHO ARE YOU?", "WHAT TIME IS IT?", "THE ISLAND", "STARTER PACK", "HOW TO PLAY (1/2)", "HOW TO PLAY (2/2)", "READY?"};
    static const Col stepCol[SETUP_STEPS] = {{34, 211, 238, 255}, {244, 114, 182, 255}, {250, 204, 21, 255}, {168, 85, 247, 255}, {74, 222, 128, 255}, {96, 165, 250, 255}, {251, 146, 60, 255}, {244, 114, 182, 255}};
    UIA = stepCol[setup.step];
    text(titles[setup.step], cx, py + 24, 40, UIA, 1);
    for (int i = 0; i < SETUP_STEPS; i++) disc(cx - (SETUP_STEPS - 1) * 14 + i * 28, py + 84, i == setup.step ? 8 : 5, i == setup.step ? 8 : 5, i <= setup.step ? UIA : C(70, 74, 130), 12);
    float y0 = py + 110; bool canNext = true; const char* nextLabel = "NEXT >";
    switch (setup.step) {
        case 0:
            textWrap("26 neighbours just moved into the 14 building on a tiny island, and the building is not entirely normal.", px + 60, y0 + 10, pw - 120, 26, WHITE);
            textWrap("Feed them, dress them, give them phones and gifts, play minigames with them, move them between rooms, and help them make friends. Watch what they do when nobody is looking.", px + 60, y0 + 100, pw - 120, 26, C(200, 210, 235));
            textWrap("This takes about a minute to set up.", px + 60, y0 + 230, pw - 120, 24, UIA);
            break;
        case 1: {
            text("Type your name (up to 12 letters)", cx, y0 - 4, 22, C(200, 210, 235), 1);
            fillRect(cx - 240, y0 + 26, 480, 54, C(10, 10, 30)); strokeRect(cx - 240, y0 + 26, 480, 54, UIA, 2);
            std::string shown = setup.name + (((int)(tAnim * 2)) % 2 ? "_" : " "); text(shown.c_str(), cx, y0 + 36, 32, WHITE, 1);
            static const char* rows[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
            for (int r = 0; r < 4; r++) {
                int n = (int)strlen(rows[r]); float kw = 76, gap = 8; float tw = n * (kw + gap) - gap; float sx = cx - tw / 2; if (r == 3) sx = cx - (tw + 2 * (kw + gap)) / 2 + 0;
                for (int k = 0; k < n; k++) {
                    char lab[2] = {rows[r][k], 0}; char shownCh = (char)((setup.shift || !isalpha((unsigned char)rows[r][k])) ? rows[r][k] : tolower(rows[r][k])); char l2[2] = {shownCh, 0};
                    if (btn(sx + k * (kw + gap), y0 + 100 + r * 70, kw, 62, l2, 28, UIA) && setup.name.size() < 12) { setup.name += shownCh; setup.shift = false; }
                    (void)lab;
                }
                if (r == 3) { float bx = sx + n * (kw + gap); if (btn(bx, y0 + 100 + r * 70, kw * 2 + gap, 62, "DEL", 24, C(248, 113, 113))) { if (!setup.name.empty()) setup.name.pop_back(); if (setup.name.empty()) setup.shift = true; } }
            }
            float by = y0 + 100 + 4 * 70; if (btn(cx - 300, by, 160, 56, setup.shift ? "SHIFT ON" : "SHIFT", 20, UIA, true, setup.shift)) setup.shift = !setup.shift;
            if (btn(cx - 130, by, 360, 56, "SPACE", 22, UIA) && setup.name.size() < 12 && !setup.name.empty()) setup.name += ' ';
            canNext = !setup.name.empty();
            if (!canNext) text("(you can also skip: Player)", cx, py + ph - 100, 16, C(130, 140, 190), 1);
            break; }
        case 2: {
            text("The island runs on your Switch clock:", cx, y0 + 6, 24, C(200, 210, 235), 1);
            text(dateStr().c_str(), cx, y0 + 52, 40, WHITE, 1); text(clockStr().c_str(), cx, y0 + 100, 64, UIA, 1);
            // note: in real mode clockStr() mirrors the game clock, which matches LT
            char tb[16]; snprintf(tb, sizeof tb, "%02d:%02d", LT.h, LT.mi); text(tb, cx, y0 + 100, 64, UIA, 1);
            if (btn(cx - 380, y0 + 200, 360, 80, "REAL TIME (recommended)", 20, UIA, true, setup.clock == 1)) setup.clock = 1;
            if (btn(cx + 20, y0 + 200, 360, 80, "FAST CLOCK (1 day = 36 min)", 20, UIA, true, setup.clock == 0)) setup.clock = 0;
            textWrap(setup.clock ? "Day and night follow your real day. Neighbours sleep when you do. The header shows today's real date and time." : "Time flies: a whole island day passes in about 36 minutes. The header shows the island's day count.", px + 80, y0 + 300, pw - 160, 20, C(200, 210, 235));
            textWrap("Wrong time? Fix it in Switch Settings > System > Date and Time.", px + 80, y0 + 380, pw - 160, 18, C(130, 140, 190));
            break; }
        case 3: {
            text("How cursed should the island be?", cx, y0, 24, C(200, 210, 235), 1);
            static const char* cl[3] = {"CALM", "NORMAL", "CURSED"}; static const char* cd[3] = {"Rare glitches. Mostly cosy.", "Odd events now and then.", "Lights flicker. Doors breathe. A lot."};
            for (int i = 0; i < 3; i++) { if (btn(px + 60 + i * 290, y0 + 40, 270, 70, cl[i], 28, UIA, true, setup.curse == i)) setup.curse = i; }
            text(cd[setup.curse], cx, y0 + 128, 22, WHITE, 1);
            text("Music", px + 120, y0 + 200, 24, WHITE); bar(px + 300, y0 + 204, 300, 22, setup.music * 100, UIA); if (btn(px + 630, y0 + 190, 60, 48, "-", 26, UIA)) setup.music = std::max(0.f, setup.music - 0.1f); if (btn(px + 700, y0 + 190, 60, 48, "+", 26, UIA)) setup.music = std::min(1.f, setup.music + 0.1f);
            text("Effects", px + 120, y0 + 280, 24, WHITE); bar(px + 300, y0 + 284, 300, 22, setup.sfxv * 100, UIA); if (btn(px + 630, y0 + 270, 60, 48, "-", 26, UIA)) setup.sfxv = std::max(0.f, setup.sfxv - 0.1f); if (btn(px + 700, y0 + 270, 60, 48, "+", 26, UIA)) setup.sfxv = std::min(1.f, setup.sfxv + 0.1f);
            musicVol = setup.music; sfxVol = setup.sfxv;
            text("You can change all of this later in Settings.", cx, y0 + 370, 18, C(130, 140, 190), 1);
            break; }
        case 4: {
            text("Pick a starter pack (you can buy the rest in the shop)", cx, y0, 22, C(200, 210, 235), 1);
            static const char* pn[3] = {"SNACK BOX", "WARDROBE", "GADGETS"}; static const char* pd[3] = {"Makroud, Brik, Mint Tea, Ice Cream, Couscous, Croissants", "Cap, Beanie, Crown, Glasses, Hoodie, Tee, Cargo Pants", "2 Phones, Football, Plush, Book, Balloons, Camera"};
            static const int pi_[3] = {0, 13, 22};
            for (int i = 0; i < 3; i++) {
                float x = px + 40 + i * 290, y = y0 + 40; bool sel = setup.pack == i; if (sel) fillRect(x, y, 270, 340, C(36, 80, 70)); else fillRect(x, y, 270, 340, C(30, 28, 70)); strokeRect(x, y, 270, 340, sel ? UIA : C(76, 78, 150), sel ? 4 : 2);
                itemIcon(pi_[i], x + 85, y + 20, 100); text(pn[i], x + 135, y + 140, 26, sel ? UIA : WHITE, 1); textWrap(pd[i], x + 16, y + 190, 238, 17, C(200, 210, 235));
                if (clickedIn(x, y, 270, 340)) { setup.pack = i; sfx(SFX_TAP); }
            }
            break; }
        case 5: {
            text("Bubbles above heads tell you what someone needs. Tap one!", cx, y0, 21, C(200, 210, 235), 1);
            struct BI { int type; int arg; const char* t; }; static const BI bi[] = {{B_WANT, 0, "Wants an item (the tag shows which)"}, {B_SAD, -1, "Sad: give food or a gift"}, {B_FRIEND, -1, "Wants to make a friend: you decide"}, {B_STUCK, -1, "Stuck: drag a friend onto them"}, {B_GAME, -1, "Wants to play a minigame"}, {B_DREAM, -1, "Dreaming: tap to read it"}};
            for (int i = 0; i < 6; i++) { float x = px + 50 + (i % 2) * 450, y = y0 + 80 + (i / 2) * 120; drawStatusBubble(x + 30, y + 52, bi[i].type, bi[i].arg, tAnim + i); textWrap(bi[i].t, x + 80, y + 14, 330, 20, WHITE); }
            break; }
        case 6: {
            text("Touch controls", px + 60, y0 + 6, 26, UIA);
            textWrap("Tap a person to say hi. Hold and drag to pick them up. Drop them on a room tile to move them, or on another person to introduce them.", px + 60, y0 + 44, pw - 120, 20, WHITE);
            textWrap("In the BAG tab, drag an item onto someone to give it. Drag goods onto the floor to leave them in the room.", px + 60, y0 + 140, pw - 120, 20, WHITE);
            text("Gamepad", px + 60, y0 + 232, 26, UIA);
            textWrap("Left stick moves the pointer, A taps and holds. B goes back, L / R switch rooms, + opens Settings.", px + 60, y0 + 270, pw - 120, 20, WHITE);
            text("Tip: Dalia's room has a door next to the bed. Select someone first.", px + 60, y0 + 380, 18, C(130, 140, 190));
            break; }
        default: {
            text(setup.name.empty() ? "Player" : setup.name.c_str(), cx, y0 + 10, 56, UIA, 1);
            textf(cx, y0 + 96, 24, WHITE, 1, "%s clock  |  %s island", setup.clock ? "Real-time" : "Fast", setup.curse == 0 ? "calm" : setup.curse == 1 ? "normal" : "cursed");
            static const char* pn2[3] = {"Snack Box", "Wardrobe", "Gadgets"}; textf(cx, y0 + 134, 22, C(200, 210, 235), 1, "Starter pack: %s", pn2[setup.pack]);
            text("Everyone is waiting in the lobby.", cx, y0 + 200, 26, WHITE, 1); text("Good luck.", cx, y0 + 244, 26, UIA, 1);
            nextLabel = "BEGIN!"; break; }
    }
    float by = py + ph - 70;
    if (setup.step > 0 && btn(px + 30, by, 190, 52, "< BACK", 22, C(148, 163, 184))) setup.step--;
    else if (setup.step == 0 && btn(px + 30, by, 190, 52, "CANCEL", 22, C(148, 163, 184))) mode = M_TITLE;
    if (btn(px + pw - 260, by, 230, 52, nextLabel, 24, UIA, canNext, true)) { if (setup.step == SETUP_STEPS - 1) finishSetup(); else setup.step++; }
}

// ---------------------------------------------------------------- title
static void drawTitle() {
    float glitch = (std::fmod(tAnim, 6.f) < 0.18f) ? (float)(rnd() % 14) - 7 : 0;
    fillRect(0, 0, SCR_W, SCR_H, C(0, 0, 0, 90));
    float cx = SCR_W / 2.f;
    fillRect(cx - 420, 110, 840, 220, C(16, 14, 40, 230)); strokeRect(cx - 420, 110, 840, 220, UIA, 4);
    text("ÎLOT14", cx + glitch - 3, 128, 96, C(168, 85, 247, 160), 1); text("ÎLOT14", cx + 3, 130, 96, C(34, 211, 238, 130), 1); text("ÎLOT14", cx, 128, 96, UIA, 1);
    text("I S L A N D", cx + glitch, 238, 54, WHITE, 1);
    text("a cursed island life sim", cx, 298, 22, C(148, 163, 184), 1);
    bool has = saveExists;
    if (has) {
        if (btn(cx - 260, 430, 240, 64, "CONTINUE", 28, UIA, true, true)) { if (loadGame()) { mode = M_MAP; } else say("Couldn't read the save."); }
        if (btn(cx + 20, 430, 240, 64, "NEW GAME", 28)) startSetup();
    } else if (btn(cx - 150, 430, 300, 64, "START", 30, UIA, true, true)) startSetup();
    if ((int)(tAnim * 2) % 2 == 0) text("TOUCH THE SCREEN", cx, 540, 24, UIA, 1);
    text(VERSION_STR, 14, SCR_H - 30, 18, C(148, 163, 184)); text("Îlot14 universe  -  not affiliated with any other life sim", SCR_W - 14, SCR_H - 30, 16, C(120, 130, 180), 2);
}

// ---------------------------------------------------------------- input plumbing
#ifdef __SWITCH__
static PadState pad;
#endif
static SDL_FingerID activeFinger = -1; static bool touchActive = false, mouseDown = false; static float tpx = 0, tpy = 0;
static void gatherInput(float dt) {
    in.pressed = in.released = false; in.hit = 0; Uint32 prevHeld = in.held; in.held = 0; in.lx = in.ly = 0;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT: running = false; break;
            case SDL_TEXTINPUT: if (mode == M_SETUP && setup.step == 1) { for (const char* p = e.text.text; *p; p++) if ((isalnum((unsigned char)*p) || *p == ' ') && setup.name.size() < 12) { setup.name += (char)(setup.name.empty() ? toupper(*p) : *p); setup.shift = false; } } break;
            case SDL_KEYDOWN: if (mode == M_SETUP && setup.step == 1 && e.key.keysym.sym == SDLK_BACKSPACE && !setup.name.empty()) setup.name.pop_back(); break;
            case SDL_FINGERDOWN: if (!touchActive) { touchActive = true; activeFinger = e.tfinger.fingerId; tpx = e.tfinger.x * SCR_W; tpy = e.tfinger.y * SCR_H; in.px = in.sx = tpx; in.py = in.sy = tpy; in.pressed = true; in.padMode = false; in.lastTouch = tAnim; } break;
            case SDL_FINGERMOTION: if (touchActive && e.tfinger.fingerId == activeFinger) { tpx = e.tfinger.x * SCR_W; tpy = e.tfinger.y * SCR_H; in.px = tpx; in.py = tpy; } break;
            case SDL_FINGERUP: if (touchActive && e.tfinger.fingerId == activeFinger) { touchActive = false; in.px = e.tfinger.x * SCR_W; in.py = e.tfinger.y * SCR_H; in.released = true; } break;
            case SDL_MOUSEBUTTONDOWN: if (e.button.button == SDL_BUTTON_LEFT && e.button.which != SDL_TOUCH_MOUSEID) { mouseDown = true; in.px = in.sx = (float)e.button.x; in.py = in.sy = (float)e.button.y; in.pressed = true; in.padMode = false; } break;
            case SDL_MOUSEMOTION: if (e.motion.which != SDL_TOUCH_MOUSEID) { in.px = (float)e.motion.x; in.py = (float)e.motion.y; } break;
            case SDL_MOUSEBUTTONUP: if (e.button.button == SDL_BUTTON_LEFT && e.button.which != SDL_TOUCH_MOUSEID && mouseDown) { mouseDown = false; in.px = (float)e.button.x; in.py = (float)e.button.y; in.released = true; } break;
            default: break;
        }
    }
    Uint32 h = 0;
#ifdef __SWITCH__
    padUpdate(&pad); u64 k = padGetButtons(&pad);
    if (k & HidNpadButton_A) h |= BTN_A; if (k & HidNpadButton_B) h |= BTN_B; if (k & HidNpadButton_X) h |= BTN_X; if (k & HidNpadButton_Y) h |= BTN_Y;
    if (k & HidNpadButton_L) h |= BTN_L; if (k & HidNpadButton_R) h |= BTN_R; if (k & HidNpadButton_Plus) h |= BTN_PLUS; if (k & HidNpadButton_Minus) h |= BTN_MINUS;
    if (k & HidNpadButton_Left) h |= BTN_LEFT; if (k & HidNpadButton_Right) h |= BTN_RIGHT; if (k & HidNpadButton_Up) h |= BTN_UP; if (k & HidNpadButton_Down) h |= BTN_DOWN;
    HidAnalogStickState ls = padGetStickPos(&pad, 0); in.lx = ls.x / 32767.f; in.ly = -ls.y / 32767.f;
#else
    const Uint8* ks = SDL_GetKeyboardState(nullptr);
    if (ks[SDL_SCANCODE_Z]) h |= BTN_A; if (ks[SDL_SCANCODE_X]) h |= BTN_B; if (ks[SDL_SCANCODE_LEFT]) h |= BTN_LEFT; if (ks[SDL_SCANCODE_RIGHT]) h |= BTN_RIGHT; if (ks[SDL_SCANCODE_UP]) h |= BTN_UP; if (ks[SDL_SCANCODE_DOWN]) h |= BTN_DOWN;
#endif
    in.held = h; in.hit = h & ~prevHeld;
    if (std::fabs(in.lx) < 0.2f) in.lx = 0; if (std::fabs(in.ly) < 0.2f) in.ly = 0;
    // gamepad -> virtual pointer
    if (in.lx != 0 || in.ly != 0) { in.padMode = true; in.cx = std::max(0.f, std::min((float)SCR_W - 1, in.cx + in.lx * 650 * dt)); in.cy = std::max(0.f, std::min((float)SCR_H - 1, in.cy + in.ly * 650 * dt)); }
    if (in.padMode && !touchActive && !mouseDown) {
        in.px = in.cx; in.py = in.cy;
        if (in.hit & BTN_A) { in.pressed = true; in.sx = in.px; in.sy = in.py; }
        if ((prevHeld & BTN_A) && !(h & BTN_A)) in.released = true;
    }
    bool padHold = in.padMode && (h & BTN_A);
    if (in.pressed) in.down = true; if (in.released) in.down = false;
    in.down = in.down || touchActive || mouseDown || padHold; if (in.released) in.down = false;
    if (in.padMode && mode == M_MAP && (in.hit & BTN_Y)) photoReq = true;
    if (in.hit & BTN_PLUS && mode != M_GAME && mode != M_TITLE && mode != M_SETUP) mode = M_SETTINGS;
    if (in.hit & BTN_B && !dlg.open && mode != M_GAME && mode != M_TITLE && mode != M_MAP && mode != M_SETUP) mode = M_MAP;
    if (mode == M_ROOM && !dlg.open) { if (in.hit & BTN_L) G.viewRoom = (G.viewRoom + NROOMS - 1) % NROOMS; if (in.hit & BTN_R) G.viewRoom = (G.viewRoom + 1) % NROOMS; }
    // swipe to rotate the island on the map screens
    static float lastX = 0; static bool rot = false;
    if (in.pressed) { rot = (mode == M_MAP || mode == M_SHOP || mode == M_PEOPLE || mode == M_PROFILE || mode == M_PHONE || mode == M_SETTINGS) && in.px < PANEL_X; lastX = in.px; }
    if (rot && in.down) { camYaw -= (in.px - lastX) * 0.006f; lastX = in.px; } if (in.released) rot = false;
}

// ---------------------------------------------------------------- main
#ifndef __SWITCH__
static void shot(const char* name) { SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, SCR_W, SCR_H, 32, SDL_PIXELFORMAT_ARGB8888); SDL_RenderReadPixels(R, nullptr, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch); SDL_SaveBMP(s, name); SDL_FreeSurface(s); }
#endif
int main(int argc, char** argv) {
    (void)argc; (void)argv;
#ifdef __SWITCH__
    padConfigureInput(1, HidNpadStyleSet_NpadStandard); padInitializeDefault(&pad);
#endif
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0"); SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) { SDL_Log("SDL init failed: %s", SDL_GetError()); return 1; }
    SDL_Window* win = SDL_CreateWindow("Ilot14 Island", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_W, SCR_H, SDL_WINDOW_SHOWN);
    if (!win) { SDL_Log("window failed: %s", SDL_GetError()); return 1; }
    R = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); if (!R) R = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!R) { SDL_Log("renderer failed: %s", SDL_GetError()); return 1; }
    SDL_SetRenderDrawBlendMode(R, SDL_BLENDMODE_BLEND); SDL_RenderSetLogicalSize(R, SCR_W, SCR_H);
    initFont(); ensureDirs(); initAudio(SAVE_DIR);
    rngS ^= (uint32_t)time(nullptr) * 2654435761u; initNewGame();
    { FILE* f = fopen((std::string(SAVE_DIR) + "save.dat").c_str(), "rb"); if (f) { saveExists = true; fclose(f); } }
#ifndef __SWITCH__
    bool tour = argc > 2 && std::string(argv[1]) == "--tour"; int tourFrame = 0; std::string tourDir = tour ? argv[2] : "";
#endif
    Uint32 last = SDL_GetTicks();
    while (running
#ifdef __SWITCH__
           && appletMainLoop()
#endif
    ) {
        Uint32 now = SDL_GetTicks(); float dt = std::min(0.05f, (now - last) / 1000.f); last = now; tAnim += dt;
        gatherInput(dt);
#ifndef __SWITCH__
        if (tour) {   // scripted walkthrough for headless testing
            tourFrame++;
            auto at = [&](int f) { return tourFrame == f; };
            if (at(5)) { shot((tourDir + "/01_title.bmp").c_str()); startSetup(); setup.step = 1; setup.name = "Amir"; }
            if (at(7)) { shot((tourDir + "/00a_setup_name.bmp").c_str()); setup.step = 2; }
            if (at(8)) { shot((tourDir + "/00b_setup_time.bmp").c_str()); setup.step = 4; }
            if (at(9)) { shot((tourDir + "/00c_setup_pack.bmp").c_str()); setup.step = 5; }
            if (at(10)) { shot((tourDir + "/00d_setup_bubbles.bmp").c_str()); setup.step = 7; }
            if (at(11)) { shot((tourDir + "/00e_setup_ready.bmp").c_str()); finishSetup(); }
            if (at(12)) { shot((tourDir + "/02_map.bmp").c_str()); G.minute = 21 * 60 + 30; }
            if (at(20)) { shot((tourDir + "/03_map_night.bmp").c_str()); G.minute = 14 * 60; for (int i = 0; i < 6; i++) setRoom(i, ROOM_LOBBY, false); G.bag[IT_PHONE] += 1; G.ch[3].bubble = B_WANT; G.ch[3].bArg = 6; G.ch[1].bubble = B_SAD; G.ch[4].bubble = B_GAME; G.ch[2].bubble = B_FRIEND; G.ch[2].bArg = 0; G.ch[0].bubble = B_STUCK; G.ch[0].bArg = 1; G.ch[5].bubble = B_CRUSH; goRoom(ROOM_LOBBY); G.selected = 3; speak(1, "I'm a bit concerned about Fares's snacks.", 30); speak(4, "Listen! I think the building is listening.", 30, true); G.ch[3].outfit[SL_HAT] = 13; G.ch[3].outfit[SL_GLASSES] = 14; }
            if (at(40)) { shot((tourDir + "/04_lobby.bmp").c_str()); tab = 1; }
            if (at(45)) { shot((tourDir + "/05_bag.bmp").c_str()); tab = 2; G.viewRoom = I_DALIA; setRoom(8, 8, false); G.ch[8].state = S_IDLE; }
            if (at(55)) { shot((tourDir + "/06_dalia_room.bmp").c_str()); G.minute = 23 * 60; for (int i = 0; i < NCHAR; i++) { setRoom(i, i, false); } for (int m = 0; m < 12; m++) tickMinute(); tab = 0; G.viewRoom = 3; }
            if (at(70)) { shot((tourDir + "/07_sleep.bmp").c_str()); G.minute = 12 * 60; G.viewRoom = ROOM_PARK; setRoom(24, ROOM_PARK, false); setRoom(17, ROOM_PARK, false); setRoom(0, ROOM_PARK, false); }
            if (at(85)) { shot((tourDir + "/08_parking.bmp").c_str()); G.viewRoom = ROOM_ICE; setRoom(24, ROOM_ICE, false); setRoom(0, ROOM_ICE, false); }
            if (at(100)) { shot((tourDir + "/09_ice.bmp").c_str()); mode = M_SHOP; }
            if (at(105)) { shot((tourDir + "/10_shop.bmp").c_str()); mode = M_PEOPLE; }
            if (at(110)) { shot((tourDir + "/11_people.bmp").c_str()); mode = M_PROFILE; profileIdx = 0; }
            if (at(115)) { shot((tourDir + "/12_profile.bmp").c_str()); mode = M_PHONE; G.texts.push_front("Dalia -> Amyas: the lights did it again"); }
            if (at(120)) { shot((tourDir + "/13_phone.bmp").c_str()); mode = M_SETTINGS; }
            if (at(125)) { shot((tourDir + "/14_settings.bmp").c_str()); mode = M_GAME; mgStart(MG_CATCH, 3); mg.phase = 1; mg.fall.push_back({400, 300, 0, 0}); mg.fall.push_back({700, 200, 0, 1}); mg.score = 80; }
            if (at(135)) { shot((tourDir + "/15_catch.bmp").c_str()); mgStart(MG_MEMORY, 3); mg.phase = 1; mg.cards[0].up = true; mg.cards[5].done = true; }
            if (at(145)) { shot((tourDir + "/16_memory.bmp").c_str()); mgStart(MG_TIMING, 3); mg.phase = 1; }
            if (at(155)) { shot((tourDir + "/17_timing.bmp").c_str()); mg.tier = 2; mg.phase = 3; mgMakePrizes(); mg.chosen = 1; mg.resultLine = "You got 120 DA!"; mg.revealT = 0; }
            if (at(165)) { shot((tourDir + "/18_prize.bmp").c_str()); mg.active = false; mode = M_MAP; saveGame(); GameState backup = G; if (!loadGame()) fprintf(stderr, "LOAD FAILED\n"); else fprintf(stderr, "save/load ok day=%d money=%d\n", G.day, G.money); (void)backup; }
            if (at(175)) { running = false; }
        }
#endif
        if (mode == M_GAME || mg.active) { mode = M_GAME; mgUpdate(dt); if (!mg.active) mode = M_ROOM; }
        else { updateWorld(dt); }
        if (mode == M_ROOM) roomInput(dt);
        // ---- draw
        setc(BLACK0); SDL_RenderClear(R);
        if (mode == M_GAME) { mgDraw(tAnim); }
        else {
            renderViewport();
            if (mode != M_TITLE && mode != M_SETUP) { setAccent(); panelBG(); switch (mode) { case M_MAP: drawMapScreen(); break; case M_SHOP: drawShop(); break; case M_PEOPLE: drawPeople(); break; case M_PROFILE: drawProfile(); break; case M_PHONE: drawPhone(); break; case M_SETTINGS: drawSettings(); break; case M_ROOM: drawRoomPanel(); break; default: break; } }
            else if (mode == M_SETUP) drawSetup(); else { UIA = C(249, 115, 22); drawTitle(); }
            // dragged item ghost / held feedback
            if (drag.kind == 2 && drag.grabbed) itemIcon(drag.id, in.px - 36, in.py - 36, 72);
        }
        if (toastT > 0) { toastT -= dt; float w = textW(toast.c_str(), 20) + 28; fillRect(PANEL_X / 2 - w / 2, 640, w, 40, C(16, 14, 40, 235)); strokeRect(PANEL_X / 2 - w / 2, 640, w, 40, UIA, 2); text(toast.c_str(), PANEL_X / 2, 648, 20, WHITE, 1); }
        if (photoFlash > 0) photoFlash -= dt;
        scanlines(28);
        if (in.padMode && tAnim - in.lastTouch > 0.5f && !touchActive) { lineD(in.cx - 12, in.cy, in.cx + 12, in.cy, UIA, 3); lineD(in.cx, in.cy - 12, in.cx, in.cy + 12, UIA, 3); strokeRect(in.cx - 6, in.cy - 6, 12, 12, WHITE, 1); }
        SDL_RenderPresent(R);
    }
    if (mode != M_TITLE) saveGame();
    if (audDev) SDL_CloseAudioDevice(audDev);
    SDL_DestroyRenderer(R); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
