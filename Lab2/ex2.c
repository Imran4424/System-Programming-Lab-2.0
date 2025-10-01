/**
 * Author: Prof. Neerja Mhaskar
 * Course: Operating Systems Comp Sci 3SH3/SFWRENG 3SH3
 */

#include <stdio.h>
#include  <sys/types.h> /* This header file has the definition for pid_t type*/

int main()
{
	pid_t pid;	
	printf("Root: %d\n",getpid()); /*This will print the root/parent*/
	pid = fork();
	if(pid == 0) {
		printf("Child: %d\n",getpid()); /*This will print the child*/
		char *argv[] = {"Hello ", "World!", NULL};
		int j = execvp("./ex3", argv);
		if(j <0) {
			printf("Error executing execvp\n");	
		}

	}
	else {
		wait(NULL);
		printf("Child process is complete\n");	
	}
	
	return 0;
}
