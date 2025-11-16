#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>

// Initialize the synthesizer and start audio playback
void synth_init(void);

// Cleanup and stop audio playback
void synth_cleanup(void);

#endif // SYNTH_H
