/* PLfork.c
 * P1
 *  |--- P2
 *       |---P5 -> P8
 *       |--- P3 -> P4 -> P6 and P7
 *       |--- P9
 *
 * Each process prints its own PID just once
 */

#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <sys/types.h>  // pid_t
#include <string.h>


int main() {
    pid_t pid;

    // P1: original/root process prints its PID 
    printf("%d\n", getpid()); // prints P1's PID

    // Create P2: fork from P1 
    pid = fork();
    if (pid == 0) {
        // This block runs in the child process -> P2 
        printf("%d\n", getpid()); // prints P2's PID

        // Left child of P2: P5 (and its child P8) 
        pid = fork();
        if (pid == 0) {
            // In P5 
            printf("%d\n", getpid()); // prints P5's PID

            // Create P8 as child of P5 
            pid = fork();
            if (pid == 0) {
                // In P8 
                printf("%d\n", getpid()); // prints P8's PID
            }
            else {
                // In P5 (the parent of P8): wait for P8 to finish 
                wait(NULL); 
            }
            // P5 and P8 done 
            exit(0); // stop P5 from executing rest of P2's code
        }
        else {
            // In P2: wait for P5 subtree to finish before making next child 
            wait(NULL);
        }

        // Middle child of P2: P3 (which makes P4->P6 and P7) 
        pid = fork();
        if (pid == 0) {
            // In P3 
            printf("%d\n", getpid()); // prints P3's PID

            // P4 is child of P3 
            pid = fork();
            if (pid == 0) {
                // In P4 
                printf("%d\n", getpid()); // prints P4's PID

                // P6 is child of P4 
                pid = fork();
                if (pid == 0) {
                    // In P6 
                    printf("%d\n", getpid()); // prints P6's PID
                } else {
                    wait(NULL); // P4 waits for P6
                }
                exit(0); // P4 ends here
            } else {
                wait(NULL); // P3 waits for P4 subtree
            }

            // P7 is another child of P3 
            pid = fork();
            if (pid == 0) {
                // In P7 
                printf("%d\n", getpid()); // prints P7's PID
            } else {
                wait(NULL); // P3 waits for P7
            }

            exit(0); // P3 done
        } else {
            wait(NULL); // P2 waits for the P3 subtree
        }

        // Right child of P2: P9  
        pid = fork();
        if (pid == 0) {
            // In P9 
            printf("%d\n", getpid()); // prints P9's PID
        } else {
            wait(NULL); // P2 waits for P9
        }

        exit(0); // P2 done
    } else {
        // In P1: wait for P2 subtree to finish, then exit 
        wait(NULL);
    }

    return 0;
}
