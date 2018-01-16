/* James Whiteley IV
 * OTP - client encryption
 * 2017-8-7
 * Description: This program connects to otp_enc_d and asks it to do OTP style encyrption.
 * usage:
 * otp_enc [plaintext file name to encrypt] [encryption key file] [port]
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/ioctl.h> 

typedef enum {False, True} bool; //create boolean type

//epic print pointer function
void print(void *thing) {
	printf("%p\n", thing);
}

//this function exits program and displays error if a char is not a captial letter or a space
void isValidChar(char c, char *file) {
	int d = c;
	//printf("%d\n", d);

	bool valid = False; //initialize to False validity

	//32 is space, 65 - 90 are capital letters
	//10 for new line since each plain text file only has one at end
	if (d == 10 || d == 32 || (d >= 65 && d <= 90)) {
		valid = True;
	}

	//if not valid, exit program
	if(!valid) {
		fprintf(stderr, "Error: file '%s' contains bad characters\n", file);
		exit(1);
	}
}

// exit(1) with an error if catches a bad character (non space or A-Z)
// exit(1) with an error if length of file1 < file2
void checkFiles(char *file1, char *file2) {

	//use this to iterate chars
	int asDec;
	char next;

	//get file length and check all characters for bad char
	FILE *fp1 = fopen(file1, "r");
	FILE *fp2 = fopen(file2, "r");
	int lenF1 = 0;
	int lenF2 = 0;

	//check file1
	next = getc(fp1);
	while(next != EOF) {
		//printf("%c", next);
		isValidChar(next, file1); //make sure char is valid
		lenF1++;
		next = getc(fp1);
	}	
	fclose(fp1);
	lenF1--; //discount new line char
	//printf("LENGTH: %d\n", lenF1 - 1);

	//check file2
	next = getc(fp2);
	while(next != EOF) {
		//printf("%c", next);
		isValidChar(next, file2);
		lenF2++;
		next = getc(fp2);
	}	
	fclose(fp2);
	lenF2--; //discount new line char
	//printf("LENGTH: %d\n", lenF2 - 1);

	//if length of file1 < file2, exit 1
	if(lenF1 < lenF2) {
		fprintf(stderr, "Error: key '%s' is too short\n", file1);
		exit(1);
	}

}

//error from example client/server.c
void error(const char *msg) { 
	perror(msg); 
	exit(0);
}

//reads all characters from a file into a buffer 
void readInFile(char *filename, char *filename2,  char *buffer) {
	long length;
	FILE *f = fopen (filename, "rb");

	if (f)
	{
		fseek (f, 0, SEEK_END); //find length of file
		length = ftell (f);
		fseek (f, 0, SEEK_SET); //reset ptr to 0 position
		fread (buffer, 1, length, f); //read in all chars to buffer
		fclose (f);
	}
	buffer[strcspn(buffer, "\n")] = '!'; // Remove the trailing \n that fgets adds, add delim
	//printf("%s\n", buffer);

	f = fopen (filename2, "rb");

	if (f) //open second file and add to buffer
	{
		fseek (f, 0, SEEK_END);
		long orgLen = length;
		length = ftell (f) ; //read from prev position in buffer
		fseek (f, 0, SEEK_SET);
		fread (buffer+orgLen, 1, length, f); //add to buffer starting at end
		fclose (f);
	}
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	//printf("%s\n", buffer);
}


int main(int argc, char *argv[]) {

	//check for all required  arguments
	if (argc < 4) {
		fprintf(stderr, "USAGE: otp_enc [plaintext] [keyfile] [port]\n");
		exit(1);
	}

	char *plaintext = argv[1]; //get plaintext to encrypt
	char *keyfile= argv[2]; //get key file 
	int portNumber = atoi(argv[3]); //get port number as an integer

	//make sure no bad chars, 
	//make sure keyfile >= plaintext
	checkFiles(keyfile, plaintext);

	//STEPS:
	//create socket socket()
	//connect socket to server connect()
	//use read() write() send() recv() to transfer data to and from socket which is auto sent to and from 
	//socket on server (sockets are like files
	int socketFD, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[200000];
	//char keyBuffer[80000];
	const char hostname[] = "localhost";

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	//portNumber = atoi(argv[2]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(hostname); // Convert the machine name into a special form of address
	//if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	//read file into buffer without trailing \n
	readInFile(plaintext, keyfile, buffer);	
	//printf("\n\n\nSIZE, LEN  OF BUFFER: %d, %d\n\n\n", sizeof(buffer), strlen(buffer));


	//make sure connecting to encryptor not decryptor, exit(2)
	char correctConn[2];
	char encD[2] = "E";
	int encWrite = send(socketFD, &encD, sizeof(encD), 0);
	int encRead = recv(socketFD, &correctConn, sizeof(correctConn), 0);	
	//printf("client reveived: %s\n", correctConn);
	if (strncmp(correctConn, "F", 1) == 0) {
		fprintf(stderr, "ERROR: otp_enc cannot use otp_dec_d\n");
		exit(2);
	}


	//send the size of buffer being sent
	int sizeOfBuffer = strlen(buffer) - 1;
	int convertedSize = htonl(sizeOfBuffer);
	//printf("1. CLIENT wants to send size: %d\n", sizeOfBuffer);
	charsWritten = send(socketFD, &convertedSize, sizeof(convertedSize), 0); // Write to the server
	//printf("2. CLIENT sent request for size: %d\n", sizeOfBuffer);

	//send buffer to server
	int cur = 0;
	int bytesLeft = strlen(buffer) - 1;
	while (cur < bytesLeft) {
		charsWritten = send(socketFD, buffer+cur, bytesLeft, 0); // Write to the server
		//printf("4. CLIENT: sent: %d\n", charsWritten);
		if (charsWritten < 0) {
			error("CLIENT: ERROR writing to socket\n");
			exit(1);
		}
		cur += charsWritten;
		bytesLeft -= charsWritten;
	}

	//get the size of message being sent over from Server 
	int recSize = 0;

	int retStatus = recv(socketFD, &recSize, sizeof(recSize), 0); // Read the server's message from the socket
	int reqSize = ntohl(recSize);
	//printf("10. CLIENT got request for size:  %d\n", reqSize);


	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	int pos = 0;
	bytesLeft = reqSize;	
	do {

		charsRead = recv(socketFD, buffer+pos, 199999, 0); // Read the client's message from the socket
		//printf("12. CLIENT Chars read: %d\n", charsRead);

		if (charsRead < 0) { 
			error("ERROR reading from socket"); 
			exit(1); 
		}
		pos += charsRead;
		bytesLeft -= charsRead;
	} while (bytesLeft > 0);  //while there are bytes left to receive



	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	printf("%s\n", buffer); //print encrypted message to stdout

	close(socketFD); // Close the socket

	return 0;

}







