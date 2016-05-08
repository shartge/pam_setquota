[![Build Status](https://travis-ci.org/shartge/pam_setquota.svg?branch=master)](https://travis-ci.org/shartge/pam_setquota) [![Coverity Scan Build Status](https://scan.coverity.com/projects/8790/badge.svg)](https://scan.coverity.com/projects/shartge-pam_setquota)

PAM `setquota` module
=====================

This module sets (or modifies) a disk quota when a session begins.

This makes quotas usable with central user databases, such as MySql or LDAP.

Usage
-----
A single invocation of `pam_setquota` applies a specific policy to a UID range.
Applying different policies to specific UID ranges is done by invoking
`pam_setquota` more than once.

Some parameters can be passed to `pam_setquota.so` through the PAM config:
- `fs` is the device file or mountpoint the policy applies to.  
  Defaults to the filesystem containing the users home directory.
- `startuid` and `enduid` describe the UID range the policy is applied to.  
  Setting `enduid=0` results in an open-ended UID range (i.e. all uids greater
  than `startuid` are included).  
  Defaults to `startuid=1000` and `enduid=0`.
- `overwrite` lets you override an existing quota.
  Note: Enabling this will remove the ability for the admin to manually configure
	a different quota for users for a filesystem with `edquota(8)`.
- `bsoftlimit`, `bhardlimit`, `isoftlimit` and `ihardlimit` are as defined by
  `quotactl(2)`:
  - `b` expresses a number of blocks (size limit), whereas
	`i` is a limit on the number of inodes.
  - `softlimit` is a threshold after which the user gets warnings,
	whereas `hard` limits cannot be exceeded.


Example
-------

	session    required     /lib/security/pam_setquota.so bsoftlimit=19000 bhardlimit=20000 isoftlimit=3000 ihardlimit=4000 startuid=1000 enduid=2000 fs=/dev/sda1
	session    required     /lib/security/pam_setquota.so bsoftlimit=1000 bhardlimit=2000 isoftlimit=1000 ihardlimit=2000 startuid=2001 enduid=0 fs=/home
	session    required     /lib/security/pam_setquota.so bsoftlimit=19000 bhardlimit=20000 isoftlimit=3000 ihardlimit=4000 startuid=1000 enduid=2000 fs=/dev/sda1 overwrite=1


Licence and credits
-------------------

Released under the GNU LGPL version 2 or later

Originally written by Ruslan Savchenko <savrus@mexmat.net> April 2006  
Structure taken from pam_mkhomedir by Jason Gunthorpe <jgg@debian.org> 1999
- <http://code.google.com/p/pam-setquota/>

Modified Jan 26, 2010 by Shane Tzen <shane@ict.usc.edu>
- added overwrite option to avoid overwriting existing quotas
- http://pam-setquota.googlecode.com/files/setquota.patch

Modified and adapted to new PAM versions by Sven Hartge <sven@svenhartge.de>
- https://github.com/shartge/pam_setquota

Documented, tested and modified by Keller Fuchs <kellerfuchs@hashbang.sh>
- https://github.com/hashbang/pam_setquota
