#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "wdt.h"

static void version(void) {
  puts(PACKAGE_STRING);
}

static void usage(void) {
  puts("Smartkiosk watchdog control daemon.\n"
       "Usage: watchdogd [OPTION...] <DRIVER> [DEVICE]\n"
       "\n"
       "Options:\n"
       " -d            Run as daemon.\n"
       " -p <FILENAME> Write PID to the specified file.\n"
       " -h            Print this message and exit.\n"
       " -v            Print version and exit.\n"
       " -i            Use specified interval, in seconds (default: 60).\n"
       "\n"
       PACKAGE_STRING ". Report bugs to <" PACKAGE_BUGREPORT ">");
}

static int parse_opts(int argc, char **argv, int *daemon, char **pidfile,
                      int *interval) {
  int ch;

  *daemon = 0;
  *pidfile = 0;
  *interval = 60;

  while((ch = getopt(argc, argv, "dp:hvi:")) != -1) {
    switch(ch) {
      case 'd':
        *daemon = 1;

        break;

      case 'p':
        *pidfile = optarg;

        break;

      case 'h':
        usage();

        return 0;

      case 'v':
        version();

        break;

      case 'i':
        *interval = atoi(optarg);

        break;

      case '?':
      case ':':
        fprintf(stderr, "Try '%s' -h.\n", argv[0]);

        return 0;
    }
  }

  return 1;
}

static int daemonize(void) {
  pid_t pid = fork();
  if(pid == -1)
    return 0;

  if(pid > 0)
    _exit(0);

  setsid();
  umask(0);
  chdir("/");
  int null = open("/dev/null", O_RDWR);
  dup2(STDIN_FILENO, null);
  dup2(STDOUT_FILENO, null);
  dup2(STDERR_FILENO, null);
  close(null);

  pid = fork();
  if(pid == -1)
    return 0;

  if(pid > 0)
    _exit(0);

  return 1;
}

int main(int argc, char *argv[]) {
  int daemon;
  int interval;
  char *pidfile;
  if(!parse_opts(argc, argv, &daemon, &pidfile, &interval))
    return 1;

  if(daemon) {
    if(!daemonize())
      return 1;

    openlog("watchdogd", LOG_PID, LOG_DAEMON);

  } else {
    openlog("watchdogd", LOG_PERROR | LOG_PID, LOG_DAEMON);
  }

  if(pidfile) {
    FILE *file = fopen(pidfile, "w");
    if(file == NULL) {
      syslog(LOG_CRIT, "unable to create pidfile: %s", strerror(errno));

      return 1;
    }

    fprintf(file, "%d\n", getpid());
    fclose(file);
  }

  if(optind == argc) {
    syslog(LOG_ERR, "driver must be specified");

    return 1;
  }

  char *driver = argv[optind], *parameter = NULL;

  if(optind + 1 > argc)
    parameter = argv[optind + 1];

  wdt_t *wdt = wdt_open(driver, parameter);
  if(wdt == NULL) {
    syslog(LOG_ERR, "unable to open watchdog");

    return 1;
  }

  syslog(LOG_INFO, "watchdog open");

  wdt_set_feed_interval(wdt, interval);
  wdt_get_feed_interval(wdt, &interval);

  syslog(LOG_INFO, "interval: %d seconds", interval);

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGQUIT);
  sigaddset(&set, SIGALRM);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGUSR1);

  sigprocmask(SIG_BLOCK, &set, NULL);

  int fd = signalfd(-1, &set, 0);
  if(fd == -1) {
    syslog(LOG_ERR, "signalfd failed: %s", strerror(errno));

    return 1;
  }

  struct itimerval ivalue;
  ivalue.it_interval.tv_sec = interval;
  ivalue.it_interval.tv_usec = 0;
  ivalue.it_value.tv_sec = interval;
  ivalue.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &ivalue, NULL);

  wdt_enable(wdt);
  wdt_feed(wdt);

  while(1) {
    struct signalfd_siginfo info;
    ssize_t bytes = read(fd, &info, sizeof(info));

    if(bytes == -1 && errno == EINTR)
      continue;

    if(bytes != sizeof(info)) {
      syslog(LOG_ERR, "unexpected number of bytes read: %ld", bytes);

      break;
    }

    switch(info.ssi_signo) {
      case SIGUSR1:
        syslog(LOG_WARNING, "resetting modem");
        wdt_disable_modem(wdt);
        sleep(1);
        wdt_enable_modem(wdt);

        break;

      case SIGALRM:
        wdt_feed(wdt);

        break;

      default:
        goto break_outer;
    }

  }
break_outer:

  wdt_disable(wdt);
  wdt_close(wdt);

  syslog(LOG_INFO, "watchdog closed");

  return 0;
}
