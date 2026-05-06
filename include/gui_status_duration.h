#ifndef GUI_STATUS_DURATION_H
#define GUI_STATUS_DURATION_H

#include <stddef.h>
#include <stdint.h>

#include "vitals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    AlertLevel tracked_level;
    uint64_t   entered_ms;
    int        active;
} GuiStatusDurationState;

void gui_status_duration_reset(GuiStatusDurationState *state);
void gui_status_duration_update(GuiStatusDurationState *state,
                                int monitoring_live,
                                int has_patient,
                                AlertLevel current_level,
                                uint64_t now_ms);
int gui_status_duration_is_active(const GuiStatusDurationState *state);
uint64_t gui_status_duration_elapsed_seconds(const GuiStatusDurationState *state,
                                             uint64_t now_ms);
int gui_status_duration_format(uint64_t elapsed_seconds, char *out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* GUI_STATUS_DURATION_H */
