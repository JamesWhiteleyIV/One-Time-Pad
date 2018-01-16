/* James Whiteley IV
 * OTP - keygen
 * 2017-8-7
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

int main(int argc, char *argv[]) {
	srand(time(0)); //seed random

	//make sure atleast 2 args entered
	//output goes to stderr
	if (argc < 2) {
		fprintf(stderr, "USAGE: keygen [keylength]\n");
		exit(1);
	}
	
	int len = atoi(argv[1]); //number of chars in key

	//make sure keylength is greater than 0
	//output goes to stderr
	if(len < 1) {
		fprintf(stderr, "keylength must be greater than 0\n");
		exit(1);
	}
	
	char letter;
	char key[len+1]; //create array of char's for key
	int i;
	char *choices = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	//add each random character to the key
	for (i = 0; i < len; i++) {
		letter = choices[random () % 27]; //get random letter or space
		key[i] = letter;
	}

	//NULL terminate
	key[len] = '\0';
	printf("%s\n", key); //print to stdout

	return 0;

}
