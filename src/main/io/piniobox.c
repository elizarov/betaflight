/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#include "platform.h"

#ifdef USE_PINIOBOX

#include "build/debug.h"

#include "common/time.h"
#include "common/utils.h"

#include "msp/msp_box.h"

#include "pg/pinio.h"
#include "pg/piniobox.h"

#include "scheduler/scheduler.h"

#include "piniobox.h"

#define PULSE_DURATION_US 100000 // pulse duration is configured in 100 ms increments

typedef enum { PULSE_NONE, PULSE_ACTIVE, PULSE_DONE } pulseState_e;

typedef struct pinioBoxRuntimeConfig_s {
    uint8_t boxId;
    uint8_t pulse; // pulse duration x 100 ms
    pulseState_e pulseState; // state of the pulse
    timeUs_t pulseStartTimeUs; // time when pulse was activated
} pinioBoxRuntimeConfig_t;

static pinioBoxRuntimeConfig_t pinioBoxRuntimeConfig[PINIO_COUNT];

void pinioBoxInit(const pinioBoxConfig_t *pinioBoxConfig, const pinioBoxPulseConfig_t *pinioBoxPulseConfig)
{
    // Convert permanentId to boxId and extract pulse configuration
    for (int i = 0; i < PINIO_COUNT; i++) {
        const box_t *box = findBoxByPermanentId(pinioBoxConfig->permanentId[i]);
        pinioBoxRuntimeConfig[i].boxId = box ? box->boxId : BOXID_NONE;
        pinioBoxRuntimeConfig[i].pulse = pinioBoxPulseConfig->pulse[i];
    }
}

void pinioBoxUpdate(timeUs_t currentTimeUs)
{
    for (int i = 0; i < PINIO_COUNT; i++) {
        uint8_t boxId = pinioBoxRuntimeConfig[i].boxId;
        if (boxId == BOXID_NONE) continue; // nothing to do for this one -- not configured
        bool desiredOn = getBoxIdState(boxId); // desired to turn "on"
        if (pinioBoxRuntimeConfig[i].pulse) {
            // process pulse and adjust desiredOn value according to pulse timing
            switch (pinioBoxRuntimeConfig[i].pulseState) {
                case PULSE_NONE:
                    // activate pulse if desired
                    if (desiredOn) {
                        pinioBoxRuntimeConfig[i].pulseState = PULSE_ACTIVE;
                        pinioBoxRuntimeConfig[i].pulseStartTimeUs = currentTimeUs;
                    }
                    break;
                case PULSE_ACTIVE:
                    // turn off only when the pulse is over
                    desiredOn = cmpTimeUs(currentTimeUs, pinioBoxRuntimeConfig[i].pulseStartTimeUs) <
                                (timeDelta_t) pinioBoxRuntimeConfig[i].pulse * PULSE_DURATION_US;
                    if (!desiredOn) {
                        pinioBoxRuntimeConfig[i].pulseState = PULSE_DONE;
                    }
                    break;
                case PULSE_DONE:
                    if (desiredOn) {
                        desiredOn = false; // pulse is over!
                    } else {
                        // reset pulse logic when it is no longer desired
                        pinioBoxRuntimeConfig[i].pulseState = PULSE_NONE;
                    }
            }
        }
        pinioSet(i, desiredOn);
    }
}

void pinioBoxTaskControl(void)
{
    bool enableTask = false;
    for (int i = 0; i < PINIO_COUNT; i++) {
        uint8_t boxId = pinioBoxRuntimeConfig[i].boxId;
        if (boxId != BOXID_NONE && isModeActivationConditionPresent(boxId)) {
            enableTask = true;
            break;
        }
    }
    setTaskEnabled(TASK_PINIOBOX, enableTask);
}
#endif
