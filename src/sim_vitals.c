/**
 * @file sim_vitals.c
 * @brief Simulation back-end for the vital signs HAL (hw_vitals.h).
 *
 * @details
 * Provides hw_init() and hw_get_next_reading() using a fixed 20-entry
 * clinical scenario table that cycles through four phases:
 *
 *   Index  0– 4 : STABLE NORMAL     — resting, all parameters within range
 *   Index  5– 8 : DETERIORATING     — gradual rise into WARNING territory
 *   Index  9–11 : CRITICAL          — life-threatening values
 *   Index 12–18 : RECOVERING        — progressive return to normal
 *   Index 19    : STABLE NORMAL     — cycle repeats
 *
 * To connect real hardware, replace this file with hw_driver.c that
 * implements the same two function signatures without modifying any
 * other source file (see hw_vitals.h for the full contract).
 *
 * All storage is static — no heap allocation (SYS-012).
 *
 * ### IEC 62304 Traceability
 * - Software Unit: UNIT-SIM
 * - Requirements covered: SWR-GUI-006
 *
 * @version 1.0.0
 * @date    2026-04-07
 * @author  vinu-engineer
 *
 * @copyright Medical Device Software — IEC 62304 Class B compliant.
 *            All rights reserved.
 */

#include "hw_vitals.h"

/* =========================================================================
 * Simulation scenario table  (static — no heap)
 * ========================================================================= */

#define SIM_SEQUENCE_LEN 20

/**
 * @brief Fixed 20-reading clinical scenario used by the simulation.
 *
 * Values are representative of realistic clinical deterioration and
 * recovery patterns. They are NOT to be used as diagnostic thresholds.
 */
static const VitalSigns SIM_SEQUENCE[SIM_SEQUENCE_LEN] = {
    /* ---- Phase 1: Stable NORMAL (indices 0–4) ---- */
    { 74, 122,  80, 36.7f, 97 },
    { 72, 118,  78, 36.8f, 98 },
    { 76, 124,  82, 36.6f, 97 },
    { 71, 120,  79, 36.9f, 98 },
    { 73, 122,  80, 36.8f, 97 },

    /* ---- Phase 2: Deteriorating → WARNING (indices 5–8) ---- */
    { 85, 132,  84, 37.1f, 95 },
    { 92, 138,  87, 37.5f, 93 },
    { 98, 142,  91, 37.9f, 92 },
    {105, 148,  94, 38.4f, 91 },

    /* ---- Phase 3: CRITICAL (indices 9–11) ---- */
    {128, 168, 108, 39.2f, 88 },
    {142, 178, 114, 39.8f, 86 },
    {152, 182, 122, 39.9f, 84 },

    /* ---- Phase 4: Recovering (indices 12–18) ---- */
    {136, 172, 108, 39.4f, 87 },
    {118, 158,  96, 38.8f, 91 },
    {104, 144,  90, 38.2f, 93 },
    { 96, 136,  86, 37.6f, 94 },
    { 88, 128,  83, 37.3f, 95 },
    { 80, 124,  82, 37.2f, 96 },
    { 76, 122,  80, 36.9f, 97 },

    /* ---- Back to stable (index 19) — cycle repeats ---- */
    { 73, 120,  78, 36.8f, 98 }
};

/** Current position within SIM_SEQUENCE. */
static int g_sim_index = 0;

/* =========================================================================
 * HAL implementation
 * ========================================================================= */

/**
 * @brief Reset simulation index to the start of the scenario table.
 * @req SWR-GUI-006
 */
void hw_init(void)
{
    g_sim_index = 0;
}

/**
 * @brief Return the next entry from the scenario table, then advance.
 *
 * Wraps back to index 0 after index 19, producing a continuous cycle.
 *
 * @param[out] out  Written with the next VitalSigns sample. Must not be NULL.
 * @req SWR-GUI-006
 */
void hw_get_next_reading(VitalSigns *out)
{
    *out = SIM_SEQUENCE[g_sim_index];
    g_sim_index = (g_sim_index + 1) % SIM_SEQUENCE_LEN;
}
