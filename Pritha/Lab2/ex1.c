/**
 * Author: Prof. Neerja Mhaskar
 * Course: Operating Systems Comp Sci 3SH3/ SFWRENG 3SH3
 * Fork() returns the pid of the child in the parent process
 * and returns 0 in the child process. Using this you can
 * print the PID of both the parent and the child processes.
 * The statement 'printf("%d\n",getpid());' prints
 * the pid of the current process.
 */

#include <stdio.h>
#include  <sys/types.h> /* This header file has the definition for pid_t type*/

int main()
{
	pid_t pid;	
	
	pid = fork();
	if(pid == 0) {	
		printf("Child Process with PID: %d\n",getpid()); 
	}
	else {
		printf("Parent Process with PID: %d\n",getpid()); 
	}
	
	return 0;
}

