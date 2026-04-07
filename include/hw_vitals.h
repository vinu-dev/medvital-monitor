/**
 * @file hw_vitals.h
 * @brief Hardware Abstraction Layer (HAL) for vital signs acquisition.
 *
 * @details
 * This interface decouples the GUI from the data source. In production,
 * replace sim_vitals.c with hw_driver.c that reads from real hardware
 * (e.g., serial port, USB medical device, ADC) without changing any
 * other source file.
 *
 * ### Replacement contract
 * Provide a translation unit that defines both functions below:
 * - hw_init()           — called once when the dashboard opens.
 * - hw_get_next_reading() — called periodically (every ~2 s) by WM_TIMER.
 *
 * ### IEC 62304 Traceability
 * - Software Unit: UNIT-HAL
 * - Requirements covered: SWR-GUI-005
 *
 * @version 1.0.0
 * @date    2026-04-07
 * @author  vinu-engineer
 *
 * @copyright Medical Device Software — IEC 62304 Class B compliant.
 *            All rights reserved.
 */

#ifndef HW_VITALS_H
#define HW_VITALS_H

#include "vitals.h"

/**
 * @brief Initialise the vital signs acquisition source.
 *
 * @details Call once before the first hw_get_next_reading() invocation.
 * For the simulation back-end this resets the sequence index to 0.
 * For a real hardware back-end this would open a COM port, start ADC
 * sampling, etc.
 *
 * @req SWR-GUI-005
 */
void hw_init(void);

/**
 * @brief Acquire the next vital signs reading from the data source.
 *
 * @details Writes one VitalSigns sample into *out.  The caller is
 * responsible for adding the result to the PatientRecord via
 * patient_add_reading().
 *
 * For the simulation back-end this cycles through a 20-entry clinical
 * scenario table (STABLE → WARNING → CRITICAL → RECOVERING).
 * For a real hardware back-end this would block briefly while reading
 * sensor values or return the most recent DMA-buffered sample.
 *
 * @param[out] out  Pointer to a caller-allocated VitalSigns structure.
 *                  Must not be NULL.
 *
 * @pre  hw_init() has been called.
 * @pre  out != NULL
 *
 * @req SWR-GUI-005
 */
void hw_get_next_reading(VitalSigns *out);

#endif /* HW_VITALS_H */
