PAM_LIB_DIR ?= /lib/security
INSTALL ?= install
CFLAGS ?= -O2 -g -Wall -Wformat-security -Wextra -fstack-protector-all

CFLAGS += -fPIC -fvisibility=hidden -DLINUX_PAM -DPAM_DYNAMIC
LDFLAGS += -Wl,-x -Wl,-z,relro -Wl,-z,now -shared

TITLE = pam_setquota
LIBSHARED = $(TITLE).so
LDLIBS = -lpam
LIBOBJ = $(TITLE).o

CC ?= cc
LD ?= ld
RM ?= rm

all: $(LIBSHARED)

$(LIBSHARED): $(LIBOBJ)
	$(CC) $(LDFLAGS) $(LIBOBJ) $(LDLIBS) -o $@

pam_setquota.o: pam_setquota.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

install: $(LIBSHARED)
	$(INSTALL) -m 0755 -d $(DESTDIR)$(PAM_LIB_DIR)
	$(INSTALL) -m 0644 $(LIBSHARED) $(DESTDIR)$(PAM_LIB_DIR)

clean:
	$(RM) *.o *.so

