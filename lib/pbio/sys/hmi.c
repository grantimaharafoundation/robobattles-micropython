// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user.

// TODO: implement additional buttons and menu system (via matrix display) for SPIKE Prime
// TODO: implement additional buttons and menu system (via screen) for NXT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/clock.h>
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

#define POWER_BUTTON_LONG_PRESS_MS (2000)
#define POWER_BUTTON_DOUBLE_TAP_MS (500)

// The selected slot is not persistent across reboot, so that the first slot
// is always active on boot. This allows consistently starting programs without
// visibility of the display.
static uint8_t selected_slot = 0;

// Active button press was a long press
static bool long_pressed = false;

// Power button has been released at some point during this hub cycle.
static bool power_button_released_this_cycle = false;

// Current button press tracking for short press, double tap, and long press.
static bool power_button_was_pressed = false;
static bool power_button_press_started_after_release = false;

// First tap waiting to see if it becomes a double tap.
static bool power_button_tap_pending = false;
static uint32_t power_button_tap_pending_time;

static void pbsys_hmi_handle_power_button_tap(uint32_t now) {
    if (power_button_tap_pending && now - power_button_tap_pending_time <= POWER_BUTTON_DOUBLE_TAP_MS) {
        power_button_tap_pending = false;
        pbsys_main_set_hub_controller_pairing_mode(false);

        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
            // Double tap enters computer/Bluetooth mode by stopping the program.
            #if PBSYS_CONFIG_PROGRAM_STOP
            pbsys_program_stop(false);
            #endif
        } else {
            // Double tap attempts to start running the program.
            pbsys_main_program_request_start(selected_slot, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);
        }
        return;
    }

    power_button_tap_pending = true;
    power_button_tap_pending_time = now;
}

static void pbsys_hmi_handle_power_button_tap_timeout(uint32_t now) {
    if (!power_button_tap_pending || now - power_button_tap_pending_time <= POWER_BUTTON_DOUBLE_TAP_MS) {
        return;
    }

    power_button_tap_pending = false;
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
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
    uint32_t now = pbdrv_clock_get_ms();

    // Bluetooth is always "on." "Bluetooth mode" (blinking light) just means a program is not running.

    if (pbio_button_is_pressed(&btn) == PBIO_SUCCESS) {
        bool program_running = pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);

        if (!program_running) {
            pbsys_main_set_hub_controller_pairing_mode(false);
        }

        bool power_button_pressed = btn & PBIO_BUTTON_CENTER;

        if (power_button_pressed) {
            if (!power_button_was_pressed) {
                // Ignore a press already held during power-on; only presses
                // that begin after a release can count as taps or long presses.
                power_button_press_started_after_release = power_button_released_this_cycle;
                power_button_was_pressed = true;
            }

            pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);

            // Take action when button is held down for 2 seconds, but not if it's still being held from power-on press
            if (power_button_press_started_after_release && pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, POWER_BUTTON_LONG_PRESS_MS)) {
                // Long press completed
                if (!long_pressed) {
                    power_button_tap_pending = false;

                    if (program_running) {
                        // Explicitly allow pairing-mode controllers.
                        pbsys_main_set_hub_controller_pairing_mode(true);
                    }
                }
                long_pressed = true;
            }
        } else {
            if (power_button_was_pressed && power_button_press_started_after_release && !long_pressed) {
                pbsys_hmi_handle_power_button_tap(now);
            }

            pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
            pbsys_hmi_handle_power_button_tap_timeout(now);

            power_button_was_pressed = false;
            power_button_press_started_after_release = false;
            long_pressed = false;
            power_button_released_this_cycle = true;
        }
    }

    pbsys_status_light_poll();
}
