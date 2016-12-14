#include <unistd.h>
#include <stdio.h>
#include <zconf.h>
#include <signal.h>
// #include <iostream>
#include <stdlib.h>
#include <errno.h>

/*
* P1 -> P2 -> P3 -> P4 -> P5
*/

void Interlock() {
	pid_t pid;
	if ((pid = fork()) < 0) {
		perror("fork error at P1");
	} else if (pid > 0) {
		//P1
		sleep(2);
		exit(0);
	}

	//fork P2
	if ((pid = fork()) < 0) {
		perror("fork error at P2");
	} else if(pid > 0) {
		sleep(4);
		abort();
	}

	//fork P3
	if ((pid = fork()) < 0) {
		perror("fork error at P3");
	} else if (pid > 0) {
		//pour 777 bytes data to a null device
		execl("/bin/dd", "dd", "if=/etc/passwd", "of=/dev/null", NULL);
		exit(7);
	}

	//P4
	if ((pid = fork()) < 0)
		perror("fork error at P4");
	else if (pid > 0) {
		sleep(8);
		exit(0);
	}

	//P5
	sleep(6);
	raise(SIGKILL);
	//abort
	exit(6);

}

void HangUpHandler(int signal) {
	printf("child process receive the hang up signal\n");
}

volatile int quit_flag = 0;

void QuitAndIntSigHandler(int signo) {
	if (SIGINT == signo) {
		printf("\ncatch signal interrupt\n");
	} else if (SIGQUIT == signo) {
		printf("\ncatch SIGQUIT\n");
		quit_flag = 1;
	}
}

void WaitUtilQuit() {
	sigset_t oldmask, newmask, waitmask;
	//register SIGQUIT and SIGINT
	if (signal(SIGINT, QuitAndIntSigHandler) == SIG_ERR)
		perror("can not catch SIGINT");
	if (signal(SIGQUIT, QuitAndIntSigHandler) == SIG_ERR)
		perror("can not catch SIGQUIT");
	//initialize all signal mask
	sigemptyset(&newmask);
	sigemptyset(&waitmask);
	//master receive the SIGQUIT so it continues, or it suspends
	sigaddset(&newmask, SIGQUIT);
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		perror("set signal mask of process error");
	//when SIGINT occurs, this atomic operation will return, without set 'quit_flag' to 1
	while (quit_flag == 0)
		sigsuspend(&waitmask);
	//continue
	//reset mask
	quit_flag = 0;
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		perror("set signal mask of process error");
	exit(0);
}

//generate an orphan processes group
void Massacre() {
	char c;
	pid_t pid;
	if ((pid = fork()) < 0)
		perror("fork error before massacre");
	//parent dead before child spawned
	else if (pid > 0) {
		sleep(5);
		printf("PID-%d : I will suicide\n", getpid());
		exit(0);
	}

	//child process
	printf("PID-%ld : seek for essential, parent named %ld, my group named %ld, foreground group on terminal named %ld\n", (long)getpid(), (long) getppid(), (long)getpgrp(), (long)tcgetpgrp(STDOUT_FILENO));
	signal(SIGHUP, HangUpHandler);
	//stop itself to hang up the foreground work
	raise(SIGTSTP);
	printf("PID-%ld : seek for essential when I'm an orphan, parent named %ld, my group named %ld, foreground group on terminal named %ld\n", (long)getpid(), (long)getppid(), (long)getpgrp(), (long)tcgetpgrp(STDOUT_FILENO));
	//child turns into a backgroud process, when it ask for read-request, terminal sends a SIGTTIN to it, error occured
	if (read(STDIN_FILENO, &c, 1) != 1) {
		printf("read error status : %d at current TTY\n", errno);
	}
}

int main() {
	// Interlock();
	// Massacre();
	WaitUtilQuit();
	return 0;
}