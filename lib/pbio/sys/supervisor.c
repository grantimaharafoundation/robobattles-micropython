// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// System Supervisor

// This file monitors the system health and takes care of things like shutting
// down power on low battery an shutting down motors on overcurrent conditions.

#include <stdbool.h>

#include <pbdrv/watchdog.h>
#include <pbio/config.h>
#include <pbsys/status.h>

#include <pbdrv/bluetooth.h>

#if PBIO_CONFIG_HUB_TECHNIC_HUB
#include <pbdrv/reset.h>

#define SHUTDOWN_REQUEST_FALLBACK_MS (10000)
#endif

/**
 * Polls the system supervisor.
 *
 * This is called periodically to handle any changes in the system state.
 */
void pbsys_supervisor_poll(void) {
    // keep the hub from resetting itself
    pbdrv_watchdog_update();

    // Shut down on low voltage so we don't damage rechargeable batteries.
    bool low_battery_shutdown = pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN, true, 3000);

    // If not connected to a host, shut down after several minutes of not
    // running any program.
    bool idle_shutdown = !pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_LE) &&
        pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING, false, 3 * 60000);

    if (low_battery_shutdown || idle_shutdown) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
    }

    #if PBIO_CONFIG_HUB_TECHNIC_HUB
    if (!pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN) &&
        pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST, true, SHUTDOWN_REQUEST_FALLBACK_MS)) {
        pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
    }
    #endif
}
