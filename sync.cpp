#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <boost/regex.hpp>
#include <iostream>
#include <string>

//still requires global resource
//only one resource in this case
volatile static int resource = 0;

extern "C" {

}

void CommunicateViaMQ() {
	key_t mq_key = IPC_PRIVATE;
	pid_t pid;
	int mqid;
	// char mq_msg_buf[1024];
	// if ((mqid = msqget(mq_key, IPC_CREAT)) < 0)
	// 	perror("acquire MQ error");
	// if (msqctl(mqid, ))
}

void LockResWithSemaphore() {
    key_t sem_key = IPC_PRIVATE;
    pid_t pid;
    int sem_id;
    if ((sem_id = semget(sem_key, 1, IPC_CREAT)) < 0)
        perror("acquire semaphore id error");
//    printf("error status: %d\n", errno);
    //define operations to semaphore set
    sembuf ops[] = { { 0, 2, SEM_UNDO } };
    //parent locks resource before forking
    //block? with a endless loop
    if (semop(sem_id, ops, 1) < 0)
        perror("set ops to semaphore set error");

    //fork a child for preemption
    if ((pid = fork()) < 0) {
        perror("fork child error");
    } else if (pid == 0) {
        //child
        //test the sem of resource, only run before parent locks it,
        //it won't await until its parent release the resource
        //attempts to access to resource
        printf("child will access to resource\n");
        //try to test the sem mapped to resource, if Sem <= 0, it will loop until another one reset it
        printf("Dead lock when child process try to ref this resource\n");
        sembuf ops_try[] = { { 0, -4, SEM_UNDO} };
        if (semop(sem_id, ops_try, 1) < 0)
        	perror("set ops to semaphore set error at child");
        resource++;
        printf("wake up with resource = %d\n", resource);
        exit(0);
    } else {
        //parent
        //wait for child
        //parent block, which may result in dead lock
        printf("wait for child\n");
        wait(NULL);
        printf("child dead, and I'll exit with the resource = %d\n", resource);
    }
}


int main() {
	LockResWithSemaphore();
}