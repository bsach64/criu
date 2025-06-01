#include <bits/types.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <linux/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <inttypes.h>

#include "zdtmtst.h"

const char *test_doc = "Check whether CRIU can c/r a fd pointing to a abstract mount created using open_tree";
const char *test_author = "Bhavik Sachdev <b.sachdev1904@gmail.com>";

char *dirname;
TEST_OPTION(dirname, string, "directory name", 1);

#ifndef MOVE_MOUNT_T_EMPTY_PATH
#define MOVE_MOUNT_T_EMPTY_PATH 0x00000040 /* Empty to path permitted */
#endif

#ifndef MOVE_MOUNT_F_EMPTY_PATH
#define MOVE_MOUNT_F_EMPTY_PATH 0x00000004 /* Empty from path permitted */
#endif

#ifndef OPEN_TREE_CLONE
#define OPEN_TREE_CLONE    1         /* Clone the target tree and attach the clone */
#endif

int open_tree(int dfd, char *pathname, unsigned int flags)
{
	return syscall(__NR_open_tree, dfd, pathname, flags);
}

int move_mount(int from_dfd, char *from_pathname, int to_dfd, char *to_pathname, unsigned int flags)
{
	return syscall(__NR_move_mount, from_dfd, from_pathname, to_dfd, to_pathname, flags);
}

int mount(char *special_file, char *dir, char *fstype, unsigned long int rwflag, void *data)
{
	return syscall(__NR_mount, special_file, dir, fstype, rwflag, data);
}

int umount(char *special_file)
{
	return syscall(__NR_umount2, special_file, 0);
}

int main(int argc, char *argv[])
{
	int mntfd;
	test_init(argc, argv);

	if (mkdir(dirname, 0700)) {
		pr_perror("mkdir %s", dirname);
		return 1;
	}

	/* create a mount point at dirname */
	if (mount("none", dirname, "tmpfs", 0, NULL)) {
		pr_perror("mount %s", dirname);
		return 1;
	}

	/* create a abstract (detached) clone mount of this mount */
	mntfd = open_tree(AT_FDCWD, dirname, OPEN_TREE_CLONE);
	if (mntfd < 0) {
		pr_perror("open_tree");
		return 1;
	}

	test_daemon();
	test_waitsig();

	/* we should still be able to create mount using mntfd */
	if (move_mount(mntfd, "", AT_FDCWD, "", MOVE_MOUNT_F_EMPTY_PATH | MOVE_MOUNT_T_EMPTY_PATH)) {
		pr_perror("move_mount");
		return 1;
	}

	/* we should be able to umount, if mounted correctly */
	if (umount(dirname)) {
		pr_perror("umount");
		return 1;
	}
	pass();

	close(mntfd);
	return 0;
}
