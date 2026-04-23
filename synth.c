#define _USE_MATH_DEFINES
#include "synth.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Audio settings
#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BUFFER_SIZE 2048

// Music settings
#define BPM 150
#define ROWS_PER_PATTERN 128 // 16 x 8
#define TICKS_PER_ROW 6
#define ROWS_PER_BEAT 4
#define SAMPLES_PER_TICK ((SAMPLE_RATE * 60) / (BPM * ROWS_PER_BEAT * TICKS_PER_ROW))
#define SAMPLES_PER_ROW (SAMPLES_PER_TICK * TICKS_PER_ROW)

// Note frequencies (C major scale, multiple octaves)
#define NOTE_C2  65.41f
#define NOTE_D2  73.42f
#define NOTE_E2  82.41f
#define NOTE_F2  87.31f
#define NOTE_G2  98.00f
#define NOTE_A2  110.00f
#define NOTE_C3  130.81f
#define NOTE_D3  146.83f
#define NOTE_E3  164.81f
#define NOTE_F3  174.61f
#define NOTE_G3  196.00f
#define NOTE_A3  220.00f
#define NOTE_C4  261.63f
#define NOTE_D4  293.66f
#define NOTE_E4  329.63f
#define NOTE_F4  349.23f
#define NOTE_G4  392.00f
#define NOTE_A4  440.00f
#define NOTE_C5  523.25f
#define NOTE_D5  587.33f
#define NOTE_E5  659.25f
#define NOTE_F5  698.46f
#define NOTE_G5  783.99f
#define NOTE_A5  880.00f

// Note type
typedef struct {
    float freq;
    int duration;  // in samples
} Note;

// Instrument types
typedef enum {
    INST_NONE = 0,
    INST_KICK,
    INST_SNARE,
    INST_HIHAT,
    INST_BASS,
    INST_LEAD,
    INST_HARMONY
} InstrumentType;

// Channel state
typedef struct {
    InstrumentType instrument;
    float freq;
    float phase;
    int samples_left;
    float volume;
    float env_time;
    // Arpeggio state
    float arp[3];     // chord freqs: root, 3rd, 5th (0 = unused)
    int   arp_step;
    int   arp_timer;
} Channel;

// Pattern note entry
typedef struct {
    InstrumentType inst;
    float freq;
    float freq2;   // 2nd arp note (0 = no arp)
    float freq3;   // 3rd arp note (0 = no arp)
} PatternNote;

// Global state
static SDL_AudioDeviceID audio_device;
static Channel channels[6];  // kick, snare, hihat, bass, lead, harmony
static int current_row = 0;
static int current_tick = 0;
static int tick_samples = 0;

// --- WAVEFORM GENERATORS ---

static float sine_wave(float phase) {
    return sinf(phase * 2.0f * M_PI);
}

static float triangle_wave(float phase) {
    float p = phase - floorf(phase);
    return (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
}

static float pulse_wave(float phase, float duty) {
    return (phase - floorf(phase)) < duty ? 1.0f : -1.0f;
}

static uint16_t lfsr_state = 1;
static float lfsr_noise(void) {
    uint16_t bit = ((lfsr_state >> 1) ^ lfsr_state) & 1;
    lfsr_state = (lfsr_state >> 1) | (uint16_t)(bit << 14);
    return (lfsr_state & 1) ? 1.0f : -1.0f;
}

// --- INSTRUMENT GENERATORS ---

static float generate_kick(Channel *ch, float sample_rate) {
    float t = ch->env_time;
    float pitch_env = expf(-t * 15.0f);
    float amp_env   = expf(-t * 5.0f);

    float freq = 60.0f + pitch_env * 100.0f;
    ch->phase += freq / sample_rate;

    float body      = sine_wave(ch->phase) * amp_env;
    float transient = lfsr_noise() * expf(-t * 80.0f) * 0.4f;
    return (body + transient) * 0.5f;
}

static float generate_snare(Channel *ch, float sample_rate) {
    float t = ch->env_time;
    float amp_env = expf(-t * 20.0f);

    float noise = lfsr_noise() * 0.7f;
    ch->phase += 200.0f / sample_rate;
    float tone = sine_wave(ch->phase) * 0.3f;

    return (noise + tone) * amp_env * 0.25f;
}

static float generate_hihat(Channel *ch, float sample_rate) {
    float t = ch->env_time;
    float amp_env = expf(-t * 22.0f);

    (void)sample_rate;
    return lfsr_noise() * amp_env * 0.07f;
}

static float generate_bass(Channel *ch, float sample_rate) {
    float t = ch->env_time;
    int step = (int)(t * 60.0f);
    float amp_env = (step < 4) ? (1.0f - expf(-t * 30.0f))
                                : fmaxf(0.0f, 1.0f - (step - 3) * 0.08f);
    ch->phase += ch->freq / sample_rate;
    return triangle_wave(ch->phase) * amp_env * 0.55f;
}

static float generate_lead(Channel *ch, float sample_rate) {
    float t = ch->env_time;
    int step = (int)(t * 60.0f);
    float amp_env = fmaxf(0.0f, 1.0f - step / 15.0f);
    ch->phase += ch->freq / sample_rate;
    return pulse_wave(ch->phase, 0.50f) * amp_env * 0.18f;
}

static float generate_harmony(Channel *ch, float sample_rate) {
    float t = ch->env_time;
    int step = (int)(t * 60.0f);
    float amp_env = fmaxf(0.0f, 1.0f - step / 20.0f);
    ch->phase += ch->freq / sample_rate;
    return pulse_wave(ch->phase, 0.50f) * amp_env * 0.20f;
}

// --- PATTERN DATA ---

// Kick pattern – 4/4 with fill at end
static const PatternNote kick_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15)
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 2 (16–31)
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 3 (32–47)
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 4 (48–63) – fill at end
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_KICK, NOTE_C2}, {INST_KICK, NOTE_C2},

    // Bar 5 (64–79)
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 6 (80–95)
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 7 (96–111)
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 8 (112–127) – fill at end
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_KICK, NOTE_C2}, {INST_KICK, NOTE_C2}
};

// Snare pattern – backbeat on 2 & 4 with fill
static const PatternNote snare_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – no snare for clean intro
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 2 (16–31) – backbeat
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},  {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 3 (32–47) – same backbeat
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},  {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 4 (48–63) – backbeat + fill
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},  {INST_NONE, 0},
    {INST_NONE, 0}, {INST_SNARE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 5 (64–79) – backbeat on 2 & 4
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 6 (80–95) – backbeat
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 7 (96–111) – backbeat
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 8 (112–127) – backbeat + fill
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_SNARE, 0}, {INST_SNARE, 0}, {INST_NONE, 0}
};

// Hi-hat pattern – light groove then driving last bar
static const PatternNote hihat_pattern[ROWS_PER_PATTERN] = {
    // Bars 1–3
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    // Bar 4 – driving 8th-note hats
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    // Bars 5–7 – quarter hats
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    // Bar 8 – driving 8th-note hats
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0}
};

// Bass pattern – backbeat 2 & 4, syncopated fills
static const PatternNote bass_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – C: root on 1, beat 2, and-of-2, beat 4
    {INST_BASS, NOTE_C2}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_C3}, {INST_NONE, 0}, {INST_BASS, NOTE_G2}, {INST_NONE, 0},
    {INST_NONE, 0},       {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_G2}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 2 (16–31) – G: syncopated and-of-1, beat 2, beat 3, beat 4
    {INST_NONE, 0},       {INST_NONE, 0}, {INST_BASS, NOTE_G2}, {INST_NONE, 0},
    {INST_BASS, NOTE_G3}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_G2}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_D3}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 3 (32–47) – Am: root on 1, beat 2, beat 3, beat 4, and-of-4 walk
    {INST_BASS, NOTE_A2}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_A3}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_A2}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_E3}, {INST_NONE, 0}, {INST_BASS, NOTE_C3}, {INST_NONE, 0},

    // Bar 4 (48–63) – F→C: beat 2, and-of-2, beat 3, beat 4
    {INST_NONE, 0},       {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_F3}, {INST_NONE, 0}, {INST_BASS, NOTE_C3}, {INST_NONE, 0},
    {INST_BASS, NOTE_F2}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_BASS, NOTE_C3}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 5 (64–79) – C: root on 1, beat 2, and-of-2, beat 4, and-of-4
    {INST_BASS, NOTE_C3}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_E3}, {INST_NONE, 0}, {INST_BASS, NOTE_G2}, {INST_NONE, 0},
    {INST_NONE, 0},       {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_G2}, {INST_NONE, 0}, {INST_BASS, NOTE_C2}, {INST_NONE, 0},

    // Bar 6 (80–95) – Am: syncopated and-of-1, beat 2, beat 3, beat 4
    {INST_NONE, 0},       {INST_NONE, 0}, {INST_BASS, NOTE_A2}, {INST_NONE, 0},
    {INST_BASS, NOTE_E3}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_A2}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_C3}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},

    // Bar 7 (96–111) – F: root on 1, beat 2, and-of-2, beat 3, beat 4, and-of-4
    {INST_BASS, NOTE_F2}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_A2}, {INST_NONE, 0}, {INST_BASS, NOTE_C3}, {INST_NONE, 0},
    {INST_BASS, NOTE_A2}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_C3}, {INST_NONE, 0}, {INST_BASS, NOTE_F2}, {INST_NONE, 0},

    // Bar 8 (112–127) – G→C: beat 1, beat 2, double on 3, double on 4
    {INST_BASS, NOTE_G2}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_D3}, {INST_NONE, 0}, {INST_NONE, 0},       {INST_NONE, 0},
    {INST_BASS, NOTE_G2}, {INST_NONE, 0}, {INST_BASS, NOTE_G2}, {INST_NONE, 0},
    {INST_BASS, NOTE_C3}, {INST_NONE, 0}, {INST_BASS, NOTE_C3}, {INST_NONE, 0},
};

// Lead pattern – 50% pulse, adventure platformer melody
static const PatternNote lead_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – C: E4 G4 A4 G4 E4 C4 D4 E4
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},

    // Bar 2 (16–31) – G: C4 E4 G4 E4 D4 F4 E4 D4
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_F4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},

    // Bar 3 (32–47) – Am: G3 C4 E4 G4 A4 G4 E4 C4
    {INST_LEAD, NOTE_G3}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0},

    // Bar 4 (48–63) – F→C: D4 E4 G4 A4 G4 E4 D4 C4
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0},

    // Bar 5 (64–79) – C: sparse quarter notes C4 E4 G4 E4
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 6 (80–95) – Am: A3 C4 E4 G4
    {INST_LEAD, NOTE_A3}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 7 (96–111) – F: F4 A4 C5 A4 (builds up)
    {INST_LEAD, NOTE_F4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 8 (112–127) – G→C: G4 D5 G4 C5 (resolve back to top)
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
};

// Harmony pattern – 50% pulse arpeggio, one chord per beat (4 rows)
static const PatternNote harmony_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – C major: C4+E4+G4
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 2 (16–31) – G5: G4+D5+G5
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 3 (32–47) – A minor: A4+C5+E5
    {INST_HARMONY, NOTE_A4, NOTE_C5, NOTE_E5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_A4, NOTE_C5, NOTE_E5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_A4, NOTE_C5, NOTE_E5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_A4, NOTE_C5, NOTE_E5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 4a (48–55) – F major: F4+A4+C5
    {INST_HARMONY, NOTE_F4, NOTE_A4, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_F4, NOTE_A4, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    // Bar 4b (56–63) – C major resolve: C4+E4+G4
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 5 (64–79) – C major: C4+E4+G4
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_C4, NOTE_E4, NOTE_G4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 6 (80–95) – A minor: A3+C4+E4
    {INST_HARMONY, NOTE_A3, NOTE_C4, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_A3, NOTE_C4, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_A3, NOTE_C4, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_A3, NOTE_C4, NOTE_E4}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 7 (96–111) – F major: F4+A4+C5
    {INST_HARMONY, NOTE_F4, NOTE_A4, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_F4, NOTE_A4, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_F4, NOTE_A4, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_F4, NOTE_A4, NOTE_C5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 8 (112–127) – G major: G4+D5+G5
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_HARMONY, NOTE_G4, NOTE_D5, NOTE_G5}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
};

// --- TRIGGER NOTE ON CHANNEL ---

static void trigger_note(int channel_idx, InstrumentType inst,
                         float freq, float freq2, float freq3) {
    Channel *ch = &channels[channel_idx];
    ch->instrument = inst;
    ch->freq       = freq;
    ch->phase      = 0.0f;
    ch->env_time   = 0.0f;
    ch->arp[0]     = freq;
    ch->arp[1]     = freq2;
    ch->arp[2]     = freq3 ? freq3 : freq;
    ch->arp_step   = 0;
    ch->arp_timer  = 0;

    switch (inst) {
        case INST_KICK:
        case INST_SNARE:
        case INST_HIHAT:
            ch->samples_left = SAMPLES_PER_TICK * 2;
            break;
        case INST_BASS:
        case INST_HARMONY:
            ch->samples_left = SAMPLES_PER_ROW * 4 - SAMPLES_PER_TICK;
            break;
        case INST_LEAD:
            ch->samples_left = SAMPLES_PER_ROW * 2 - SAMPLES_PER_TICK;
            break;
        default:
            ch->samples_left = SAMPLES_PER_TICK * 2;
            break;
    }
}

// --- PROCESS PATTERN ROW ---

static void process_row(void) {
    const PatternNote *patterns[6] = {
        kick_pattern,
        snare_pattern,
        hihat_pattern,
        bass_pattern,
        lead_pattern,
        harmony_pattern
    };

    for (int i = 0; i < 6; i++) {
        const PatternNote *note = &patterns[i][current_row];
        if (note->inst != INST_NONE) {
            trigger_note(i, note->inst, note->freq, note->freq2, note->freq3);
        }
    }

    current_row = (current_row + 1) % ROWS_PER_PATTERN;
}

// --- GENERATE SAMPLE FOR CHANNEL ---

static float generate_channel_sample(Channel *ch) {
    if (ch->instrument == INST_NONE || ch->samples_left <= 0) {
        return 0.0f;
    }

    // Arpeggio: cycle ch->freq through arp[] at ~64 Hz
    if (ch->arp[1] > 0.0f) {
        if (++ch->arp_timer >= SAMPLE_RATE / 64) {
            ch->arp_timer = 0;
            ch->arp_step  = (ch->arp_step + 1) % 3;
            ch->freq      = ch->arp[ch->arp_step];
        }
    }

    float sample = 0.0f;

    switch (ch->instrument) {
        case INST_KICK:
            sample = generate_kick(ch, SAMPLE_RATE);
            break;
        case INST_SNARE:
            sample = generate_snare(ch, SAMPLE_RATE);
            break;
        case INST_HIHAT:
            sample = generate_hihat(ch, SAMPLE_RATE);
            break;
        case INST_BASS:
            sample = generate_bass(ch, SAMPLE_RATE);
            break;
        case INST_LEAD:
            sample = generate_lead(ch, SAMPLE_RATE);
            break;
        case INST_HARMONY:
            sample = generate_harmony(ch, SAMPLE_RATE);
            break;
        default:
            break;
    }

    ch->env_time += 1.0f / SAMPLE_RATE;
    ch->samples_left--;

    return sample;
}

// --- SDL AUDIO CALLBACK ---

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;

    Sint16 *buffer = (Sint16 *)stream;
    int samples = len / (sizeof(Sint16) * CHANNELS);

    for (int i = 0; i < samples; i++) {
        if (tick_samples >= SAMPLES_PER_TICK) {
            tick_samples = 0;
            current_tick++;

            if (current_tick >= TICKS_PER_ROW) {
                current_tick = 0;
                process_row();
            }
        }
        tick_samples++;

        float mixed = 0.0f;
        for (int ch = 0; ch < 6; ch++) {
            mixed += generate_channel_sample(&channels[ch]);
        }

        mixed = fmaxf(-1.0f, fminf(1.0f, mixed));
        Sint16 sample_16 = (Sint16)(mixed * 32767.0f);

        buffer[i * 2]     = sample_16;
        buffer[i * 2 + 1] = sample_16;
    }
}

// --- INITIALIZE SYNTHESIZER ---

void synth_init(void) {
    memset(channels, 0, sizeof(channels));
    lfsr_state   = 1;
    current_row  = 0;
    current_tick = 0;
    tick_samples = 0;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = CHANNELS;
    want.samples  = BUFFER_SIZE;
    want.callback = audio_callback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    if (audio_device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(audio_device, 0);

    printf("Synthesizer initialized: %d Hz, %d channels\n", have.freq, have.channels);
}

// --- CLEANUP SYNTHESIZER ---

void synth_cleanup(void) {
    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
}
