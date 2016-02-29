#!/bin/sh -ev

make
clang-format-3.4 pam_setquota.c | diff -q pam_setquota.c -
