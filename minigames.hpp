// minigames.hpp - three original minigames + the prize pick
#pragma once
#include "ui.hpp"

enum { MG_CATCH, MG_MEMORY, MG_TIMING, MG_COUNT };
static const char* MG_NAME[MG_COUNT] = {"ASH CATCH", "PAIR UP", "ELEVATOR STOP"};
static const char* MG_HELP[MG_COUNT] = {"Slide to catch the orange ash. Dodge the purple glitches!", "Flip cards and find all the pairs before time runs out.", "Stop the marker inside the zone. The zone shrinks every round."};

struct Falling { float x, y, vy; int kind; };
struct Card { int sym; bool up, done; };
struct Prize { int kind, val; };   // kind 0 = money, 1 = item
struct MG {
    bool active = false; int type = 0, asker = 0, phase = 0;   // phase 0 intro, 1 play, 2 result, 3 prize, 4 outro
    float t = 0, timeLeft = 30; int score = 0, tier = 0;
    // catch
    float bx = 640; std::vector<Falling> fall; float spawn = 0; float flash = 0;
    // memory
    Card cards[16]; int first = -1, second = -1; float flipT = 0; int pairs = 0;
    // timing
    float mark = 0, dir = 1, zone = 140, zoneY = 200; int round = 0, hits = 0; float speed = 520; float feedback = 0; bool lastHit = false;
    // prize
    Prize prizes[3]; int chosen = -1; float revealT = 0; std::string resultLine;
};
static MG mg;

static void mgStart(int type, int asker) {
    mg = MG(); mg.active = true; mg.type = type; mg.asker = asker; mg.phase = 0;
    curTrack = TR_GAME;
    if (type == MG_CATCH) mg.timeLeft = 30;
    if (type == MG_MEMORY) { mg.timeLeft = 60; int syms[16]; for (int i = 0; i < 16; i++) syms[i] = i / 2; for (int i = 15; i > 0; i--) std::swap(syms[i], syms[ri(i + 1)]); for (int i = 0; i < 16; i++) mg.cards[i] = {syms[i], false, false}; }
    if (type == MG_TIMING) { mg.zone = 150; mg.zoneY = rr(120, 480); mg.mark = 60; mg.dir = 1; mg.speed = 480; }
}
static void mgFinish(int score) {
    mg.score = score; mg.phase = 2; mg.t = 0;
    if (mg.type == MG_CATCH) mg.tier = score >= 220 ? 3 : score >= 140 ? 2 : score >= 60 ? 1 : 0;
    else if (mg.type == MG_MEMORY) mg.tier = mg.pairs >= 8 ? 3 : mg.pairs >= 6 ? 2 : mg.pairs >= 3 ? 1 : 0;
    else mg.tier = mg.hits >= 5 ? 3 : mg.hits >= 4 ? 2 : mg.hits >= 2 ? 1 : 0;
    sfx(mg.tier ? SFX_GOOD : SFX_BAD);
}
static void mgMakePrizes() {
    for (int i = 0; i < 3; i++) {
        int r = ri(100);
        if (mg.tier >= 3 && r < 35) mg.prizes[i] = {1, IT_MYSTERY};
        else if (mg.tier >= 2 && r < 30) mg.prizes[i] = {1, ri(11)};
        else if (mg.tier >= 3 && r < 45) mg.prizes[i] = {1, 11 + ri(11)};
        else mg.prizes[i] = {0, (mg.tier == 1 ? 60 : mg.tier == 2 ? 120 : 220) + ri(5) * 20};
    }
}
static void mgUpdate(float dt) {
    mg.t += dt;
    if (mg.phase == 0) { if (mg.t > 0.3f && (in.released || (in.hit & BTN_A))) { mg.phase = 1; mg.t = 0; sfx(SFX_BELL); } return; }
    if (mg.phase == 1) {
        if (mg.type == MG_CATCH) {
            mg.timeLeft -= dt; if (in.down && in.px > 0) mg.bx += (in.px - mg.bx) * std::min(1.f, dt * 14.f);
            mg.bx += in.lx * 900.f * dt; if (in.held & BTN_LEFT) mg.bx -= 800 * dt; if (in.held & BTN_RIGHT) mg.bx += 800 * dt; mg.bx = std::max(70.f, std::min(1210.f, mg.bx));
            mg.spawn -= dt; float rate = 0.55f - std::min(0.3f, (30 - mg.timeLeft) * 0.01f);
            if (mg.spawn <= 0) { mg.spawn = rate; int k = rf() < 0.22f ? 1 : (rf() < 0.07f ? 2 : 0); mg.fall.push_back({rr(60, 1220), -30, rr(240, 330) + (30 - mg.timeLeft) * 6.f, k}); }
            for (auto& f : mg.fall) f.y += f.vy * dt;
            for (auto& f : mg.fall) if (f.y > 590 && f.y < 650 && std::fabs(f.x - mg.bx) < 70) { f.y = 9999; if (f.kind == 0) { mg.score += 10; sfx(SFX_COIN); } else if (f.kind == 2) { mg.score += 30; sfx(SFX_GOOD); } else { mg.score = std::max(0, mg.score - 15); mg.flash = 0.5f; sfx(SFX_GLITCH); } }
            mg.fall.erase(std::remove_if(mg.fall.begin(), mg.fall.end(), [](const Falling& f) { return f.y > 760; }), mg.fall.end());
            mg.flash = std::max(0.f, mg.flash - dt);
            if (mg.timeLeft <= 0) mgFinish(mg.score);
        } else if (mg.type == MG_MEMORY) {
            mg.timeLeft -= dt;
            if (mg.second >= 0) { mg.flipT -= dt; if (mg.flipT <= 0) { Card &a = mg.cards[mg.first], &b = mg.cards[mg.second]; if (a.sym == b.sym) { a.done = b.done = true; mg.pairs++; sfx(SFX_GOOD); } a.up = b.up = false; mg.first = mg.second = -1; } }
            else if (in.released) for (int i = 0; i < 16; i++) {
                float cx = 360 + (i % 4) * 140, cy = 130 + (i / 4) * 140;
                if (inside(cx, cy, 124, 124, in.px, in.py) && !mg.cards[i].up && !mg.cards[i].done) { mg.cards[i].up = true; sfx(SFX_TAP); if (mg.first < 0) mg.first = i; else { mg.second = i; mg.flipT = 0.7f; } break; }
            }
            if (mg.pairs >= 8 || mg.timeLeft <= 0) mgFinish(mg.pairs * 30 + (int)mg.timeLeft);
        } else {
            mg.mark += mg.dir * mg.speed * dt; if (mg.mark > 640) { mg.mark = 640; mg.dir = -1; } if (mg.mark < 60) { mg.mark = 60; mg.dir = 1; }
            mg.feedback = std::max(0.f, mg.feedback - dt);
            if (mg.feedback <= 0 && mg.t > 0.4f && (in.pressed || (in.hit & BTN_A))) {
                bool hit = mg.mark >= mg.zoneY && mg.mark <= mg.zoneY + mg.zone; mg.lastHit = hit; mg.feedback = 0.7f; if (hit) { mg.hits++; sfx(SFX_GOOD); } else sfx(SFX_BAD);
                mg.round++; mg.speed += 90; mg.zone = std::max(54.f, mg.zone - 22); mg.zoneY = rr(100, 640 - mg.zone); if (mg.round >= 5) mgFinish(mg.hits);
            }
        }
        return;
    }
    if (mg.phase == 2) { if (mg.t > 0.6f && (in.released || (in.hit & BTN_A))) { if (mg.tier > 0) { mg.phase = 3; mgMakePrizes(); mg.chosen = -1; mg.t = 0; } else { mg.phase = 4; mg.t = 0; } } return; }
    if (mg.phase == 3) {
        if (mg.chosen < 0 && in.released) for (int i = 0; i < 3; i++) if (inside(310 + i * 230, 250, 200, 200, in.px, in.py)) { mg.chosen = i; mg.revealT = 0; sfx(SFX_BELL); Prize p = mg.prizes[i]; if (p.kind == 0) { G.money += p.val; mg.resultLine = "You got " + fmtMoney(p.val) + "!"; } else { G.bag[p.val]++; mg.resultLine = std::string("You got: ") + ITEMS[p.val].name + "!"; } }
        if (mg.chosen >= 0) { mg.revealT += dt; if (mg.revealT > 0.8f && (in.released || (in.hit & BTN_A))) { mg.phase = 4; mg.t = 0; } }
        return;
    }
    if (mg.phase == 4) {
        mg.active = false; curTrack = isNight() ? TR_NIGHT : TR_DAY;
        Char& c = G.ch[mg.asker]; addMood(mg.asker, 8 + mg.tier * 8); addBond(mg.asker, 3 + mg.tier * 2); G.money += mg.tier * 30; c.bubble = B_NONE; c.bCool = 90;
        speak(mg.asker, voiced(mg.asker, mg.tier ? pick(L_WIN) : pick(L_LOSE)), 4.f);
        notice(std::string("Played ") + MG_NAME[mg.type] + " with " + nm(mg.asker) + ". Score tier " + std::to_string(mg.tier) + "/3.");
    }
}
static void mgDraw(float tAnim) {
    static const Col mc[3] = {{34, 211, 238, 255}, {192, 132, 252, 255}, {74, 222, 128, 255}}; UIA = mc[mg.type];
    vgrad(0, 0, SCR_W, SCR_H, C(18, 14, 52), C(10, 44, 78));
    for (int y = 0; y < SCR_H; y += 40) fillRect(0, y, SCR_W, 1, C(40, 44, 100));
    for (int x = 0; x < SCR_W; x += 40) fillRect(x, 0, 1, SCR_H, C(36, 34, 78));
    text(MG_NAME[mg.type], 640, 22, 34, UIA, 1);
    textf(40, 24, 22, WHITE, 0, "with %s", nm(mg.asker));
    if (mg.phase == 0) {
        fillRect(240, 200, 800, 300, C(30, 28, 70)); strokeRect(240, 200, 800, 300, UIA, 3);
        textWrap(MG_HELP[mg.type], 280, 250, 720, 28, WHITE); text("TAP / PRESS A TO START", 640, 420, 28, UIA, 1);
        return;
    }
    if (mg.phase == 1 || mg.phase == 2) {
        if (mg.type == MG_CATCH) {
            textf(1240, 24, 26, C(250, 204, 21), 2, "SCORE %d", mg.score); textf(640, 66, 22, WHITE, 1, "%0.0f s", std::max(0.f, mg.timeLeft));
            for (auto& f : mg.fall) {
                if (f.kind == 0) { tri2(f.x, f.y - 18, f.x + 14, f.y, f.x, f.y + 18, UIA); tri2(f.x, f.y - 18, f.x - 14, f.y, f.x, f.y + 18, mulc(UIA, 0.8f)); }
                else if (f.kind == 2) { disc(f.x, f.y, 18, 18, C(250, 204, 21), 8); text("*", f.x, f.y - 12, 26, BLACK0, 1); }
                else { fillRect(f.x - 14 + (rnd() % 5) - 2, f.y - 14, 28, 28, C(168, 85, 247)); fillRect(f.x - 8, f.y - 2, 16, 4, BLACK0); }
            }
            fillRect(mg.bx - 70, 620, 140, 22, UIA); fillRect(mg.bx - 60, 600, 8, 20, UIA); fillRect(mg.bx + 52, 600, 8, 20, UIA);
            if (mg.flash > 0) fillRect(0, 0, SCR_W, SCR_H, C(168, 85, 247, (int)(mg.flash * 160)));
        } else if (mg.type == MG_MEMORY) {
            textf(1240, 24, 26, C(250, 204, 21), 2, "PAIRS %d/8", mg.pairs); textf(640, 66, 22, WHITE, 1, "%0.0f s", std::max(0.f, mg.timeLeft));
            for (int i = 0; i < 16; i++) {
                float cx = 360 + (i % 4) * 140, cy = 130 + (i / 4) * 140; Card& c = mg.cards[i];
                if (c.done) { strokeRect(cx, cy, 124, 124, C(60, 62, 110), 2); continue; }
                fillRect(cx, cy, 124, 124, c.up ? C(36, 34, 78) : C(64, 60, 140)); strokeRect(cx, cy, 124, 124, UIA, 3);
                if (c.up) { int item = (c.sym * 3) % 11 + (c.sym > 5 ? 11 : 0); itemIcon(item, cx + 22, cy + 22, 80); } else text("14", cx + 62, cy + 40, 44, UIA, 1);
            }
        } else {
            textf(1240, 24, 26, C(250, 204, 21), 2, "ROUND %d/5   HITS %d", std::min(5, mg.round + 1), mg.hits);
            fillRect(560, 60, 160, 600, C(30, 28, 70)); strokeRect(560, 60, 160, 600, UIA, 3);
            fillRect(560, mg.zoneY, 160, mg.zone, C(34, 120, 70)); fillRect(550, mg.mark - 5, 180, 10, mg.feedback > 0 ? (mg.lastHit ? C(74, 222, 128) : C(239, 68, 68)) : WHITE);
            if (mg.feedback > 0) text(mg.lastHit ? "NICE!" : "MISS", 900, 330, 44, mg.lastHit ? C(74, 222, 128) : C(239, 68, 68), 1);
        }
        if (mg.phase == 2) {
            fillRect(340, 230, 600, 240, C(16, 14, 40, 235)); strokeRect(340, 230, 600, 240, UIA, 3);
            text(mg.tier ? "NICE ONE!" : "TOO BAD...", 640, 260, 40, mg.tier ? UIA : C(148, 163, 184), 1);
            for (int i = 0; i < 3; i++) { Col sc = i < mg.tier ? C(250, 204, 21) : C(60, 62, 110); tri2(500 + i * 140, 380, 560 + i * 140, 380, 530 + i * 140, 330, sc); tri2(500 + i * 140, 350, 560 + i * 140, 350, 530 + i * 140, 400, sc); }
            text("TAP TO CONTINUE", 640, 430, 20, WHITE, 1);
        }
        return;
    }
    if (mg.phase == 3) {
        text("PICK A PRIZE!", 640, 130, 40, UIA, 1);
        for (int i = 0; i < 3; i++) {
            float x = 310 + i * 230, y = 250; bool sel = mg.chosen == i, open = mg.chosen >= 0;
            fillRect(x, y, 200, 200, sel ? C(48, 46, 110) : C(30, 28, 70)); strokeRect(x, y, 200, 200, UIA, 3);
            if (!open) { text("?", x + 100, y + 60, 90, UIA, 1); }
            else if (sel) { Prize p = mg.prizes[i]; if (p.kind == 0) { text(fmtMoney(p.val).c_str(), x + 100, y + 80, 30, C(250, 204, 21), 1); } else { itemIcon(p.val, x + 50, y + 40, 100); text(ITEMS[p.val].name, x + 100, y + 160, 18, WHITE, 1); } }
            else { Prize p = mg.prizes[i]; text(p.kind == 0 ? "DA" : "item", x + 100, y + 80, 28, C(90, 80, 70), 1); }
        }
        if (mg.chosen >= 0) { text(mg.resultLine.c_str(), 640, 500, 30, WHITE, 1); text("TAP TO CONTINUE", 640, 560, 20, UIA, 1); }
    }
}
