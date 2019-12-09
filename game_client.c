#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include "string.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>

void verifyN(int n, int direction);

void error (char *message) {
    perror(message);
    exit(0);
}

int sockfd; //stores values returned by the socket system
int portno; //stores the port number on which the server accepts connection
int n; //return value for the read() and write() calls

struct sockaddr_in serv_addr; //contains server address of the server to be connected to
struct hostent *server; //holds host information in struct hostent inside netdb.h

char buffer[256]; //sets buffer to read into

int main() {
    char inputSize = 20;
    int option, locid, playerID;
    char command[20];
    int quit = 0;
    char username[18];
    bool iswin = false;
    
    while (quit != 1) {
        printf("Please enter a command\n");
        fscanf(stdin, "%s", command);

        //drops string to lowercase
        for (int i = 0; i < strlen(command); i++) {
            command[i] = tolower(command[i]);
        }

        //converts input text to option paramater
        option = strcmp(command, "connect") == 0 ? 0 : strcmp(command, "reset") == 0 ? 1 :
                                                       strcmp(command, "display") == 0 ? 2 :
                                                       strcmp(command, "move") == 0 ? 3 :
                                                       strcmp(command, "quit") == 0 ? 4 : -1;

        switch (option) {
            case 0: //connect
                printf("connect\n");
                char portNumber[32], hostName[32], username[16], filename[32], portnum[20];
                fprintf(stdout, "please enter a host name and port number seperated by a space\n");
                fscanf(stdin, "%s %s", hostName, portNumber);
                fprintf(stdout, "hostName: %s, port: %s\n", hostName, portNumber);
                //user enters port number, if port isnt available send error
                portno = atoi(portNumber);
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0)
                    error("ERROR opening socket");

                //gets hostname from hostent in host ent *h_addr contains host ip address
                server = gethostbyname(hostName);
                if (server == NULL) {
                    fprintf(stderr, "ERROR, no such host\n");
                    exit(0);
                }

                //sets all the fields in serv_addr to the values of the server
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *) server->h_addr,
                      (char *) &serv_addr.sin_addr.s_addr,
                      server->h_length);
                serv_addr.sin_port = htons(portno);

                //attempts to connect to the given socket and returns error if it cannot connect
                if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                    error("ERROR connecting");
                } else {
                    fprintf(stdin, "Successfully Connected!");
                }
                memset(buffer, 0, strlen(buffer));

		printf("Please enter your username:\n");
		scanf("%s",&buffer);
		n = write(sockfd,username,19);
		n = read(sockfd,buffer,255);
		printf("%s\n",buffer);
		if(strcmp(buffer,"block")== 0){
		  printf("Server full, please try again!\n");
		}
		//n = read(sockfd,buffer,255);
		n = write(sockfd,"ok", 3);
		bzero(buffer,256);
		//-------------Game Part-----------------------------------------------//

		while(strcmp(buffer,"start") != 0)
		  n = read(sockfd,buffer,255); //waits for 2 people

		
		printf("Game started!",buffer);
		n = write(sockfd, "start", 6);
		bzero(buffer,256);
		n = read(sockfd,buffer,256);
		locid = atoi(buffer);
		printf("You are player: %d\n", locid);
		n = write(sockfd,"ok",3);
		bzero(buffer,256);

		while(!iswin){
		  n = read(sockfd,buffer,255);
		  playerID = atoi(buffer);
		  playerID++;
		  printf("It is player %d's turn:\n",playerID);
		  playerID--;

		  n = write(sockfd,"ok",3);
		  while(strcmp(buffer,"done") != 0){
		      bzero(buffer,255);
		      n = read(sockfd,buffer,255);
		      printf("%s\n",buffer);
		      n = write(sockfd,"ok",3);
		  }

		  n = read(sockfd,buffer,255);
		  bzero(buffer,256);
		  
		  if(playerID == (locid -1)){
		    printf("Where do you want to drop your piece?\n");
		    scanf("%s",&buffer);
		    n = write(sockfd,buffer,2);
		    n = read(sockfd, buffer,255);
		  }else{
		    printf("Waiting on opponent's move...\n");
		    
		    while(strcmp(buffer,"next") != 0)
		      n = read(sockfd,buffer,255);
		  }
		  n = write(sockfd,"endturn",8);
		  //n = read(sockfd, buffer,255);
		  bzero(buffer,256);

		  n = read(sockfd,buffer,255);
		  if(strcmp(buffer,"true") == 0){
		    iswin = true;
		    playerID++;
		    printf("%d has won!\n Game over!\n", playerID);
		  }
		  
		  
		}
		
		
		break;
	    case 1: {// reset test
                n = write(sockfd, "reset", 5);
		n = read(sockfd, buffer, 255);
		printf("game reset!\n");
                break;
            }
            case 2: { //display test
	        n = write(sockfd, "display", 8);
		printf("display\n");
		while(strcmp(buffer,"done") != 0){
		  bzero(buffer, 255);
		  n = read(sockfd, buffer, 255);
		  printf("%s\n",buffer);
		  n = write(sockfd,"ok",3);
		}
                break;
            }
	    case 3:{ //move test
	      n = write(sockfd,"move",5);
	      bzero(buffer,256);
	      printf("Pick a column to move to:\n");
	      scanf("%s",&buffer);
	      if(atoi(buffer) < 0 || atoi(buffer) > 6)
		printf("Please pick a number between 0 and 6\n");
	      else{
		n = write(sockfd,buffer, 2);
		n = read(sockfd, buffer, 255);
	      }
                break;
	    }
            case 4: //quit
                n = write(sockfd, "quit", 5);
                verifyN(n, 1);
                quit = 1;
                break;
            default:
                error("incorrect command entered\n");
                break;
                }
        }
}


void verifyN(int n, int direction) {
    if (direction == 1) {
        if (n < 0)
            error("ERROR writing to socket");
        bzero(buffer, 256);
    }
    else {
        error("ERROR reading from socket");
        bzero(buffer, 256);
    }
}

