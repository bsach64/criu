#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <linux/limits.h>

#include "zdtmtst.h"

const char *test_doc = "Check files on a detached mount";
const char *test_author = "Bhavik Sachdev <b.sachdev1904@gmail.com>";

char *dirname;
TEST_OPTION(dirname, string, "directory name", 1);

#define TEST_FILE "hello" /* probably will come with a better name */

int main(int argc, char **argv)
{
	char *loop;
	 /* opened to a file on the detached mount point */
	int fd;
	/* some data for testing */
	char *hello = "hello";
	size_t len = strlen(hello);
	char buf[len + 1];
	char path[PATH_MAX];

	test_init(argc, argv);

	loop = getenv("ZDTM_DETACHED_MNT");
	if (loop == NULL) {
		pr_perror("ZDTM_DETACHED_MNT is not set");
		return 1;
	}

	if (mount(loop, dirname, "ext4", 0, NULL) == -1) {
		pr_perror("mount");
		return -1;
	}

	snprintf(path, sizeof(path), "%s/%s", dirname, TEST_FILE);

	fd = open(path, O_CREAT | O_RDWR);
	if (fd < 0) {
		pr_perror("open %s", path);
		return 1;
	}

	if (write(fd, hello, len) != len) {
		pr_perror("write %s", path);
		goto err;
	}

	/* detach the mount lazily */
	if (umount2(dirname, MNT_DETACH)) {
		pr_perror("umount2 %s", dirname);
		goto err;
	}

	test_daemon();
	test_waitsig();

	/* Should still be able to read from the fd */
	if (lseek(fd, 0, SEEK_SET)) {
		pr_perror("lseek %s", path);
		goto err;
	}

	if (read(fd, buf, len) != len) {
		pr_perror("read %s", path);
		goto err;
	}

	buf[len] = 0;
	/* Should contain the same data */
	if (strncmp(hello, buf, len) != 0) {
		fail();
	} else {
		pass();
	}
	close(fd);
	return 0;
err:
	close(fd);
	return 1;
}
