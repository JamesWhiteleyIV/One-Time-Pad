/* James Whiteley IV
 * OTP - server encryption
 * 2017-8-7
 * Description: runs in background. Receives text file and key from otp_enc and encodes the file,
 * 	then sends back to otp_enc. 
 * usage:
 * otp_enc_d [port] &
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h> 

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//converts space character to 26, converts A-Z into 0-25
int convertChar(char c) {
	//convert character into numeric for conversion
	int cdec = c; 		
	if (cdec == 32) //space character convert to 26
		cdec = 26;
	else
		cdec -= 65; // 0-25

	return cdec;
}

//converts an integer into the correct capital letter or space
char convertInt(int i) {
	char c;

	//for space
	if (i == 26) {
		return ' ';
	}
	else {
		i += 65;  //add 65 to int	
		char c = i;
		return c;
	}
}


//encrypt the plaintext using key 
void encrypt(char *buffer) {
	char *plaintext;
	char *key;
	//char *encryptedText;
	char delim[2] = "!";
	plaintext = strtok(buffer, delim);
	key = strtok(NULL, delim);
	int i = 0;
	char c = plaintext[i];
	char k = key[i];
	int cdec; //decimal form of c
	int kdec; // decimal form of k
	while(c) {
		cdec = convertChar(c);
		kdec = convertChar(k);
		cdec += kdec; //add key value to plaintext char
		if (cdec > 26)  //if over 26, minus 27
			cdec -= 27;  

		buffer[i] = convertInt(cdec); //append character to buffer	
		i++;
		c = plaintext[i]; //get next letters
		k = key[i];
	}

}

int main(int argc, char *argv[])
{
	//Check if port available, if not error
	//listen to port passed
	//when connection is made, otp_enc_d must call accept() to generate socket
	//child process must make sure its communicating with otp_enc (five processes at a time supported)
	//child received the plaint text and key via communication socket (not original listen socket)
	//child then encrypts file and writes back the cipher text to the otp_enc process connected to same communication socket
	//original server daemon (parent?) continues to listen for new connections (only the child encrypts and sends back)

	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[200000];
	char *encrypted;
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	pid_t pid = -5;
	int status;

	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");
		//printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));



		pid = fork(); //create child process
		//printf("forked pid: %d\n", pid);

		if (pid < 0) {error("ERROR forking process\n"); exit(1);} //error forking

		if (pid == 0) { //child process

			// Get the message from the client and display it
			memset(buffer, '\0', 200000);

			//send approval for connection
			char encD[2];
			int encRead = recv(establishedConnectionFD, &encD, sizeof(encD), 0); 
			if (strncmp(encD, "E", 1) == 0) {
				char correctConn[2] = "T";
				int encWrite = send(establishedConnectionFD, &correctConn, sizeof(correctConn), 0); 
			}
			else {
				char correctConn[2] = "F";
				int encWrite = send(establishedConnectionFD, &correctConn, sizeof(correctConn), 0); 
			}


			//get the size of message being sent over from client
			int recSize = 0;
			int retStatus = recv(establishedConnectionFD, &recSize, sizeof(recSize), 0); // Read the client's message from the socket
			int reqSize = ntohl(recSize);
			//printf("3. SERVER got request for size (bytesLeft):  %d\n", reqSize);

			int pos = 0;
			int bytesLeft = reqSize;	
			do {

				charsRead = recv(establishedConnectionFD, buffer+pos, 199999, 0); // Read the client's message from the socket			
				if (charsRead < 0) { 
					error("ERROR reading from socket"); 
					exit(1); 
				}
				pos += charsRead;
				//printf("POS: %d\n", pos);
				bytesLeft -= charsRead; //remove how many chars have been read
				//printf("bytesLeft: %d\n", bytesLeft);
			} while (bytesLeft > 0); //while still bytes to read

			//printf("6. DONE READING FROM CLIENT\n");

			encrypt(buffer);  	

			//send size of buffer being sent
			int sizeOfBuffer = strlen(buffer);
			int convertedSize = htonl(sizeOfBuffer);
			//printf("8. SERVER wants to send size: %d\n", sizeOfBuffer);
			int charsWritten = send(establishedConnectionFD, &convertedSize, sizeof(convertedSize), 0); // Write to the Client 
			//printf("9. SERVER sent request for size: %d\n", sizeOfBuffer);

			//send encrypted buffer back to client
			int cur = 0;
			bytesLeft = strlen(buffer);
			while (cur < bytesLeft) {
				charsWritten = send(establishedConnectionFD, buffer+cur, bytesLeft, 0); // Write to client 
				//printf("11. SERVER: Chars written: %d\n", charsWritten);
				if (charsWritten < 0) {
					error("SERVER: ERROR writing to socket");
					exit(1);
				}
				cur += charsWritten;
				bytesLeft -= charsWritten; //remove the amount of bytes read in
			}

			close(establishedConnectionFD); // Close the existing socket which is connected to the client

		}

		while (pid > 0) { //parent process

			pid = waitpid(-1, &status, WNOHANG);
		}


	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
