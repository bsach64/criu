#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

#include "zdtmtst.h"

const char *test_doc = "Checks pidfd sends signal to child process after restore\n";
const char *test_author = "Bhavik Sachdev <b.sachdev1904@gmail.com>";

static int pidfd_open(pid_t pid, unsigned int flags)
{
	return syscall(SYS_pidfd_open, pid, flags);
}

static int pidfd_send_signal(int pidfd, int sig, siginfo_t* info, unsigned int flags)
{
	return syscall(SYS_pidfd_send_signal, pidfd, sig, info, flags);
}

int main(int argc, char* argv[])
{
	#define READ 0
	#define WRITE 1

	int pidfd, ret, sigfd, p[2], recv_sig;
	pid_t child;
	sigset_t set;
	struct signalfd_siginfo info;
	
	test_init(argc, argv);

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &set, NULL)) {
		pr_perror("sigprocmask failed\n");
		return 1;
	}

	if (pipe(p)) {
		pr_perror("Unable to create pipe\n");
		return 1;
	}

	child = fork();
	if (child != 0) {
		close(p[WRITE]);
		pidfd = pidfd_open(child, 0);
		if (pidfd < 0) {
			pr_perror("pidfd_open failed\n");
			return 1;
		} 
		
	} else {
		close(p[READ]);
		sigfd = signalfd(-1, &set, 0);
		if (sigfd < 0) {
			pr_perror("Can't create signalfd\n");
			return 1;
		}
		ret = read(sigfd, &info, sizeof(info));
		if (ret != sizeof(info)) {
			pr_perror("no signal\n");
			return 1;
		}
		recv_sig = 1;
		if (write(p[WRITE], &recv_sig, sizeof(recv_sig)) < 0) {
			pr_perror("Could not write\n");
			return 1;
		}
		close(sigfd);
		close(p[WRITE]);
		return 0;
	}

	test_daemon();
	test_waitsig();

	if (pidfd_send_signal(pidfd, SIGUSR1, NULL, 0)) {
		fail("Could not send signal\n");
		goto out;
	}
	if (read(p[READ], &recv_sig, sizeof(recv_sig)) != sizeof(recv_sig)) {
		fail("Could not read from pipe\n");
		goto out;
	}
	if (recv_sig) {
		pass();
	} else {
		fail("Expected recv_sig = 1, recv_sig = %d\n", recv_sig);
	}
out:
	close(p[READ]);
	close(pidfd);
	return 0;
}
