/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "uart.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c.h"
#include "iwdg.h"

#include "senoko.h"
#include "senoko-events.h"
#include "senoko-i2c.h"
#include "senoko-shell.h"
#include "senoko-slave.h"
#include "senoko-wdt.h"

#include "chg.h"
#include "power.h"
#include "gg.h"

static void shell_termination_handler(eventid_t id) {
  static int i = 1;
  (void)id;

  chprintf(stream, "\r\nRespawning shell (shell #%d)\r\n", ++i);
  senokoShellRestart();
}

static int pb_is_armed;
static virtual_timer_t release_vt;

static void release_powerbutton(void *arg) {
  (void)arg;

  if (pb_is_armed) {
    pb_is_armed = 0;
    if (powerIsOff())
      powerOn();
    else
      powerOff();
  }
}

static void power_button_pressed_handler(eventid_t id) {
  (void)id;

  /* If we're powered off, power on.
   * Otherwise, wait 4 seconds.  If the button is released, suspend.  Otherwise,
   * hard-cut poweroff.
   */
  pb_is_armed = 1;
  if (powerIsOff()) {
    chprintf(stream, " [Poweron Wait] ");
    chVTSet(&release_vt, MS2ST(200), release_powerbutton, NULL);
  }
  else {
    chprintf(stream, " [Poweroff Wait] ");
    chVTSet(&release_vt, S2ST(3), release_powerbutton, NULL);
  }
}

static void power_button_released_handler(eventid_t id) {
  (void)id;
  if (pb_is_armed) {
    chprintf(stream, " [Suspending] ");
    pb_is_armed = 0;
  }
  else {
    chprintf(stream, " [Already off] ");
    pb_is_armed = 0;
  }
}

static void ac_unplugged_handler(eventid_t id) {
  (void)id;
  chprintf(stream, " [AC unplugged] ");
}

static void ac_plugged_handler(eventid_t id) {
  (void)id;
  chprintf(stream, " [AC plugged] ");
}

static evhandler_t event_handlers[] = {
  shell_termination_handler,
  power_button_pressed_handler,
  power_button_released_handler,
  ac_unplugged_handler,
  ac_plugged_handler,
};

static event_listener_t event_listeners[ARRAY_SIZE(event_handlers)];

static const char *files[] = {
 /*  0 */  "unknown",
 /*  1 */  "senoko-i2c.c",
 /*  2 */  "i2c.c",
 /*  3 */  "i2c-lld.c",
 /*  4 */  "chmtx.c",
 /*  5 */  "chschd.c",
 /*  6 */  "chthread.c",
};

/*
 * Application entry point.
 */
int main(void) {
  uint32_t pre_pc, pre_thr;
  uint32_t cur_pc, cur_thr;
  uint32_t crashln, crashfil;
  uint32_t reason;

  cur_pc  = (((*((uint32_t *)(0x40006c00 + 0x04))) & 0xffff) << 0) |
            0x08000000;
  cur_thr = (((*((uint32_t *)(0x40006c00 + 0x08))) & 0xffff) << 0) |
            0x20000000;
  pre_pc  = (((*((uint32_t *)(0x40006c00 + 0x0c))) & 0xffff) << 0) |
            0x08000000;
  pre_thr = (((*((uint32_t *)(0x40006c00 + 0x10))) & 0xffff) << 0) |
            0x20000000;

  crashln = (((*((uint32_t *)(0x40006c00 + 0x24))) & 0xffff));
  crashfil= (((*((uint32_t *)(0x40006c00 + 0x28))) & 0xffff));
  reason  = (((*((uint32_t *)(0x40006c00 + 0x14))) & 0xffff) << 0) |
            0x08000000;
  if (crashfil > sizeof(files) / sizeof(*files))
	  crashfil = 0;

  *((uint32_t *)(0x40006c00 + 0x14)) = 0;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /* Set up I2C early, to prevent conflicting with the RAM DDC.*/
  senokoI2cInit();

  /* Set up the various I2C slave handlers.*/
  senokoSlaveInit();

  /* Start serial, so we can get status output.*/
  senokoShellInit();
  chEvtRegister(&shell_terminated, &event_listeners[0], 0);

  /* Listen to GPIO events (e.g. button presses, status changes).*/
  senokoEventsInit();
  chEvtRegister(&power_button_pressed, &event_listeners[1], 1);
  chEvtRegister(&power_button_released, &event_listeners[2], 2);
  chEvtRegister(&ac_unplugged, &event_listeners[3], 3);
  chEvtRegister(&ac_plugged, &event_listeners[4], 4);

  chprintf(stream, "\r\nStarting Senoko (Ver %d.%d, git version %s)\r\n", 
      SENOKO_OS_VERSION_MAJOR,
      SENOKO_OS_VERSION_MINOR,
      gitversion);
  chprintf(stream, "Possibly crashed in %s:%d\r\n", files[crashfil], crashln);
  if (reason)
	  chprintf(stream, "Assertion on last boot: %s\r\n", (char *)reason);
  chprintf(stream, "Before reboot, PC was at: 0x%08x in thread 0x%08x\r\n",
                   cur_pc, cur_thr);
  chprintf(stream, "Previous thread PC was at: 0x%08x in thread 0x%08x\r\n",
                   pre_pc, pre_thr);

  /* Start the Senoko watchdog timer thread.*/
  senokoWatchdogInit();

  /* Turn on the charger (and start charging, if necessary).*/
  chgInit();

  /* Turn on mainboard and synchronize power state.*/
  powerInit();

  /* Power up gas gauge.*/
  ggInit();

  chThdSetPriority(LOWPRIO + 10);
  senokoShellRestart();
  while (TRUE)
    chEvtDispatch(event_handlers, chEvtWaitOne(ALL_EVENTS));

  return 0;
}

void senokoHandleHalt(const char *reason) {
  extern void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]);
  chprintf(stream, "\r\n\r\nSystem halt!\r\n");
  chprintf(stream, "Reason: %s\r\n", reason);
  chprintf(stream, "Threads:\r\n");
  cmd_threads(stream, 0, NULL);
  chprintf(stream, "System will reboot now\r\n");
  int i;
  for (i = 32; i < 128; i++)
    chprintf(stream, "%c", i);
}
