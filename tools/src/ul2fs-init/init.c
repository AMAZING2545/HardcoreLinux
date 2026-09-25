#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <time.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>

int main(){
	//set LD_LIBRARY_PATH
	puts("init");
	puts("forking ul2fs-mount");
	pid_t pid = fork();
	if (pid == 0) {
		mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
		char* argv[] = {"ul2fs","/dev/sda2","/newroot", "-s","f","-o", "allow_other",(char*)0};
		if(execv("/sbin/ul2fs", argv))
			perror("execve(child) failed");
    	}
	else {
        	//wait at least 2 seconds to mount
		sleep(2);
		//execve new init
		mkdir("/newroot/oldroot", 0755);
		if(syscall(SYS_pivot_root,"/newroot", "/newroot/oldroot"))
			perror("pivot_root failed");
		chdir("/");
		mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
		umount2("/oldroot", MNT_DETACH);
		rmdir("/oldroot");
		char* argv[] = {"init",(char*)0};
		if(execv("/etc/init", argv))
			perror("execve(parent) failed");
		puts("system is deadlocked: cannot execve new init");
		puts("either pivot_root failed or the child died");
		puts("entering a coma for 1.5 minutes");
		sleep(100);
	}
}
