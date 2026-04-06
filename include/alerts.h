#ifndef ALERTS_H
#define ALERTS_H

/* ---------------------------------------------------------------
 * Alert generation — produces human-readable alert records
 * from a set of vital signs.
 * REQ-ALT: For every parameter outside the normal range an
 *          Alert record shall be generated with the correct
 *          AlertLevel and a descriptive message.
 * --------------------------------------------------------------- */

#include "vitals.h"

#define MAX_ALERTS    5
#define ALERT_MSG_LEN 96

typedef struct {
    AlertLevel level;
    char       parameter[32];
    char       message[ALERT_MSG_LEN];
} Alert;

/*
 * Fills `out` with one Alert per out-of-range parameter.
 * Returns the number of alerts written (0 = all normal).
 * At most `max_out` entries are written.
 */
int generate_alerts(const VitalSigns *vitals, Alert *out, int max_out);

#endif /* ALERTS_H */
