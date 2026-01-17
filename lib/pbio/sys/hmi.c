// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement additional buttons and menu system (via matrix display) for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/core.h>
#include <pbdrv/reset.h>
#include <pbdrv/led.h>
#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/event.h>
#include <pbio/light.h>
#include <pbsys/config.h>
#include <pbsys/main.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>
#include <pbsys/storage_settings.h>

#include "light_matrix.h"
#include "light.h"

static struct pt update_program_run_button_wait_state_pt;

// The selected slot is not persistent across reboot, so that the first slot
// is always active on boot. This allows consistently starting programs without
// visibility of the display.
static uint8_t selected_slot = 0;

// Active button press was a long press
static bool long_pressed = false;

/**
 * Protothread to monitor the button state to trigger starting the user program.
 * @param [in]  button_pressed      The current button state.
 */
static PT_THREAD(update_program_run_button_wait_state(bool button_pressed)) {
    struct pt *pt = &update_program_run_button_wait_state_pt;

    // This should not be active if a long press has happened
    if (long_pressed) {
        PT_EXIT(pt);
    }

    PT_BEGIN(pt);

    for (;;) {
        // button may still be pressed from power on or user program stop
        PT_WAIT_UNTIL(pt, !button_pressed);
        PT_WAIT_UNTIL(pt, button_pressed);
        PT_WAIT_UNTIL(pt, !button_pressed);

        // Short press completed
        pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
    }

    PT_END(pt);
}

#if PBSYS_CONFIG_HMI_NUM_SLOTS

/**
 * Gets the currently selected program slot.
 *
 * @return The currently selected program slot (zero-indexed).
 */
uint8_t pbsys_hmi_get_selected_program_slot(void) {
    return selected_slot;
}

#endif // PBSYS_CONFIG_HMI_NUM_SLOTS

void pbsys_hmi_init(void) {
    pbsys_status_light_init();
    pbsys_hub_light_matrix_init();
    PT_INIT(&update_program_run_button_wait_state_pt);
}

void pbsys_hmi_handle_event(process_event_t event, process_data_t data) {
    pbsys_status_light_handle_event(event, data);
    pbsys_hub_light_matrix_handle_event(event, data);

    #if PBSYS_CONFIG_BATTERY_CHARGER
    // On the Technic Large hub, USB can keep the power on even though we are
    // "shutdown", so if the button is pressed again, we reset to turn back on
    if (
        pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)
        && event == PBIO_EVENT_STATUS_SET
        && (pbio_pybricks_status_t)data == PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED
        ) {
        pbdrv_reset(PBDRV_RESET_ACTION_RESET);
    }
    #endif // PBSYS_CONFIG_BATTERY_CHARGER
}

/**
 * Polls the HMI.
 *
 * This is called periodically to update the current HMI state.
 */
void pbsys_hmi_poll(void) {
    pbio_button_flags_t btn;

    // Bluetooth is always "on." "Bluetooth mode" (blinking light) just means a program is not running.

    if (pbio_button_is_pressed(&btn) == PBIO_SUCCESS) {
        if (btn & PBIO_BUTTON_CENTER) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            update_program_run_button_wait_state(true);

            // Take action when button is held down for 2 seconds
            if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, 2000)) {
                // Long press completed
                if (!long_pressed) {
                    // Stop program if currently running. This puts hub in bluetooth mode.
                    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
                        #if PBSYS_CONFIG_PROGRAM_STOP
                        pbsys_program_stop(false);
                        #endif
                    } else {
                        // Attempt to start program if not currently running
                        pbsys_main_program_request_start(selected_slot, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);
                    }
                }
                long_pressed = true;
            }
        } else {
            pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            update_program_run_button_wait_state(false);
            long_pressed = false;
        }
    }

    pbsys_status_light_poll();
}
