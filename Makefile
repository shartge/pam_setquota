CC ?= cc
LD ?= ld

pam_setquota.so: pam_setquota.o
	${LD} -x -z relro -z now -shared -o $@ $<

pam_setquota.o: pam_setquota.c
	${CC} -Os -fPIC -DLINUX_PAM -DPAM_DYNAMIC -D_FORTIFY_SOURCE=2 \
		-Wall -Wextra -fstack-protector-strong -c $<

install: pam_setquota.so
	install --mode=644 pam_setquota.so /lib/security

clean:
	rm -f pam_setquota.o pam_setquota.so

