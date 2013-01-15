#include <stdlib.h>
#include <libusb.h>
#include <syslog.h>
#include <stdint.h>

#include "wdt.h"

typedef struct WDT_PORT_INFO {
  uint8_t porta;
  uint8_t portb;
  uint8_t portc;
  uint8_t valid_ports;
} WDT_PORT_INFO;

#define WDT_PORTA   (1 << 0)
#define WDT_PORTB   (1 << 1)
#define WDT_PORTC   (1 << 2)

#define WDT_VID 0x03EB
#define WDT_PID 0x2006

#define WDT_COMMAND_GET_PORTS     7
#define WDT_COMMAND_READ_EEPROM   8
#define WDT_COMMAND_WRITE_EEPROM  9
#define WDT_COMMAND_WATCHDOG_OFF  100
#define WDT_COMMAND_WATCHDOG_ON   101
#define WDT_COMMAND_RELAY_OFF     110
#define WDT_COMMAND_RELAY_ON      111
#define WDT_COMMAND_RESET_OFF     120
#define WDT_COMMAND_RESET_ON      121
#define WDT_COMMAND_WATCHDOG_FEED 140

typedef struct itusbwdt {
  libusb_context *context;
  libusb_device_handle *handle;
} itusbwdt_t;

static int itusbwdt_command(itusbwdt_t *wdt,
                            uint8_t command, uint16_t param1, uint16_t param2,
                            void *buffer, size_t bytes) {

  unsigned char direction;

  if(buffer)
    direction = LIBUSB_ENDPOINT_IN;
  else
    direction = LIBUSB_ENDPOINT_OUT;

  return libusb_control_transfer(wdt->handle,
                                 LIBUSB_TRANSFER_TYPE_CONTROL |
                                 LIBUSB_RECIPIENT_ENDPOINT |
                                 LIBUSB_REQUEST_TYPE_VENDOR |
                                 direction,
                                 command,
                                 param1,
                                 param2,
                                 buffer,
                                 bytes,
                                 100);
}


static void *itusbwdt_open(const char *parameter) {
  (void) parameter;

  itusbwdt_t *wdt = malloc(sizeof(itusbwdt_t));
  if(wdt == NULL)
    return NULL;

  int ret = libusb_init(&wdt->context);
  if(ret != 0) {
    syslog(LOG_ERR, "libusb_init failed: %d", ret);

    free(wdt);

    return NULL;
  }

  wdt->handle = libusb_open_device_with_vid_pid(wdt->context, WDT_VID, WDT_PID);
  if(wdt->handle == NULL) {
    syslog(LOG_ERR, "libusb_open_device_with_vid_pid failed");

    libusb_exit(wdt->context);
    free(wdt);

    return NULL;
  }

  return wdt;
}

static void itusbwdt_close(void *data) {
  itusbwdt_t *wdt = data;

  libusb_close(wdt->handle);
  libusb_exit(wdt->context);
  free(wdt);
}

static int itusbwdt_set_enabled(void *data, int enabled) {
  return itusbwdt_command(data,
                          enabled ? WDT_COMMAND_WATCHDOG_ON :
                                    WDT_COMMAND_WATCHDOG_OFF,
                          0, 0, NULL, 0);
}

static int itusbwdt_set_modem_power(void *data, int on) {
  return itusbwdt_command(data,
                          on ? WDT_COMMAND_RELAY_OFF :
                               WDT_COMMAND_RELAY_ON,
                          0, 0, NULL, 0);
}

static int itusbwdt_feed(void *data) {
  return itusbwdt_command(data, WDT_COMMAND_WATCHDOG_FEED, 0, 0, NULL, 0);
}

static int itusbwdt_get_feed_interval(void *data, int *interval) {
  (void) data;

  *interval = 60;

  return 0;
}

static int itusbwdt_set_feed_interval(void *data, int interval) {
  (void) data;
  (void) interval;

  /* unsupported by hardware */

  return 0;
}

const wdt_driver_t itusbwdt = {
  .name = "itusbwdt",
  .open = itusbwdt_open,
  .close = itusbwdt_close,
  .set_enabled = itusbwdt_set_enabled,
  .set_modem_power = itusbwdt_set_modem_power,
  .feed = itusbwdt_feed,
  .get_feed_interval = itusbwdt_get_feed_interval,
  .set_feed_interval = itusbwdt_set_feed_interval,
};
