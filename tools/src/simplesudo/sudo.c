#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include "include/sudoers.h"

const char msg1[]="something wrong happened\n";
const char msg2[]="not enough privileges\n";
const char msg3[]="group ";
const char msg4[]=" not found\n";

int main (int argc, char *argv[]){

	if(argc==1){
		write(2, &msg1, sizeof(msg1));
		return 1;
	}

	uid_t uid = getuid();
	
	//if root, then just execute

	if(uid==0)
		goto exec;

	struct group* grp=getgrnam(GROUPNAME);

	//cannot find group

	if(!grp){
		write(2, &msg3, sizeof(msg3));
		write(2, GROUPNAME, sizeof(GROUPNAME));
		write(2, &msg4, sizeof(msg4));
		return 1;
	}

	//if user is wheel (or equivalent)

	if (getgid() == grp->gr_gid)
		goto exec;
	else{
		int allow=0;
		int ngroups = getgroups(0, NULL);
        	gid_t *groups = malloc(ngroups * sizeof(gid_t));
        	getgroups(ngroups, groups);
        	for (int i = 0;i<ngroups;i++)
            		if (groups[i] == grp->gr_gid){
                		free(groups);
				goto exec;
            		}
     		free(groups);
	}
	write(2, &msg2, sizeof(msg2));
	return 255;
	exec:
		setuid(0);
		setgid(0);
		execvp(argv[1],&argv[1]);
                write(2, &msg1, sizeof(msg1));
                return 1;
}
