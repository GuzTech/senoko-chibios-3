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
#include "chprintf.h"

#include "bionic.h"
#include "senoko.h"
#include "power.h"
#include "shell.h"

void cmd_power(BaseSequentialStream *chp, int argc, char *argv[])
{
  if ((argc > 0) && !strcasecmp(argv[0], "on")) {
    chprintf(chp, "Powering on... ");
    powerOn();
    chprintf(chp, "Ok\r\n");
  }
  else if ((argc > 0) && !strcasecmp(argv[0], "off")) {
    chprintf(chp, "Powering off... ");
    powerOff();
    chprintf(chp, "Ok\r\n");
  }
  else if ((argc > 0)) {
    chprintf(chp, "Usage: power [on|off]\r\n");
  }
  else {
    chprintf(chp, "Power status: %s\r\n", powerIsOn() ? "on" : "off");
  }
  chprintf(chp, "\r\n");
}
