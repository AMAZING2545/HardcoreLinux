#ifndef EXEC_H
#define EXEC_H

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int exec(const char *file, char *const argv[]) {

    pid_t pid = fork();

    if (pid == 0) {
        execvp(file, argv);
        perror("exec");
        exit(127);
    } else {
        siginfo_t info;
        if (waitid(P_PID, pid, &info, WEXITED) == 0) {} else {
            perror("exec");
            return -1;
        }
    }

    return 0;
}

#endif // EXEC_H
