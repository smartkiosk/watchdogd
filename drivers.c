#include "wdt.h"

extern const wdt_driver_t itusbwdt;

const wdt_driver_t *wdt_drivers[] = {
  &itusbwdt,
  0
};
