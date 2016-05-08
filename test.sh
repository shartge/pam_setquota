#!/bin/sh -ev

make
make DESTDIR=`mktemp -d` install
clang-format-3.4 pam_setquota.c | diff -q pam_setquota.c -
