/*
Operating Systems
CO20-320202
Brian Sherif Nazmi Hanna Nasralla B.hannanasralla@jacobs-university.de
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>

int
execute(int argc, char **args, int *b_flag, int *e_flag, int *n_flag, int secs) {

    while (1) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Couldn't fork\n");
            exit(-1);
        }
        else if (pid == 0) { //Inside child process
            if(execvp(args[0], args) == -1) {
                if(*b_flag){
                    printf("a\n");
                }

                fprintf(stderr, "exec failure\n");
                exit(1);
            }
            _exit(0);
        }
        else { //Inside parent process
            int status;
            waitpid(-1, &status, WUNTRACED);

            if(status && *e_flag){
                exit(1);
            }

            if(*n_flag) {
                if(secs < 1) {
                    fprintf(stderr, "Invalid time input\n");
                    return 1;
                }
                sleep(secs);
            }
            else {
                //Default sleep time is 2 seconds if -n is not specified
                sleep(2);
            }
        }
    }
    return 1;
}

int
main(int argc, char **argv)
{
    int opt;
    int b_flag = 0;
    int e_flag = 0;
    int n_flag = 0;
    int secs;
    while ((opt = getopt(argc, argv, "+ben:")) != -1) {
        switch (opt) {
            case 'b':
                b_flag = 1;
                break;
            case 'e':
                e_flag = 1;
                break;
            case 'n':
                n_flag = 1;
                secs = atoi(optarg);
                break;
            case '?':
            {
                fprintf(stderr, "Non valid parameter\n");
                return 1;
            }
        }
    }

    //Define the size of args to be the number of non-option arguments plus 1
    char *args[argc - optind + 1];
    //Copy the non-option arguments to the first positions of args
    for(int index = 0; index < argc - optind; index++){
        args[index] = argv[optind + index];
    }
    //Terminate args with null pointer
    args[argc - optind] = '\0';

    int ret = execute(argc, args, &b_flag, &e_flag, &n_flag, secs);
    if(ret) {
        return 1;
    }

    return 0;
}
