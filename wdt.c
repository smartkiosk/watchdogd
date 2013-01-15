#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include "wdt.h"

struct wdt {
  const wdt_driver_t *driver;
  void *driver_data;
};

static const wdt_driver_t *find_driver(const char *name);

wdt_t *wdt_open(const char *driver, const char *parameter) {
  wdt_t *wdt = malloc(sizeof(wdt_t));
  if(wdt == NULL)
    return NULL;

  wdt->driver = find_driver(driver);
  if(wdt->driver == NULL) {
    syslog(LOG_ERR, "unsupported device '%s'", driver);

    free(wdt);

    return NULL;
  }

  wdt->driver_data = wdt->driver->open(parameter);
  if(wdt->driver_data == NULL) {
    free(wdt);

    return NULL;
  }

  return wdt;
}

void wdt_close(wdt_t *wdt) {
  wdt->driver->close(wdt->driver_data);
  free(wdt);
}

int wdt_enable(wdt_t *wdt) {
  return wdt->driver->set_enabled(wdt->driver_data, 1);
}

int wdt_disable(wdt_t *wdt) {
  return wdt->driver->set_enabled(wdt->driver_data, 0);
}

int wdt_enable_modem(wdt_t *wdt) {
  return wdt->driver->set_modem_power(wdt->driver_data, 1);
}

int wdt_disable_modem(wdt_t *wdt) {
  return wdt->driver->set_modem_power(wdt->driver_data, 0);
}

int wdt_feed(wdt_t *wdt) {
  return wdt->driver->feed(wdt->driver_data);
}

int wdt_set_feed_interval(wdt_t *wdt, int interval) {
  return wdt->driver->set_feed_interval(wdt->driver_data, interval);
}

int wdt_get_feed_interval(wdt_t *wdt, int *interval) {
  return wdt->driver->get_feed_interval(wdt->driver_data, interval);
}

static const wdt_driver_t *find_driver(const char *name) {
  int i;

  for(i = 0; wdt_drivers[i] != NULL; i++)
    if(strcmp(name, wdt_drivers[i]->name) == 0)
      return wdt_drivers[i];

  return NULL;
}
