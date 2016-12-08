//  Created by Nuxes Lin on 16/10/10.
//  Copyright © 2016年 Nuxes Lin. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <zconf.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

class AsyncBase {
public:
    inline AsyncBase() {};
    void Ref();
    virtual void wrap() = 0;
    virtual ~AsyncBase() {};
};

class AsyncStream : public AsyncBase{
public:
    inline AsyncStream() {};
    virtual void yields() {};
    void wrap() {}
    int fields;
};

class Observer {
public:
    inline void observer();
};

class Looper {
    
};

void Observer::observer() {
    AsyncStream* stream = new AsyncStream();
    std::cout << " " << sizeof(AsyncStream) << std::endl;
    delete stream;
}

void ForkSlave() {
    int shared_counter = 0;
    pid_t pid;
    char buffer[] = "data to be written to buffer\n";
    //line buffer if we print out to terminal
    //but if we redirect to a file it all exist in buffer
    if (write(STDOUT_FILENO, buffer, sizeof(buffer) - 1) != sizeof(buffer) - 1)
        perror("write failed !\n");
    printf("start to fork a slave process\n");
    if ((pid = fork()) < 0) {
       perror("process fork error\n");
    } else if (pid == 0) {
        //slave process
        //COW the stack (some registers) the slave modified would duplicate a new stack to store all its state context
        shared_counter++;
    } else {
        //parent process
        //wait for slave avoiding it make no sense if the master exit
        sleep(2);
    }

    printf("in fork experiment, show the result ==> pid = %ld, counter = %d\n", (long)getpid(), shared_counter);
    exit(0);
}

void ReportToMaster(int status) {
    if (WIFEXITED(status)) 
        printf("normal termination, exit with the status = %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("terminated by signal = %d%s\n", WTERMSIG(status),
#ifdef WCOREDUMP
                WCOREDUMP(status) ? "(core dump file generated)" : "");
#else
                "");
#endif
    else if (WIFSTOPPED(status))
        printf("stpped by signal = %d\n", WSTOPSIG(status));
}

void SlaveDyingHandler(int signal) {
    printf("master receive the message that slave dead\n");
}

void BoostContextify(char* msg) {
    int c;
    char* itor;
    //each time when process writes without buffer, kernel will switch to next context, so that boost the race condition
    //Therefore, we set buffer to NULL
    //Otherwise, kernel only send the string to buffer, just do that twice, then goes back to schedule processes(put down IO operations), IO device work with buffer，
    //which weaken the race condition
    setbuf(stdout, nullptr);
    for (itor = msg; (c = *itor++) != 0; ) {
        putc(c, stdout);
    }
}

void Dispatch() {
    pid_t pid;
    int status;
    if ((pid = fork()) < 0) {
        perror("fork error !\n");
    } else if (pid == 0) {
        //send a SIGABORT to master
        abort();
        // status /= 0;
    } else {
        // sleep(2);
    }
        
    if (wait(&status) != pid)
        perror("wait error \n");
    ReportToMaster(status);
}


int main(int argc, const char * argv[]) {
    signal(SIGCHLD, SlaveDyingHandler);
    Dispatch();
    BoostContextify("boost\n");
    // printf("master process pid ==>  %ld\n", (long) getpid());
    // ForkSlave();
    return 0;
}
