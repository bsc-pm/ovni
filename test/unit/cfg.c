#define _GNU_SOURCE

#include "emu/pv/cfg.h"
#include "common.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>

int main(void)
{
	char dir[] = "/tmp/ovni.trace.XXXXXX";
	if (mkdtemp(dir) == NULL)
		die("mkdtemp failed:");

	if (cfg_generate(dir) != 0)
		die("cfg_generate failed");

	/* Check that one configuration file is present */
	char cfg[PATH_MAX];
	sprintf(cfg, "%s/cfg/cpu/ovni/pid.cfg", dir);

	struct stat st;
	if (stat(cfg, &st) != 0)
		die("stat failed");

	return 0;
}
