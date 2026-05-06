#include "gui_status_duration.h"

#include <stdio.h>

void gui_status_duration_reset(GuiStatusDurationState *state)
{
    if (state == NULL) return;

    state->tracked_level = ALERT_NORMAL;
    state->entered_ms    = 0;
    state->active        = 0;
}

void gui_status_duration_update(GuiStatusDurationState *state,
                                int monitoring_live,
                                int has_patient,
                                AlertLevel current_level,
                                uint64_t now_ms)
{
    if (state == NULL) return;

    if (!monitoring_live || !has_patient || current_level == ALERT_NORMAL) {
        gui_status_duration_reset(state);
        return;
    }

    if (!state->active || state->tracked_level != current_level || now_ms < state->entered_ms) {
        state->tracked_level = current_level;
        state->entered_ms    = now_ms;
        state->active        = 1;
    }
}

int gui_status_duration_is_active(const GuiStatusDurationState *state)
{
    return state != NULL && state->active;
}

uint64_t gui_status_duration_elapsed_seconds(const GuiStatusDurationState *state,
                                             uint64_t now_ms)
{
    if (!gui_status_duration_is_active(state) || now_ms < state->entered_ms) {
        return 0;
    }

    return (now_ms - state->entered_ms) / 1000ULL;
}

int gui_status_duration_format(uint64_t elapsed_seconds, char *out, size_t out_len)
{
    unsigned long long hours;
    unsigned long long minutes;
    unsigned long long seconds;
    int written;

    if (out == NULL || out_len == 0) return 0;

    hours   = elapsed_seconds / 3600ULL;
    minutes = (elapsed_seconds % 3600ULL) / 60ULL;
    seconds = elapsed_seconds % 60ULL;

    if (hours > 0ULL) {
        written = snprintf(out, out_len, "%02llu:%02llu:%02llu",
                           hours, minutes, seconds);
    } else {
        unsigned long long total_minutes = elapsed_seconds / 60ULL;
        written = snprintf(out, out_len, "%02llu:%02llu",
                           total_minutes, seconds);
    }

    return written > 0 && (size_t)written < out_len;
}
