ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = generate-gitfiles.sh

bin_PROGRAMS      = watchdogd
watchdogd_SOURCES = main.c wdt.h wdt.c drivers.c itusbwdt.c
watchdogd_CFLAGS  = -Wall -W ${USB_CFLAGS}
watchdogd_LDADD   = ${USB_LIBS}

dist-hook:
	sh $(top_srcdir)/generate-gitfiles.sh $(top_srcdir) $(distdir)
