// world.hpp - simulation state, rules, and save/load
#pragma once
#include "gfx.hpp"
#ifdef __SWITCH__
#include <switch.h>
#endif
#include "data.hpp"
#include "audio.hpp"
#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <deque>
#include <functional>

#ifdef __SWITCH__
#define SAVE_DIR "sdmc:/switch/ilot14_island/"
#else
#define SAVE_DIR "./ilot14_island_data/"
#endif

// ---- tiny rng
static uint32_t rngS = 2463534242u;
static inline uint32_t rnd() { rngS ^= rngS << 13; rngS ^= rngS >> 17; rngS ^= rngS << 5; return rngS; }
static inline float rf() { return (rnd() & 0xFFFFFF) / 16777216.f; }
static inline int ri(int n) { return n <= 0 ? 0 : (int)(rnd() % (uint32_t)n); }
static inline float rr(float a, float b) { return a + (b - a) * rf(); }
template <class T, size_t N> static inline const T& pick(const T (&a)[N]) { return a[ri((int)N)]; }
static std::string fmtName(const char* tmpl, const char* n) {
    std::string s = tmpl; size_t p = s.find("{n}"); if (p != std::string::npos) s.replace(p, 3, n); return s;
}

enum Bubble { B_NONE, B_WANT, B_SAD, B_FRIEND, B_STUCK, B_GAME, B_DREAM, B_CRUSH };
enum CState { S_IDLE, S_WALK, S_SLEEP, S_EAT, S_HELD, S_CHAT };

struct Char {
    // persistent
    int room = 0; float x = 0, z = 0;
    float mood = 60, hunger = 70, energy = 80, bond = 0;
    int outfit[4] = {-1, -1, -1, -1};     // hat, top, bottom, glasses (item idx)
    int held = -1; bool hasPhone = false;
    float aff[NCHAR]; uint8_t friends[NCHAR];
    // transient
    float tx = 0, tz = 0, face = 0, faceT = 0, anim = 0, stateT = 0, bounce = 0, glitch = 0, walkAmp = 0, sway = 0, lookYaw = 0;
    int state = S_IDLE;
    int bubble = B_NONE, bArg = -1, bCool = 0; int dream = -1;
    std::string say; float sayT = 0; bool thought = false;
    float botX = 0, botZ = 0;
};
struct Prop { int room, item; float x, z; };
struct Convo { bool on = false; int a = -1, b = -1, stage = 0; float t = 0; };

struct GameState {
    int day = 1, minute = 8 * 60;
    int money = 1450;
    int bag[NITEMS];
    Char ch[NCHAR];
    std::vector<Prop> props;
    std::deque<std::string> notices, texts;
    float musicV = 0.5f, sfxV = 0.7f; int realClock = 1; int glitchLevel = 1; char playerName[16] = "Player";
    // transient
    float clockAcc = 0; Convo convo[NROOMS]; float convoCd[NROOMS];
    int viewRoom = 0; int selected = -1;
    float curseFlash = 0; int lastDayPaid = 1;
};
static GameState G;

static const char* nm(int i) { return ROSTER[i].name; }
static int homeRoom(int i) { return i; }
static int hourOf() { return G.minute / 60; }
static bool isNight() { int h = hourOf(); return h >= 21 || h < 6; }
static bool sleepTime() { int h = hourOf(); return h >= 22 || h < 7; }
static float ageScale(int age) { float s = 0.42f + age * 0.04f; return std::max(0.58f, std::min(1.12f, s)); }
static bool isBeef(int a, int b) { return (a == I_AMIR && (b == I_AMYAS || b == I_ADAMB)) || (b == I_AMIR && (a == I_AMYAS || a == I_ADAMB)); }
static bool isHateCrush(int a, int b) { return (a == I_DALIA && b == I_GABRIEL) || (a == I_GABRIEL && b == I_DALIA); }
static bool isSiblings(int a, int b) {
    auto in = [&](int x, int y) { return (a == x && b == y) || (a == y && b == x); };
    return in(I_AMIR, I_ALICIA) || in(I_ADAMB, I_AMYAS) || in(I_ADAMB, I_DALIA) || in(I_AMYAS, I_DALIA) || in(I_JALIL, I_ARWA) || in(I_ANES, I_ABDULLAH);
}
static const char* tierName(int a, int b) {
    if (isHateCrush(a, b)) return "Oil & water";
    if (isBeef(a, b) && G.ch[a].aff[b] < 40) return "Beef";
    float v = G.ch[a].aff[b];
    return v < 15 ? "Strangers" : v < 35 ? "Acquaintances" : v < 60 ? "Friends" : "Best friends";
}
static Col tierCol(int a, int b) { const char* t = tierName(a, b); return t[0] == 'B' && t[1] == 'e' && t[2] == 'e' ? C(239, 68, 68) : t[0] == 'O' ? C(239, 68, 68) : G.ch[a].aff[b] < 15 ? C(148, 163, 184) : G.ch[a].aff[b] < 35 ? C(250, 204, 21) : G.ch[a].aff[b] < 60 ? C(74, 222, 128) : C(249, 115, 22); }
// ---- real date/time (device clock)
struct LTime { int y = 2026, mo = 10, d = 25, wd = 0, h = 12, mi = 0, ok = 0; };
static LTime LT;
static const char* WDAY[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char* MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static void refreshLocalTime() {
#ifdef __SWITCH__
    static bool inited = false; if (!inited) { timeInitialize(); inited = true; }
    u64 ts = 0;
    if (R_SUCCEEDED(timeGetCurrentTime(TimeType_LocalSystemClock, &ts))) {
        TimeCalendarTime cal; TimeCalendarAdditionalInfo inf;
        if (R_SUCCEEDED(timeToCalendarTimeWithMyRule(ts, &cal, &inf))) { LT.y = cal.year; LT.mo = cal.month; LT.d = cal.day; LT.h = cal.hour; LT.mi = cal.minute; LT.wd = inf.wday; LT.ok = 1; return; }
    }
#endif
    time_t tt = time(nullptr); struct tm* lt = localtime(&tt);
    if (lt) { LT.y = lt->tm_year + 1900; LT.mo = lt->tm_mon + 1; LT.d = lt->tm_mday; LT.wd = lt->tm_wday; LT.h = lt->tm_hour; LT.mi = lt->tm_min; LT.ok = 1; }
}
static std::string dateStr() { char b[48]; snprintf(b, sizeof b, "%s %d %s %d", WDAY[LT.wd % 7], LT.d, MONS[(LT.mo + 11) % 12], LT.y); return b; }
static std::string clockStr() { char b[16]; snprintf(b, sizeof b, "%02d:%02d", G.minute / 60, G.minute % 60); return b; }
static void notice(const std::string& s) { G.notices.push_front(s); while (G.notices.size() > 30) G.notices.pop_back(); }
static std::string voiced(int ci, const std::string& line) {
    int p = ROSTER[ci].pers; std::string r = line;
    if (p == P_TINY) return r;
    if (rf() < 0.5f) r = std::string(VOICE_PRE[p][ri(2)]) + r;
    if (rf() < 0.5f) r += VOICE_SUF[p][ri(2)];
    return r;
}
static void speak(int ci, const std::string& s, float t = 3.2f, bool thought = false) { Char& c = G.ch[ci]; c.say = s; c.sayT = t; c.thought = thought; }
static float spawnX(int room) { (void)room; return rr(-2.4f, 2.6f); }
static bool inFurniture(float x, float z) { return (x < -1.2f && z < -1.0f) || (x > 1.6f && z < -1.5f && x < 3.9f) || (x < -3.0f && z > 0.1f && z < 1.8f) || (x > 3.1f && z > 0.0f && z < 0.7f); }
static void randomSpot(int room, float& x, float& z) {
    for (int k = 0; k < 20; k++) { x = rr(-3.3f, 3.3f); z = rr(-2.2f, 2.4f); if (room >= NCHAR || !inFurniture(x, z)) return; }
    x = 0; z = 1.2f;
}
static void setRoom(int ci, int room, bool atDoor = true) {
    Char& c = G.ch[ci]; c.room = room; c.state = S_IDLE; c.bubble = (c.bubble == B_DREAM) ? B_NONE : c.bubble;
    if (atDoor && room == G.viewRoom) { c.x = 3.7f; c.z = rr(0.5f, 2.0f); randomSpot(room, c.tx, c.tz); c.state = S_WALK; }
    else { randomSpot(room, c.x, c.z); c.tx = c.x; c.tz = c.z; }
}

static void initNewGame() {
    G = GameState();
    for (int i = 0; i < NITEMS; i++) G.bag[i] = 0;
    G.bag[0] = 2; G.bag[6] = 2; G.bag[8] = 3; G.bag[24] = 1; G.bag[26] = 2; G.bag[11] = 1;
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i];
        c.mood = rr(45, 80); c.hunger = rr(45, 90); c.energy = rr(50, 100); c.bond = 0; c.room = i;
        for (int j = 0; j < NCHAR; j++) { c.aff[j] = i == j ? 0 : rr(2, 22); c.friends[j] = 0; }
        randomSpot(i, c.x, c.z); c.tx = c.x; c.tz = c.z;
    }
    for (int i = 0; i < NCHAR; i++) for (int j = 0; j < NCHAR; j++) {
        if (i == j) continue;
        if (isSiblings(i, j)) { G.ch[i].aff[j] = 55; G.ch[i].friends[j] = 1; }
        if ((i == I_AMIR && j == I_AMINE) || (i == I_AMINE && j == I_AMIR)) { G.ch[i].aff[j] = 72; G.ch[i].friends[j] = 1; }
        if (isHateCrush(i, j) && i == I_DALIA) G.ch[i].aff[j] = 0;
        if (isBeef(i, j)) G.ch[i].aff[j] = 8;
    }
    G.ch[I_AMIR].hasPhone = false;
    refreshLocalTime(); if (G.realClock) G.minute = LT.h * 60 + LT.mi;
    notice("Welcome to Îlot14 Island. Everyone moved in. Nobody remembers signing anything.");
    G.viewRoom = 0;
}

// ---------------------------------------------------------------- save / load (chunked so old saves survive updates)
static void ensureDirs() {
#ifdef __SWITCH__
    mkdir("sdmc:/switch", 0777); mkdir(SAVE_DIR, 0777); mkdir(SAVE_DIR "music", 0777); mkdir(SAVE_DIR "photos", 0777);
#else
    system("mkdir -p ./ilot14_island_data/music ./ilot14_island_data/photos");
#endif
}
struct PChar { int32_t room; float x, z, mood, hunger, energy, bond; int32_t outfit[4]; int32_t held; int32_t hasPhone; float aff[NCHAR]; uint8_t friends[NCHAR]; };
static void wChunk(FILE* f, const char* tag, const void* d, uint32_t n) { fwrite(tag, 1, 4, f); fwrite(&n, 4, 1, f); fwrite(d, 1, n, f); }
static bool saveGame() {
    ensureDirs();
    std::string tmp = std::string(SAVE_DIR) + "save.tmp", fin = std::string(SAVE_DIR) + "save.dat";
    FILE* f = fopen(tmp.c_str(), "wb"); if (!f) return false;
    fwrite("ILOT14IS", 1, 8, f);
    int32_t gen[8] = {G.day, G.minute, G.money, G.realClock, G.glitchLevel, G.lastDayPaid, 0, 0}; wChunk(f, "GENR", gen, sizeof gen);
    float vol[2] = {G.musicV, G.sfxV}; wChunk(f, "VOLS", vol, sizeof vol);
    { char pl[20]; memset(pl, 0, sizeof pl); memcpy(pl, G.playerName, 16); int32_t ver = 4; memcpy(pl + 16, &ver, 4); wChunk(f, "PLAY", pl, sizeof pl); }
    wChunk(f, "BAG_", G.bag, sizeof G.bag);
    std::vector<PChar> pc(NCHAR);
    for (int i = 0; i < NCHAR; i++) { Char& c = G.ch[i]; PChar& p = pc[i]; memset(&p, 0, sizeof p); p.room = c.room; p.x = c.x; p.z = c.z; p.mood = c.mood; p.hunger = c.hunger; p.energy = c.energy; p.bond = c.bond; for (int k = 0; k < 4; k++) p.outfit[k] = c.outfit[k]; p.held = c.held; p.hasPhone = c.hasPhone; memcpy(p.aff, c.aff, sizeof p.aff); memcpy(p.friends, c.friends, sizeof p.friends); }
    int32_t nc = NCHAR; std::vector<char> blob(4 + sizeof(PChar) * NCHAR); memcpy(blob.data(), &nc, 4); memcpy(blob.data() + 4, pc.data(), sizeof(PChar) * NCHAR);
    wChunk(f, "CHRS", blob.data(), (uint32_t)blob.size());
    wChunk(f, "PROP", G.props.data(), (uint32_t)(G.props.size() * sizeof(Prop)));
    fclose(f);
    remove(fin.c_str()); return rename(tmp.c_str(), fin.c_str()) == 0;
}
static bool loadGame() {
    std::string fin = std::string(SAVE_DIR) + "save.dat";
    FILE* f = fopen(fin.c_str(), "rb"); if (!f) return false;
    char mg[8]; if (fread(mg, 1, 8, f) != 8 || memcmp(mg, "ILOT14IS", 8)) { fclose(f); return false; }
    initNewGame(); G.notices.clear(); bool sawPlay = false;
    char tag[4]; uint32_t n;
    while (fread(tag, 1, 4, f) == 4 && fread(&n, 4, 1, f) == 1) {
        std::vector<char> d(n); if (n && fread(d.data(), 1, n, f) != n) break;
        if (!memcmp(tag, "GENR", 4) && n >= 24) { int32_t* g = (int32_t*)d.data(); G.day = g[0]; G.minute = g[1]; G.money = g[2]; G.realClock = g[3]; G.glitchLevel = g[4]; G.lastDayPaid = g[5]; }
        else if (!memcmp(tag, "VOLS", 4) && n >= 8) { float* v = (float*)d.data(); G.musicV = v[0]; G.sfxV = v[1]; }
        else if (!memcmp(tag, "PLAY", 4) && n >= 16) { memcpy(G.playerName, d.data(), 15); G.playerName[15] = 0; sawPlay = true; }
        else if (!memcmp(tag, "BAG_", 4)) { memcpy(G.bag, d.data(), std::min<size_t>(n, sizeof G.bag)); }
        else if (!memcmp(tag, "CHRS", 4) && n >= 4) {
            int32_t nc; memcpy(&nc, d.data(), 4);
            if (n >= 4 + sizeof(PChar) * (size_t)nc) {
                PChar* pc = (PChar*)(d.data() + 4);
                for (int i = 0; i < std::min<int>(nc, NCHAR); i++) { Char& c = G.ch[i]; PChar& p = pc[i]; c.room = std::max(0, std::min(NROOMS - 1, (int)p.room)); c.x = p.x; c.z = p.z; c.tx = p.x; c.tz = p.z; c.mood = p.mood; c.hunger = p.hunger; c.energy = p.energy; c.bond = p.bond; for (int k = 0; k < 4; k++) c.outfit[k] = p.outfit[k]; c.held = p.held; c.hasPhone = p.hasPhone; memcpy(c.aff, p.aff, sizeof c.aff); memcpy(c.friends, p.friends, sizeof c.friends); }
            }
        }
        else if (!memcmp(tag, "PROP", 4)) { G.props.resize(n / sizeof(Prop)); if (n) memcpy(G.props.data(), d.data(), G.props.size() * sizeof(Prop)); }
    }
    fclose(f); if (!sawPlay) G.realClock = 1;                  // saves from before v0.4 move to the real clock
    notice("Save loaded."); return true;
}

// ---------------------------------------------------------------- rules
static int roomPop(int room, int* out = nullptr, int cap = 0) {
    int n = 0; for (int i = 0; i < NCHAR; i++) if (G.ch[i].room == room) { if (out && n < cap) out[n] = i; n++; } return n;
}
static void addAff(int a, int b, float d) {
    auto cl = [](float v) { return std::max(0.f, std::min(100.f, v)); };
    float cap = 100;
    if (isBeef(a, b)) cap = 30; if (isHateCrush(a, b)) cap = 6;
    G.ch[a].aff[b] = std::min(cap, cl(G.ch[a].aff[b] + d)); G.ch[b].aff[a] = std::min(cap, cl(G.ch[b].aff[a] + d));
}
static void addMood(int ci, float d) { G.ch[ci].mood = std::max(0.f, std::min(100.f, G.ch[ci].mood + d)); }
static void addBond(int ci, float d) { G.ch[ci].bond = std::max(0.f, std::min(100.f, G.ch[ci].bond + d)); }
static int favFood(int ci) { return (ci * 7 + 3) % 11; }
static int favGood(int ci) { return GOODS_FAV[(ci * 3 + 1) % 10]; }
static int wantedItemFor(int ci) {
    Char& c = G.ch[ci]; if (c.hunger < 35) return rf() < 0.5f ? favFood(ci) : ri(11);
    int r = ri(3); if (r == 0) return favGood(ci); if (r == 1) return 11 + ri(11); return favFood(ci);
}
static void clearBubble(int ci) { G.ch[ci].bubble = B_NONE; G.ch[ci].bArg = -1; G.ch[ci].bCool = 40 + ri(40); }

static std::string chatLine(int a, int b) {
    if (isHateCrush(a, b)) return a == I_GABRIEL ? "Dalia! Uh. Nice weather. Is it weather? It's weather." : "No.";
    if (ROSTER[a].pers == P_TINY) return pick(L_TINY_CHAT);
    return voiced(a, fmtName(pick(L_CHAT), nm(b)));
}
static void textFrom(int a) {
    Char& c = G.ch[a]; if (!c.hasPhone) return;
    int b = ri(NCHAR); if (b == a) return;
    char buf[256]; snprintf(buf, sizeof buf, "%s -> %s: %s", nm(a), nm(b), pick(L_PHONE_TXT));
    G.texts.push_front(buf); while (G.texts.size() > 40) G.texts.pop_back(); addAff(a, b, 0.6f);
}
static void pairTrigger(int a, int b) {
    // called after a conversation; may raise bubbles that the player can act on
    Char& A = G.ch[a];
    if (a == I_GABRIEL && b == I_DALIA && A.bubble == B_NONE) { A.bubble = B_CRUSH; A.bArg = b; return; }
    if (isHateCrush(a, b)) return;
    if (A.bubble != B_NONE || A.bCool > 0) return;
    if (!A.friends[b] && A.aff[b] >= 30) { A.bubble = B_FRIEND; A.bArg = b; return; }
    if (A.aff[b] < 6 && rf() < 0.25f) { A.bubble = B_STUCK; A.bArg = b; return; }
}
static void finishConvo(int room) {
    Convo& cv = G.convo[room]; int a = cv.a, b = cv.b; cv.on = false;
    float d = isBeef(a, b) ? 0.6f : 3.0f + (ROSTER[a].pers == ROSTER[b].pers ? 1.5f : 0.f) + (isSiblings(a, b) ? 0.5f : 0.f);
    if (isHateCrush(a, b)) d = -0.5f;
    addAff(a, b, d); addMood(a, 2); addMood(b, 2);
    pairTrigger(a, b); if (rf() < 0.5f) pairTrigger(b, a);
}
static void startConvo(int room) {
    int in[NCHAR]; int n = roomPop(room, in, NCHAR); std::vector<int> ok;
    for (int k = 0; k < n; k++) { Char& c = G.ch[in[k]]; if (c.state != S_SLEEP && c.state != S_HELD && c.state != S_EAT) ok.push_back(in[k]); }
    if (ok.size() < 2) return;
    int a = ok[ri((int)ok.size())], b; do { b = ok[ri((int)ok.size())]; } while (b == a);
    Convo& cv = G.convo[room]; cv.on = true; cv.a = a; cv.b = b; cv.stage = 0; cv.t = 0;
    if (room != G.viewRoom) { finishConvo(room); return; }          // off-screen: resolve instantly
    Char &A = G.ch[a], &B = G.ch[b]; float mx = (A.x + B.x) * 0.5f, mz = (A.z + B.z) * 0.5f;
    A.tx = mx - 0.55f; A.tz = mz; B.tx = mx + 0.55f; B.tz = mz; A.state = B.state = S_WALK;
}
static void updateConvo(int room, float dt) {
    Convo& cv = G.convo[room]; if (!cv.on) return;
    Char &A = G.ch[cv.a], &B = G.ch[cv.b];
    if (A.room != room || B.room != room || A.state == S_HELD || B.state == S_HELD || A.state == S_SLEEP || B.state == S_SLEEP) { cv.on = false; return; }
    cv.t += dt;
    if (cv.stage == 0 && (cv.t > 4.f || (std::fabs(A.x - A.tx) < 0.15f && std::fabs(B.x - B.tx) < 0.15f && cv.t > 0.6f))) { cv.stage = 1; cv.t = 0; A.faceT = std::atan2(B.x - A.x, B.z - A.z); B.faceT = std::atan2(A.x - B.x, A.z - B.z); A.state = B.state = S_CHAT; speak(cv.a, chatLine(cv.a, cv.b), 3.4f); sfx(SFX_TAP); }
    else if (cv.stage == 1 && cv.t > 3.5f) { cv.stage = 2; cv.t = 0; std::string r = isHateCrush(cv.a, cv.b) ? "No." : (ROSTER[cv.b].pers == P_TINY ? std::string(pick(L_TINY_CHAT)) : voiced(cv.b, pick(L_REPLY))); speak(cv.b, r, 3.2f); }
    else if (cv.stage == 2 && cv.t > 3.4f) { A.state = B.state = S_IDLE; finishConvo(room); }
}

static void relocate(int ci) {
    Char& c = G.ch[ci]; int old = c.room;
    if (sleepTime()) { if (c.room != homeRoom(ci)) { setRoom(ci, homeRoom(ci), false); } return; }
    if (ci == I_AMYAS && hourOf() >= 9 && hourOf() < 17) { if (c.room != ROOM_ICE) setRoom(ci, ROOM_ICE, old == G.viewRoom ? false : false); return; }
    if (c.state == S_HELD) return;
    int r = rf() < 0.45f ? homeRoom(ci) : (NCHAR + ri(3));
    if (rf() < 0.15f) r = ri(NCHAR);
    if (r != c.room) setRoom(ci, r, true);
}

static void tickMinute() {
    G.minute++;
    if (G.minute >= 1440) { G.minute = 0; G.day++; }
    if (G.day != G.lastDayPaid) { G.lastDayPaid = G.day; G.money += 300; notice("Day " + std::to_string(G.day) + ": the island paid you 300 DA. Nobody knows why."); }
    int h = hourOf(), mi = G.minute % 60;
    for (int i = 0; i < NCHAR; i++) {
        Char& c = G.ch[i];
        c.hunger = std::max(0.f, c.hunger - 0.075f);
        bool asleep = c.state == S_SLEEP;
        c.energy = asleep ? std::min(100.f, c.energy + 0.25f) : std::max(0.f, c.energy - 0.045f);
        float target = 58 + (c.hunger < 25 ? -28 : 0) + (c.energy < 15 ? -14 : 0) + (c.bond * 0.1f);
        c.mood += (target - c.mood) * 0.012f;
        if (c.bCool > 0) c.bCool--;
        // sleep schedule
        if (sleepTime()) {
            if (c.room != homeRoom(i) && c.state != S_HELD) { setRoom(i, homeRoom(i), false); }
            if (c.room == homeRoom(i) && c.state != S_SLEEP && c.state != S_HELD && mi % 5 == 0) { c.state = S_SLEEP; c.bubble = B_DREAM; c.dream = ri(NDREAMS); c.x = -2.4f; c.z = -1.9f; c.tx = c.x; c.tz = c.z; }
        } else if (c.state == S_SLEEP && h >= 7) { c.state = S_IDLE; if (c.bubble == B_DREAM) c.bubble = B_NONE; randomSpot(c.room, c.tx, c.tz); }
        if (c.state == S_SLEEP) continue;
        if (c.state != S_HELD && (mi == 0 || mi == 30) && rf() < 0.35f) relocate(i);
        if (i == I_AMYAS && h == 9 && mi == 0) relocate(i);
        // bubbles
        if (c.bubble == B_NONE && c.bCool == 0 && c.state != S_HELD) {
            if (c.hunger < 28) { c.bubble = B_WANT; c.bArg = favFood(i); if (rf() < 0.5f) c.bArg = ri(11); }
            else if (c.mood < 26) { c.bubble = B_SAD; }
            else if (rf() < 0.004f && c.mood > 45) { c.bubble = B_GAME; }
            else if (rf() < 0.004f) { c.bubble = B_WANT; c.bArg = wantedItemFor(i); }
        }
        if (rf() < 0.002f && c.bubble == B_NONE && c.state == S_IDLE) {                 // idle thought about someone
            int in[NCHAR]; int n = roomPop(c.room, in, NCHAR); int o = n > 1 ? in[ri(n)] : ri(NCHAR); if (o == i) o = (i + 1) % NCHAR;
            float a = c.aff[o]; std::string t;
            if (isHateCrush(i, o) && i == I_DALIA) t = "Gabriel again. Why is he always there.";
            else if (a >= 55) t = fmtName("I really like hanging out with {n}.", nm(o));
            else if (a >= 30) t = fmtName("{n} is actually pretty cool.", nm(o));
            else if (a < 10) t = fmtName("{n} gives me weird vibes.", nm(o));
            else t = fmtName("I wonder what {n} is up to.", nm(o));
            speak(i, t, 4.f, true);
        }
        if (c.hasPhone && rf() < 0.012f) textFrom(i);
    }
    // cursed events
    if (mi == 0 && rf() < 0.10f * G.glitchLevel) {
        int who = ri(NCHAR); std::string ev = fmtName(pick(CURSED_EVENTS), nm(who));
        notice(ev); G.curseFlash = 1.f; sfx(SFX_GLITCH);
        if (ev.find("Ash Crystal") != std::string::npos) { G.bag[IT_CRYSTAL]++; }
        for (int i = 0; i < NCHAR; i++) G.ch[i].glitch = 1.f;
    }
    // off-screen conversations
    for (int r = 0; r < NROOMS; r++) if (r != G.viewRoom && mi % 12 == 0 && rf() < 0.5f) startConvo(r);
}

// ---------------------------------------------------------------- giving things
static int slotOf(const ItemDef& it) { return it.slot; }
static void giveItem(int ci, int item, std::string& reply) {
    Char& c = G.ch[ci]; const ItemDef& it = ITEMS[item]; G.bag[item]--;
    bool fav = false; float gain = 8;
    if (it.cat == CAT_FOOD) {
        c.hunger = std::min(100.f, c.hunger + it.power); c.state = S_EAT; c.stateT = 1.4f; fav = favFood(ci) == item; gain = fav ? 25 : 8;
        reply = fav ? pick(L_FAVE) : pick(L_THX_FOOD);
    } else if (it.cat == CAT_CLOTH) {
        int prev = c.outfit[it.slot]; if (prev >= 0) G.bag[prev]++; c.outfit[it.slot] = item; c.bounce = 1.f; gain = 10; reply = pick(L_THX_CLOTH);
        if (item == IT_CROWN) reply = "I'm the king of the island now.";
    } else {
        fav = favGood(ci) == item; gain = fav ? 28 : 10; reply = fav ? pick(L_FAVE) : pick(L_THX_ITEM); c.bounce = 1.f;
        if (item == IT_PHONE) { c.hasPhone = true; reply = "A phone! I can text people now!"; }
        else if (item == IT_CRYSTAL) { reply = "It's warm. It's humming. I love it."; c.glitch = 1.5f; G.curseFlash = 0.8f; sfx(SFX_GLITCH); }
        else if (item == IT_MYSTERY) { int p = ri(NITEMS); while (p == IT_CRYSTAL || p == IT_MYSTERY) p = ri(NITEMS); G.bag[p]++; reply = std::string("There was a ") + ITEMS[p].name + " inside! You can have it."; gain = 12; }
        else { if (c.held >= 0 && c.held != item) G.bag[c.held]++; c.held = item; }
    }
    if (c.bubble == B_WANT && c.bArg == item) { gain += 18; addBond(ci, 6); G.money += 40; c.bubble = B_NONE; c.bCool = 30; }
    else if (c.bubble == B_WANT && ITEMS[c.bArg].cat == it.cat && it.cat == CAT_FOOD) { c.bubble = B_NONE; c.bCool = 30; }
    else if (c.bubble == B_SAD && c.mood + gain > 40) { c.bubble = B_NONE; c.bCool = 40; }
    else if (c.bubble == B_STUCK && rf() < 0.4f) { addAff(ci, c.bArg, 10); c.bubble = B_NONE; c.bCool = 40; reply += " ...okay, maybe I overreacted."; }
    if (it.cat != CAT_FOOD && !fav && !(c.bubble == B_WANT)) { if (rf() < 0.12f && it.cat == CAT_GOODS) reply = pick(L_MEH); }
    addMood(ci, gain); addBond(ci, fav ? 4 : 2); c.sayT = 0;
    speak(ci, voiced(ci, ROSTER[ci].pers == P_TINY ? std::string(pick(L_TINY_THX)) : reply), 3.6f);
    sfx(fav ? SFX_GOOD : SFX_POP);
}
