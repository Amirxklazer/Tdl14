// audio.hpp - procedural soundtrack + sfx (no asset files needed).
// Drop-in replacement: put menu.wav / day.wav / night.wav / minigame.wav in <save dir>/music/ and they loop instead.
#pragma once
#include <SDL.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>

enum { TR_TITLE, TR_DAY, TR_NIGHT, TR_GAME, TR_COUNT };
enum { SFX_TAP, SFX_GOOD, SFX_BAD, SFX_POP, SFX_DROP, SFX_COIN, SFX_GLITCH, SFX_BELL, SFX_COUNT };

static const int AUD_RATE = 48000;
static const int REST = -99;
struct TrackDef { float bpm; int root; int wave; std::vector<int> mel; std::vector<int> bass; };
static std::vector<TrackDef> TRACKS;
static std::vector<int16_t> userPcm[TR_COUNT];
static SDL_AudioDeviceID audDev = 0;
static volatile int curTrack = TR_TITLE;
static volatile float musicVol = 0.5f, sfxVol = 0.7f;
static volatile float curseAmt = 0.4f;
static double trackPos[TR_COUNT] = {0, 0, 0, 0};
static size_t userPos[TR_COUNT] = {0, 0, 0, 0};
static float ph1 = 0, ph2 = 0;
struct SfxVoice { bool on; float f0, f1, t, dur; int wave; float vol; float ph; };
static SfxVoice sfxv[8];

static float waveS(int w, float p) {
    p -= std::floor(p);
    switch (w) { case 0: return p < 0.5f ? 0.6f : -0.6f; case 1: return p < 0.25f ? 0.6f : -0.6f; case 2: return 4.f * std::fabs(p - 0.5f) - 1.f; default: return (p * 2.f - 1.f) * 0.6f; }
}
static void audioCb(void*, Uint8* stream, int len) {
    int16_t* out = (int16_t*)stream; int n = len / 4;
    int tr = curTrack; float mv = musicVol, sv = sfxVol;
    for (int i = 0; i < n; i++) {
        float s = 0;
        if (!userPcm[tr].empty()) {
            auto& pcm = userPcm[tr]; size_t& pos = userPos[tr];
            float l = pcm[pos] / 32768.f, r = pcm[pos + 1] / 32768.f; pos += 2; if (pos + 1 >= pcm.size()) pos = 0;
            float o = 0;
            for (auto& v : sfxv) if (v.on) { v.t += 1.f / AUD_RATE; if (v.t > v.dur) { v.on = false; continue; } float f = v.f0 + (v.f1 - v.f0) * (v.t / v.dur); v.ph += f / AUD_RATE; o += waveS(v.wave, v.ph) * v.vol * (1.f - v.t / v.dur) * 0.3f * sv; }
            int L = (int)((l * mv + o) * 32000), Rr = (int)((r * mv + o) * 32000);
            out[i * 2] = (int16_t)std::max(-32767, std::min(32767, L)); out[i * 2 + 1] = (int16_t)std::max(-32767, std::min(32767, Rr)); continue;
        }
        const TrackDef& T = TRACKS[tr];
        double& tp = trackPos[tr];
        double stepLen = AUD_RATE * 60.0 / T.bpm / 2.0;               // eighth notes
        tp += 1.0; long step = (long)(tp / stepLen); double within = (tp - step * stepLen) / stepLen;
        int mi = (int)(step % T.mel.size()); int bi = (int)((step / 2) % T.bass.size());
        float wob = 1.f + 0.006f * curseAmt * std::sin((float)tp * 2.f * 3.14159f * 0.23f / AUD_RATE);
        int m = T.mel[mi];
        if (m != REST) { float f = 440.f * std::pow(2.f, (T.root + m - 9) / 12.f) * wob; ph1 += f / AUD_RATE; float env = std::exp(-3.5f * (float)within); s += waveS(T.wave, ph1) * env * 0.35f; }
        int b = T.bass[bi];
        if (b != REST) { float f = 440.f * std::pow(2.f, (T.root + b - 21) / 12.f) * wob; ph2 += f / AUD_RATE; float env = 0.7f + 0.3f * std::exp(-2.f * (float)((tp - (step / 2) * stepLen * 2) / (stepLen * 2))); s += waveS(2, ph2) * env * 0.4f; }
        // cursed bit-crush hiss
        if (curseAmt > 0.6f && (rand() % 90000) == 0) s += 0.6f * ((rand() & 255) / 128.f - 1.f);
        s *= mv;
        for (auto& v : sfxv) if (v.on) { v.t += 1.f / AUD_RATE; if (v.t > v.dur) { v.on = false; continue; } float f = v.f0 + (v.f1 - v.f0) * (v.t / v.dur); v.ph += f / AUD_RATE; float w = v.wave == 9 ? ((rand() & 255) / 128.f - 1.f) * 0.5f : waveS(v.wave, v.ph); s += w * v.vol * (1.f - v.t / v.dur) * 0.5f * sv; }
        int o = (int)(s * 26000); o = std::max(-32767, std::min(32767, o));
        out[i * 2] = out[i * 2 + 1] = (int16_t)o;
    }
}
static void loadUserWavs(const std::string& dir) {
    static const char* names[TR_COUNT] = {"menu.wav", "day.wav", "night.wav", "minigame.wav"};
    for (int i = 0; i < TR_COUNT; i++) {
        std::string p = dir + "music/" + names[i];
        SDL_AudioSpec spec; Uint8* buf; Uint32 len;
        if (!SDL_LoadWAV(p.c_str(), &spec, &buf, &len)) continue;
        SDL_AudioCVT cvt;
        if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_S16SYS, 2, AUD_RATE) >= 0) {
            cvt.len = (int)len; cvt.buf = (Uint8*)SDL_malloc(len * cvt.len_mult); memcpy(cvt.buf, buf, len);
            if (SDL_ConvertAudio(&cvt) == 0) { userPcm[i].assign((int16_t*)cvt.buf, (int16_t*)(cvt.buf + cvt.len_cvt)); if (userPcm[i].size() < 4) userPcm[i].clear(); }
            SDL_free(cvt.buf);
        }
        SDL_FreeWAV(buf);
    }
}
static void initAudio(const std::string& dir) {
    // title: slow, minor, uneasy
    TRACKS.push_back({78, 57, 2, {0, REST, 3, REST, 7, REST, 5, 3, 0, REST, -2, REST, 3, REST, REST, REST, 7, REST, 10, REST, 8, 7, 5, REST, 3, REST, 2, REST, 0, REST, REST, REST}, {0, REST, -4, REST, -2, REST, -5, REST}});
    // day: bright pentatonic
    TRACKS.push_back({112, 60, 1, {0, 4, 7, 4, 9, 7, 4, 2, 0, 4, 7, 12, 9, 7, 4, REST, 2, 5, 9, 5, 12, 9, 5, 4, 0, 2, 4, 7, 4, 2, 0, REST}, {0, 0, -3, -3, -5, -5, -7, -7}});
    // night: sparse, detuned feeling
    TRACKS.push_back({70, 52, 2, {0, REST, REST, 3, REST, REST, 7, REST, REST, 5, REST, 3, 0, REST, REST, REST, -2, REST, REST, 2, REST, REST, 5, REST, 3, REST, REST, REST, REST, REST, REST, REST}, {0, REST, REST, REST, -4, REST, REST, REST}});
    // minigame: fast arps
    TRACKS.push_back({150, 62, 0, {0, 4, 7, 12, 7, 4, 0, 4, 2, 5, 9, 14, 9, 5, 2, 5, -1, 2, 5, 11, 5, 2, -1, 2, 0, 4, 7, 12, 14, 12, 7, 4}, {0, -3, -5, -7}});
    memset(sfxv, 0, sizeof sfxv);
    SDL_AudioSpec want, have; SDL_zero(want); want.freq = AUD_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024; want.callback = audioCb;
    audDev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!audDev) { SDL_Log("audio open failed: %s", SDL_GetError()); return; }
    loadUserWavs(dir);
    SDL_PauseAudioDevice(audDev, 0);
}
static void sfx(int type) {
    if (!audDev) return;
    struct P { float f0, f1, dur; int wave; float vol; };
    static const P ps[SFX_COUNT] = {{900, 1200, 0.06f, 0, 0.5f}, {500, 1400, 0.25f, 1, 0.7f}, {400, 120, 0.3f, 3, 0.7f}, {300, 700, 0.1f, 2, 0.8f},
                                    {500, 200, 0.12f, 2, 0.8f}, {1200, 1800, 0.12f, 0, 0.6f}, {200, 60, 0.35f, 9, 0.8f}, {880, 880, 0.5f, 2, 0.7f}};
    SDL_LockAudioDevice(audDev);
    for (auto& v : sfxv) if (!v.on) { v.on = true; v.f0 = ps[type].f0; v.f1 = ps[type].f1; v.dur = ps[type].dur; v.wave = ps[type].wave; v.vol = ps[type].vol; v.t = 0; v.ph = 0; break; }
    SDL_UnlockAudioDevice(audDev);
}
