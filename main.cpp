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

/*
* P1 -> P2 -> P3
*/

void TryBlurred() {
    pid_t pid;
    int status;
    if ((pid = fork()) < 0)
        perror("fork error");
    else if (pid == 0) {
        //P2
        if ((pid = fork()) < 0)
            perror("fork error at child process");
        else if (pid > 0) {
            //P2
            sleep(3);
            exit(0);
        }
        //P3
        if (nice(-10) == -1 && !errno)
            perror("increase process priority error");
        // sleep(2);
        printf("P3's parrent process ==> %ld\n", (long) getppid());
        abort();
    }

    //P1
    //wait for all its child processes
    waitpid(pid, &status, 0);
    ReportToMaster(status);
    exit(0);
}

void Dispatch() {
    pid_t pid;
    int status;
    if ((pid = fork()) < 0) {
        perror("fork error !");
    } else if (pid == 0) {
        //send a SIGABORT to master
        // abort();
        // status /= 0;
        BoostContextify("cheer up!\n");
    } else {
        // sleep(2);
        BoostContextify("cheer up!\n");
    }
        
    // if (wait(&status) != pid)
    //     perror("wait error");
    // ReportToMaster(status);
}


int main(int argc, const char * argv[]) {
    signal(SIGCHLD, SlaveDyingHandler);
    // Dispatch();
    // printf("master process pid ==>  %ld\n", (long) getpid());
    TryBlurred();
    return 0;
}
