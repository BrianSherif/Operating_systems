/*
Operating Systems
CO20-320202
Brian Sherif Nazmi Hanna Nasralla B.hannanasralla@jacobs-university.de
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>

int
main(int argc, char **argv) {

    int opt;
    char input[1];

    optind = argc - 1;
    while((opt = getopt(argc, argv, "+slp")) >= -1) {
        switch (opt) {
            case 's': {
                ssize_t check = read(0, &input, 1);
                while (check == sizeof(input)) {
                    if (check == -1) {  //Error handling read return value
                        fprintf(stderr, "Error reading input\n");
                        return 1;
                    }
                    check = write(1, &input, 1);
                    if (check == -1 || check != sizeof(input)) {  //Error handling write return value
                        fprintf(stderr, "Error writing output\n");
                        return 1;
                    }
                    //Reading next char and updating check
                    check = read(0, &input, 1);
                }
                break;
            }
            case 'l':
            default: {
                input[0] = getc(stdin);
                if (input[0] == EOF)
                    return 0;

                int check;
                while(input[0] != EOF) {
                    check = putc(input[0], stdout);

                    if (check == EOF) {
                        fprintf(stderr, "Error writing input\n");
                        return 1;
                    }
                    input[0] = getc(stdin);
                }
                break;
            }
            case 'p':
                while(sendfile(1, 0, NULL, 1)); //Copying one byte at a time
                break;
            case '?':
                fprintf(stderr, "Unknown option");
                return 1;
        }
    }

    return 0;
}
