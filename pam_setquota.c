/* PAM Set Quota module

   This module will set disk quota when session begins.  This allows
   users to be present in central database (such as mysql). To
   have different policies, pam_setquota works only with uids
   from startuid to enduid. If you do not have max uid, set enduid=0.
   The fs parameter should be either block device or mount point. If
   fs does not set, homedir filesystem will be used to set quota.

      
   Here are sample PAM config lines:

   session    required     /lib/security/pam_setquota.so bsoftlimit=19000 bhardlimit=20000 isoftlimit=3000 ihardlimit=4000 startuid=1000 enduid=2000 fs=/dev/sda1
   session    required     /lib/security/pam_setquota.so bsoftlimit=1000 bhardlimit=2000 isoftlimit=1000 ihardlimit=2000 startuid=2001 enduid=0 fs=/home
   session    required     /lib/security/pam_setquota.so bsoftlimit=19000 bhardlimit=20000 isoftlimit=3000 ihardlimit=4000 startuid=1000 enduid=2000 fs=/dev/sda1 overwrite=1
   
   Released under the GNU LGPL version 2 or later
   Originally written by Ruslan Savchenko <savrus@mexmat.net> April 2006
   Structure taken from pam_mkhomedir by Jason Gunthorpe
     <jgg@debian.org> 1999

   Modified Jan 26, 2010 by Shane Tzen <shane at ict d0t usc d0t edu>
     - added overwrite option to avoid overwriting existing quotas

*/

#include <sys/types.h>
#include <sys/quota.h>
#include <linux/quota.h>
#include <pwd.h>
#include <syslog.h>
#include <errno.h>
#include <mntent.h>
#include <stdio.h>

#define PAM_SM_SESSION

#include <security/pam_modules.h>
#include <security/_pam_macros.h>
#include <security/pam_ext.h>
#include <security/pam_modutil.h>

struct pam_params
{
	uid_t start_uid;
	uid_t end_uid;
	char *fs;
	int overwrite;
	int debug;
};

static void
debug(pam_handle_t *pamh, struct if_dqblk *p)
{
	pam_syslog(pamh, LOG_DEBUG, "bsoftlimit=%llu bhardlimit=%llu isoftlimit=%llu ihardlimit=%llu", p->dqb_bsoftlimit, p->dqb_bhardlimit, p->dqb_isoftlimit, p->dqb_ihardlimit);
}


static void 
_pam_parse_dqblk(int argc, const char **argv, struct if_dqblk *p)
{
	for (; argc-- > 0; ++argv)
	{
		if (strncmp (*argv, "bhardlimit=", 11) == 0)
		{
			p->dqb_bhardlimit = strtol (*argv + 11, NULL, 10);
			p->dqb_valid |= QIF_BLIMITS;
		}
		else if (strncmp (*argv, "bsoftlimit=", 11) == 0)
		{
			p->dqb_bsoftlimit = strtol (*argv + 11, NULL, 10);
			p->dqb_valid |= QIF_BLIMITS;
		}
		else if (strncmp (*argv, "ihardlimit=", 11) == 0)
		{
			p->dqb_ihardlimit = strtol (*argv + 11, NULL, 10);
			p->dqb_valid |= QIF_ILIMITS;
		}
		else if (strncmp (*argv, "isoftlimit=", 11) == 0)
		{
			p->dqb_isoftlimit = strtol (*argv + 11, NULL, 10);
			p->dqb_valid |= QIF_ILIMITS;
		}
		else if (strncmp (*argv, "btime=", 6) == 0)
		{
			p->dqb_btime = strtol (*argv + 6, NULL, 10);
			p->dqb_valid |= QIF_TIMES;
		}
		else if (strncmp (*argv, "itime=", 6) == 0)
		{
			p->dqb_itime = strtol (*argv + 6, NULL, 10);
			p->dqb_valid |= QIF_TIMES;
		}
	}
}

static void
_pam_parse_params(int argc, const char **argv, struct pam_params *p)
{
	for (; argc-- > 0; ++argv)
	{
		if (strncmp (*argv, "startuid=",9) == 0)
			p->start_uid = strtol (*argv + 9, NULL, 10);
		else if (strncmp (*argv, "enduid=", 7) == 0)
			p->end_uid = strtol (*argv + 7, NULL, 10);
		else if (strncmp (*argv, "fs=",3) == 0)
			p->fs = (char*) *argv + 3;
		else if (strncmp (*argv, "overwrite=",10) == 0)
			p->overwrite = strtol (*argv + 10, NULL, 10);
		else if (strncmp (*argv, "debug=",6) == 0)
			p->debug = strtol (*argv + 6, NULL, 10);
	}
}



PAM_EXTERN int
pam_sm_open_session (pam_handle_t *pamh, int flags, int argc,
		     const char **argv)
{
	int retval;
	const void *user;
	const struct passwd *pwd;
	struct pam_params param =
	{
		.start_uid = 1000,
		.end_uid = 0,
		.fs = NULL
	};
	struct if_dqblk ndqblk;
	FILE *fd;
	char mntdevice[BUFSIZ], mntpoint[BUFSIZ];
	const struct mntent *mnt;
	const char *service;

	
	if (pam_get_item (pamh, PAM_SERVICE, (const void **) &service) != PAM_SUCCESS)
		service = "";

	/* Parse values */
	_pam_parse_params(argc, argv, &param);

	/* Determine the user name so we can get the home directory */
	retval = pam_get_item(pamh, PAM_USER, &user);
	if (retval != PAM_SUCCESS || user == NULL ||
		*(const char *)user == '\0')
	{
		pam_syslog(pamh, LOG_NOTICE, "user unknown");
		return PAM_USER_UNKNOWN;
	}

	/* Get the password entry */
	pwd = getpwnam(user);
	if (pwd == NULL)
	{
		return PAM_CRED_INSUFFICIENT;
	}

	/* Check if we should not set quotas for user */
	if ((pwd->pw_uid < param.start_uid)
	    || ((param.end_uid >= param.start_uid)
		&& (param.start_uid != 0)
		&& (pwd->pw_uid > param.end_uid)))
		return PAM_SUCCESS;

	/* Remove the unnecessary '/' from the end of fs parameter */
	if (param.fs != NULL)
	{
		if (strlen(param.fs) > 1)
		{
			if (param.fs[strlen(param.fs) - 1] == '/')
				param.fs[strlen(param.fs) - 1] = '\0';
		}
	}

	/* Find out what device the filesystem is hosted on */
	if ((fd = setmntent("/etc/mtab", "r")) == NULL)
	{
		pam_syslog(pamh, LOG_ERR, "Unable to open /etc/mtab");
		return PAM_PERM_DENIED;
	}
	
	*mntpoint = *mntdevice = '\0';
	while ((mnt = getmntent (fd)) != NULL)
	{
		if (param.fs == NULL)
		{
			/* If fs is not specified use filesystem with homedir as default */
			if ((strncmp(pwd->pw_dir, mnt->mnt_dir, strlen(mnt->mnt_dir)) == 0) && (strlen(mnt->mnt_dir) > strlen(mntpoint)))
			{
				strncpy(mntpoint, mnt->mnt_dir, sizeof (mntpoint));
				strncpy(mntdevice, mnt->mnt_fsname, sizeof (mntdevice));
			}
		}
		else if (((strncmp(param.fs, mnt->mnt_dir, strlen(mnt->mnt_dir)) == 0) && (strlen(param.fs) == strlen(mnt->mnt_dir))) ||
			 (strncmp(param.fs, mnt->mnt_fsname, strlen(mnt->mnt_fsname)) == 0) && ((strlen(param.fs) == strlen(mnt->mnt_fsname))))
		{
			strncpy(mntdevice, mnt->mnt_fsname, sizeof (mntdevice));
		}
	}

	if (*mntdevice == '\0'){
		pam_syslog(pamh, LOG_ERR, "Filesystem not found");
		return PAM_PERM_DENIED;
	}
	
	/* Get limits */
	if (quotactl(QCMD(Q_GETQUOTA,USRQUOTA), mntdevice, pwd->pw_uid, (void*) &ndqblk) == -1)
	{
		pam_syslog(pamh, LOG_ERR, "fail to get limits for user %s : %s", pwd->pw_name, strerror(errno));
		return PAM_PERM_DENIED;
	}

	if ( param.debug == 1 )
		debug(pamh, &ndqblk);

	/* Only overwrite if quotas aren't already set or if overwrite is set */
	if ( (ndqblk.dqb_bsoftlimit == 0 && ndqblk.dqb_bhardlimit == 0 && ndqblk.dqb_isoftlimit == 0 && ndqblk.dqb_ihardlimit == 0) || param.overwrite == 1 )
	{
		/* Parse new limits */
		ndqblk.dqb_valid = 0;
		_pam_parse_dqblk(argc,argv,&ndqblk);

		/* Set limits */
		if (quotactl(QCMD(Q_SETQUOTA,USRQUOTA), mntdevice, pwd->pw_uid, (void*) &ndqblk) == -1)
		{
			pam_syslog(pamh, LOG_ERR, "fail to set limits for user %s : %s", pwd->pw_name, strerror(errno));
			return PAM_PERM_DENIED;
		}
		if ( param.debug == 1 )
			debug(pamh, &ndqblk);	
	}			
	return PAM_SUCCESS;	
}


PAM_EXTERN
int pam_sm_close_session (pam_handle_t * pamh, int flags,
			  int argc, const char **argv)
{
	return PAM_SUCCESS;
}

#ifdef PAM_STATIC

/* static module data */
struct pam_module _pam_setquota_modstruct =
{
	   "pam_setquota",
	   NULL,
	   NULL,
	   NULL,
	   pam_sm_open_session,
	   pam_sm_close_session,
	   NULL
};

#endif
