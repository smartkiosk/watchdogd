#ifndef __WDT__H__
#define __WDT__H__

typedef struct wdt wdt_t;
typedef struct wdt_driver wdt_driver_t;

struct wdt_driver {
  const char *name;

  void *(*open)(const char *parameter);
  void (*close)(void *data);
  int (*set_enabled)(void *data, int enabled);
  int (*set_modem_power)(void *data, int on);
  int (*feed)(void *data);
  int (*set_feed_interval)(void *data, int interval);
  int (*get_feed_interval)(void *data, int *interval);
};

wdt_t *wdt_open(const char *driver, const char *parameter);
void wdt_close(wdt_t *wdt);

int wdt_enable(wdt_t *wdt);
int wdt_disable(wdt_t *wdt);
int wdt_feed(wdt_t *wdt);
int wdt_set_feed_interval(wdt_t *wdt, int interval);
int wdt_get_feed_interval(wdt_t *wdt, int *interval);

int wdt_enable_modem(wdt_t *wdt);
int wdt_disable_modem(wdt_t *wdt);

extern const wdt_driver_t *wdt_drivers[];

#endif
