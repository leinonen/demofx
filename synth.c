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
#define BPM 120
#define ROWS_PER_PATTERN 64 // 16 x 4
#define TICKS_PER_ROW 6
#define ROWS_PER_BEAT 4
#define SAMPLES_PER_TICK ((SAMPLE_RATE * 60) / (BPM * ROWS_PER_BEAT * TICKS_PER_ROW))

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
    INST_LEAD
} InstrumentType;

// Channel state
typedef struct {
    InstrumentType instrument;
    float freq;
    float phase;
    int samples_left;
    float volume;
    float env_time;
} Channel;

// Pattern note entry
typedef struct {
    InstrumentType inst;
    float freq;
} PatternNote;

// Global state
static SDL_AudioDeviceID audio_device;
static Channel channels[5];  // 5 channels: kick, snare, hihat, bass, lead
static int current_row = 0;
static int current_tick = 0;
static int tick_samples = 0;

// --- WAVEFORM GENERATORS ---

static float sine_wave(float phase) {
    return sinf(phase * 2.0f * M_PI);
}

static float square_wave(float phase) {
    return (phase - floorf(phase)) < 0.5f ? 1.0f : -1.0f;
}

static float sawtooth_wave(float phase) {
    return 2.0f * (phase - floorf(phase)) - 1.0f;
}

static float noise_wave(void) {
    return ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
}

// --- INSTRUMENT GENERATORS ---

static float generate_kick(Channel *ch, float sample_rate) {
    // Kick: pitched sine sweep down with envelope
    float t = ch->env_time;
    float pitch_env = expf(-t * 15.0f);  // Fast pitch decay
    float amp_env = expf(-t * 5.0f);     // Volume decay

    float freq = 60.0f + pitch_env * 100.0f;  // Sweep from ~160Hz to 60Hz
    ch->phase += freq / sample_rate;

    return sine_wave(ch->phase) * amp_env * 0.58f;
}

static float generate_snare(Channel *ch, float sample_rate) {
    // Snare: filtered noise with tone
    float t = ch->env_time;
    float amp_env = expf(-t * 20.0f);  // Fast decay

    // Mix noise with a bit of tone
    float noise = noise_wave() * 0.7f;
    ch->phase += 200.0f / sample_rate;
    float tone = sine_wave(ch->phase) * 0.3f;

    return (noise + tone) * amp_env * 0.25f;
}

static float generate_hihat(Channel *ch, float sample_rate) {
    // Hi-hat: short high-frequency noise burst
    float t = ch->env_time;
    float amp_env = expf(-t * 35.0f);  // Very fast decay

    (void)sample_rate;  // Unused
    return noise_wave() * amp_env * 0.123f;
}

static float generate_bass(Channel *ch, float sample_rate) {
    // Bass: square wave with slight decay
    float t = ch->env_time;
    float amp_env = 1.0f - expf(-t * 10.0f);  // Quick attack
    amp_env *= expf(-t * 2.0f);  // Slow decay

    ch->phase += ch->freq / sample_rate;
    return square_wave(ch->phase) * amp_env * 0.4f;
}

static float generate_lead(Channel *ch, float sample_rate) {
    // Lead: sawtooth + square mix with vibrato for rich synthwave sound
    float t = ch->env_time;
    float amp_env = 1.0f - expf(-t * 15.0f);  // Fast attack
    amp_env *= expf(-t * 0.8f);  // Longer sustain for melody

    // Add subtle vibrato
    float vibrato = sinf(t * 6.0f) * 0.003f;
    ch->phase += (ch->freq * (1.0f + vibrato)) / sample_rate;

    // Mix sawtooth (60%) and square (40%) for richer tone
    float saw = sawtooth_wave(ch->phase) * 0.6f;
    float square = square_wave(ch->phase) * 0.4f;

    return (saw + square) * amp_env * 0.35f;
}

// --- PATTERN DATA (Hardcoded demo track) ---

// Kick pattern – 4 bars, simple 4/4 with a small fill at the end
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

    // Bar 4 (48–63) – same 4/4 plus extra kicks for a mini-fill
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_NONE, 0},       {INST_NONE, 0},
    {INST_KICK, NOTE_C2}, {INST_NONE, 0},       {INST_KICK, NOTE_C2}, {INST_KICK, NOTE_C2}
};


// Snare pattern – empty bar 1, then backbeat on 2 & 4, with a fill at the very end
static const PatternNote snare_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – no snare for a clean intro
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},

    // Bar 2 (16–31) – snare on beats 2 & 4 (rows 20, 28)
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},  {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 3 (32–47) – same backbeat
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},  {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},

    // Bar 4 (48–63) – backbeat + last-bar fill
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_SNARE, 0}, {INST_NONE, 0},
    {INST_NONE, 0}, {INST_NONE, 0}, {INST_NONE, 0},  {INST_NONE, 0},
    {INST_NONE, 0}, {INST_SNARE, 0}, {INST_SNARE, 0}, {INST_NONE, 0}
};


// Hi-hat pattern – light groove, then a more driving last bar
static const PatternNote hihat_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – original pattern
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    // Bar 2 (16–31) – same feel
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    // Bar 3 (32–47) – same feel
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_NONE, 0},  {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0},

    // Bar 4 (48–63) – more driving 8th-note style hats
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_NONE, 0},
    {INST_HIHAT, 0}, {INST_NONE, 0},  {INST_HIHAT, 0}, {INST_HIHAT, 0}
};


// Bass pattern – 4-bar progression with simple variations
static const PatternNote bass_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – original G pattern
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G2}, {INST_BASS, NOTE_G2},
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G2}, {INST_BASS, NOTE_G3},
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G2}, {INST_BASS, NOTE_G2},
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G3}, {INST_BASS, NOTE_G4},

    // Bar 2 (16–31) – same as bar 1 (keeps the groove stable)
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G2}, {INST_BASS, NOTE_G2},
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G2}, {INST_BASS, NOTE_G3},
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G2}, {INST_BASS, NOTE_G2},
    {INST_NONE, 0},      {INST_NONE, NOTE_G2}, {INST_BASS, NOTE_G3}, {INST_BASS, NOTE_G4},

    // Bar 3 (32–47) – same pattern, transposed to F
    {INST_NONE, 0},      {INST_NONE, NOTE_F2}, {INST_BASS, NOTE_F2}, {INST_BASS, NOTE_F2},
    {INST_NONE, 0},      {INST_NONE, NOTE_F2}, {INST_BASS, NOTE_F2}, {INST_BASS, NOTE_F3},
    {INST_NONE, 0},      {INST_NONE, NOTE_F2}, {INST_BASS, NOTE_F2}, {INST_BASS, NOTE_F2},
    {INST_NONE, 0},      {INST_NONE, NOTE_F2}, {INST_BASS, NOTE_F3}, {INST_BASS, NOTE_F4},

    // Bar 4 (48–63) – same pattern, transposed to E
    {INST_NONE, 0},      {INST_NONE, NOTE_E2}, {INST_BASS, NOTE_E2}, {INST_BASS, NOTE_E2},
    {INST_NONE, 0},      {INST_NONE, NOTE_E2}, {INST_BASS, NOTE_E2}, {INST_BASS, NOTE_E3},
    {INST_NONE, 0},      {INST_NONE, NOTE_E2}, {INST_BASS, NOTE_E2}, {INST_BASS, NOTE_E2},
    {INST_NONE, 0},      {INST_NONE, NOTE_E2}, {INST_BASS, NOTE_E3}, {INST_BASS, NOTE_E4}
};


// Lead pattern – 4-bar evolving melody
static const PatternNote lead_pattern[ROWS_PER_PATTERN] = {
    // Bar 1 (0–15) – your original motif
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},

    // Bar 2 (16–31) – variation, reaching up to C5
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C5}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},

    // Bar 3 (32–47) – more lift, up to D5 then back down
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C5}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D5}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C5}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},

    // Bar 4 (48–63) – faster run that resolves back to C4
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_F4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_A4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_G4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_E4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_D4}, {INST_NONE, 0},
    {INST_LEAD, NOTE_C4}, {INST_NONE, 0}
};


// --- TRIGGER NOTE ON CHANNEL ---

static void trigger_note(int channel_idx, InstrumentType inst, float freq) {
    Channel *ch = &channels[channel_idx];
    ch->instrument = inst;
    ch->freq = freq;
    ch->phase = 0.0f;
    ch->env_time = 0.0f;
    ch->samples_left = SAMPLES_PER_TICK * 2;  // Note duration
}

// --- PROCESS PATTERN ROW ---

static void process_row(void) {
    const PatternNote *patterns[5] = {
        kick_pattern,
        snare_pattern,
        hihat_pattern,
        bass_pattern,
        lead_pattern
    };

    for (int i = 0; i < 5; i++) {
        const PatternNote *note = &patterns[i][current_row];
        if (note->inst != INST_NONE) {
            trigger_note(i, note->inst, note->freq);
        }
    }

    current_row = (current_row + 1) % ROWS_PER_PATTERN;
}

// --- GENERATE SAMPLE FOR CHANNEL ---

static float generate_channel_sample(Channel *ch) {
    if (ch->instrument == INST_NONE || ch->samples_left <= 0) {
        return 0.0f;
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
        default:
            break;
    }

    ch->env_time += 1.0f / SAMPLE_RATE;
    ch->samples_left--;

    return sample;
}

// --- SDL AUDIO CALLBACK ---

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;  // Unused

    Sint16 *buffer = (Sint16 *)stream;
    int samples = len / (sizeof(Sint16) * CHANNELS);

    for (int i = 0; i < samples; i++) {
        // Advance sequencer
        if (tick_samples >= SAMPLES_PER_TICK) {
            tick_samples = 0;
            current_tick++;

            if (current_tick >= TICKS_PER_ROW) {
                current_tick = 0;
                process_row();
            }
        }
        tick_samples++;

        // Mix all channels
        float mixed = 0.0f;
        for (int ch = 0; ch < 5; ch++) {
            mixed += generate_channel_sample(&channels[ch]);
        }

        // Clamp and convert to 16-bit
        mixed = fmaxf(-1.0f, fminf(1.0f, mixed));
        Sint16 sample_16 = (Sint16)(mixed * 32767.0f);

        // Write stereo
        buffer[i * 2] = sample_16;      // Left
        buffer[i * 2 + 1] = sample_16;  // Right
    }
}

// --- INITIALIZE SYNTHESIZER ---

void synth_init(void) {
    // Clear channel state
    memset(channels, 0, sizeof(channels));
    current_row = 0;
    current_tick = 0;
    tick_samples = 0;

    // Set up SDL audio
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = CHANNELS;
    want.samples = BUFFER_SIZE;
    want.callback = audio_callback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    if (audio_device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    // Start playback
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
