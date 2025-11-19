#ifndef TRANSITIONS_H
#define TRANSITIONS_H

#include "common.h"

// Transition types
typedef enum {
    TRANSITION_CROSSFADE = 0,
    TRANSITION_HORIZONTAL_WIPE,
    TRANSITION_PIXELATE_DISSOLVE,
    TRANSITION_CIRCULAR_WIPE,
    TRANSITION_COUNT
} TransitionType;

// Transition names for display
extern const char *transition_names[TRANSITION_COUNT];

// Apply a transition effect between two frames
// progress: 0.0 (show old_frame) to 1.0 (show new_frame)
void transition_crossfade(pixel_t *dest, const pixel_t *old_frame,
                         const pixel_t *new_frame, float progress);

void transition_horizontal_wipe(pixel_t *dest, const pixel_t *old_frame,
                               const pixel_t *new_frame, float progress);

void transition_pixelate_dissolve(pixel_t *dest, const pixel_t *old_frame,
                                 const pixel_t *new_frame, float progress);

void transition_circular_wipe(pixel_t *dest, const pixel_t *old_frame,
                             const pixel_t *new_frame, float progress);

// Helper: apply current transition type
void transition_apply(TransitionType type, pixel_t *dest,
                     const pixel_t *old_frame, const pixel_t *new_frame,
                     float progress);

#endif // TRANSITIONS_H
