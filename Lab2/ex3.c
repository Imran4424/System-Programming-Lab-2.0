/**
 * Author: Prof. Neerja Mhaskar
 * Course: Operating Systems Comp Sci 3SH3/SFWRENG 3SH3
 */

#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include  <sys/types.h> /* This header file has the definition for pid_t type*/

int main(int argc, char **argv) {
	int i=0;
	char str[100] = "";
	printf("argument count: %d\n", argc);
	while (i < argc) {
		strcat(str, argv[i]);
		i=i+1;
	}
	printf("Print from ex3\n");	
	printf("%s\n", str);
	return 0;
}

