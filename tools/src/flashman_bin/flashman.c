#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "include/exec.h"
#include <sys/stat.h>
#include <fcntl.h>

long nothing=0;
long* empty_env={&nothing,0};

int main(int argc, char **argv) {

    /* if no arguments are passed, or the first argument is help */

    if (argc < 2 || strcmp(argv[1], "help") == 0) {
        printf("HardCore Linux Package Manager (AKA Flashman)\n");
        printf("Options:\n\n");

        printf("install <pkg.tar>\n");
        printf("Installs package from a tar archive (requires root)\n\n");

        printf("uninstall <pkg>\n");
        printf("Uninstalls a package by name (requires root)\n\n");

        printf("info <pkg>\n");
        printf("Shows info about a package, potentially including dependencies (might require root)\n"); // or does it?

        return 0;
    }

    /* if the argument is install, we install the package */

    if (strcmp(argv[1], "install") == 0) {

        if (argc < 3 || argv[2] == NULL) {
            printf("ERROR: flashman: install requires a package\n");
            return 1;
        }

        /* 1- Extract the tar archive to / */
        char *tar_args[] = {"tar", "xpzf", argv[2], "-C", "/", NULL};
        exec(tar_args[0], tar_args);

        /* 2- Remove the .tar*/

        char package[PATH_MAX];
        strncpy(package, argv[2], sizeof(package) - 1);
        package[sizeof(package) - 1] = 0;

        char *dot = strrchr(package, '.');

        if (dot != NULL) {
            *dot = 0;
        }

        /* 3- Dependency check */

        char path[PATH_MAX];
        char dependency[4096];

        snprintf(path, sizeof(path), "/etc/installer/%s/pkinfo", package);

        int pkinfo_fd = open(path, O_RDONLY);
        if (pkinfo_fd == -1) {
            perror("ERROR: flashman: cannot open pkinfo");
            return 1;
        }
	char* pkinfo=malloc(4096);
	read(pkinfo_fd, pkinfo, 4096);
        int i = 0;
        int errors = 0;

        puts("missing dependencies");

        while (sscanf(pkinfo, "%255s", dependency) == 1) {

            /* skip first two tokens */
            if (i > 1) {

                char dep_path[PATH_MAX];

                snprintf(dep_path, sizeof(dep_path), "/etc/installer/%s", dependency);

                if (access(dep_path, F_OK) != 0) {
                    printf("%s\t", dependency);
                    errors++;
                }
            }

            i++;
        }

        if (errors > 0) {
            printf("\nAborting\n");
            char* uninst_path=malloc(PATH_MAX);
            snprintf(uninst_path, PATH_MAX, "/etc/installer/%s/uninstall", package);
	    chmod(uninst_path,(7<<6)|(5<<3)|5);
	    char* args[]={"ash",uninst_path};
	    exec("/bin/ash",args);
        } else {
            char* inst_path=malloc(PATH_MAX);
            snprintf(inst_path, PATH_MAX, "/etc/installer/%s/install", package);
            chmod(inst_path,(7<<6)|(5<<3)|5);
	    char* args[]={"ash",inst_path};
	    exec("/bin/ash",args);
        }
        return 0;
    }

    /* if the argument is uninstall, we uninstall it */

    if (strcmp(argv[1], "uninstall") == 0 && argv[2] != NULL) {
        char *package = argv[2];
        char* uninstall_path=malloc(PATH_MAX);
        snprintf(uninstall_path, PATH_MAX, "/etc/installer/%s/uninstall", package);
	printf("%s\n",uninstall_path);
        chmod(uninstall_path,(7<<6)|(5<<3)|5);
	char* args[]={"ash",uninstall_path};
	exec("/bin/ash",args);
        return 0;
    }

    /* if the argument is info, we output the info of the specified package */

    if (strcmp(argv[1], "info") == 0 && argv[1] != NULL) {
        char *package = argv[2];
        char ipkg[PATH_MAX];
        snprintf(ipkg, sizeof(ipkg), "/etc/installer/%s/pkinfo", package);
        char *args[] = {"cat", ipkg, NULL};
        exec(args[0], args);
    }

    /* if the argument is list, we just exec ls /etc/installer */

    if (strcmp(argv[1], "list") == 0) {
        char *args[] = {"ls", "/etc/installer", NULL};
        exec(args[0], args);
        return 0;
    }

}
