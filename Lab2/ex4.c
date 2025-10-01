/**
 * Author: Prof. Neerja Mhaskar
 * Course: Operating Systems Comp Sci 3SH3/SFWRENG 3SH3
 * You can verify your answer by counting 
 * the number of unique processes which are output
 * by the call to getpid() - which is 2 unique processes.
 * Note: The statement 'printf("%d\n",getpid());' prints
 * the pid of the current process.
 */

#include <stdio.h>
#include  <sys/types.h> /* This header file has the definition for pid_t type*/

int main()
{
	pid_t pid;	
	
	fork();
	fork();
	fork();
	
	//return 0;
}

