pam_setquota.so: pam_setquota.o
	ld -x -shared -o pam_setquota.so pam_setquota.o

pam_setquota.o: pam_setquota.c
	gcc -fPIC -DLINUX_PAM -Dlinux -Di386 -DPAM_DYNAMIC -c pam_setquota.c

install: pam_setquota.so
	install --mode=644 pam_setquota.so /lib/security

clean:
	rm -f pam_setquota.o pam_setquota.so

