#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include "string.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>

void menu(int sock);
void reset();
void send_display(int sock, char s[][7]);
void make_move(int sock, int choice, int id);
bool check_win(char s[][7]);

void error(char *msg)
{
    perror(msg);
    exit(1);
}

//Global Variables for Game.
char spot[7][7], globalnameone[18], globalnametwo[18];
int choice;
static int *player_count;
static int *gchoice;        //Global Choice
static int *playerID;

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, clilen, pid;

     player_count = mmap(NULL, sizeof *player_count, PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_ANONYMOUS, -1,0);

     gchoice  = mmap(NULL, sizeof *gchoice, PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_ANONYMOUS, -1,0);

     playerID = mmap(NULL, sizeof *playerID, PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_ANONYMOUS, -1,0);
     
     *player_count = 0;
     *playerID = -1;
     *gchoice = -1;
     
     
     struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     while (1) {
         newsockfd = accept(sockfd, 
               (struct sockaddr *) &cli_addr, &clilen);
         if (newsockfd < 0) 
             error("ERROR on accept");
         pid = fork();
         if (pid < 0)
             error("ERROR on fork");
         if (pid == 0)  {
	   //player_count++;
             close(sockfd);
             menu(newsockfd);
             exit(0);
         }
         else close(newsockfd);
     } /* end of while */
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void menu(int sock)
{
   int n, column, locID, pc;
   char buffer[256];
   char command[20], username1[18], username2[18], choice[2];
   bool iswin = false;
   
   printf("Conected!\n");
   n = read(sock,buffer,255);
   
   if(*player_count == 0){
     strcpy(globalnameone,buffer);
     *player_count = 1;
     n = write(sock,"Waiting for someone to join",29);
     n = read(sock,buffer,255);
     bzero(buffer,256);
     n = write(sock,"0",2);
   }else if(*player_count == 1){
     strcpy(globalnametwo,buffer);
     *player_count = 2;
     n = write(sock,"Joined game!",13);
     n = read(sock,buffer,255);
     bzero(buffer,256);
     n = write(sock,"1",2);
   }else{
     n = write(sock,"block",6);
     strcpy(buffer,"quit");
   }

   reset();

   pc = *player_count;
   
   
   locID = *player_count;
   
   while(pc < 2){
     pc = *player_count;
     
   }
   //--------------------------Start Game ----------//
   printf("Start Game!\n");
   bzero(buffer,256);
   n = write(sock,"start", 6);

   n = read(sock,buffer,255);
   bzero(buffer,256);
   sprintf(buffer,"%d",locID);
   n = write(sock,buffer,3);
   n =read(sock,buffer,255);
   bzero(buffer,256);
   
   while(!iswin){
     *gchoice = -1;
     sprintf(buffer,"%d",*playerID);
     n = write(sock,buffer,3);
     n = read(sock,buffer,255);

     send_display(sock,spot);

     printf(" %d: Player: %d Turn\n",locID,*playerID);
     
     n = write(sock, "move", 5);
     bzero(buffer,256);

     if(locID == (*playerID + 1)){
       n = read(sock,buffer,255);
       //column = atoi(buffer);
       *gchoice = atoi(buffer);
     }else{
       while(*gchoice == -1)
	 column = 0;
     }

     make_move(sock,*gchoice,*playerID);
     n = write(sock,"next",5);
     n = read(sock,buffer,255);
     printf("%d: %s\n", locID, buffer);

     // n = write(sock,"sync",4);
     //bzero(buffer,256);

     if(check_win(spot)){
       iswin = true;
       n = write(sock,"true",5);
     }else
       n = write(sock,"false",6);
     
   }

   //---------------------------END GAME-------------//

   
   while(strcmp(buffer,"quit") != 0){
     printf("listening\n");
     n = read(sock, buffer, 255);

     if(strcmp(buffer,"reset") == 0){
       reset();
       n = write(sock,"game reseted",15);
     }
     if(strcmp(buffer,"display") == 0){
       //printf("displaying...\n");
       send_display(sock, spot);
     }

     if(strcmp(buffer,"move") == 0){
       n = read(sock,buffer,255);
       column = atoi(buffer);
       make_move(sock,column, *playerID);
     }
   }
	
}

//Functions ----------------------------------------------------------------
void reset(){
  int c,r;

  *playerID = 0;
  choice = 0;
  *gchoice = 0;
  
  for(c = 0; c < 7; c++){
    for(r = 0; r < 7; r++){
      spot[c][r] = 'O';
    }
  }
  printf("\nNew Game! \n");
}

//----------------Send Display--------------//
void send_display(int sock, char s[][7]){
  int r, c, n;
  char buffer[256];
  //printf("display\n");
  bzero(buffer, 256);
  for(r = 0; r < 7; r++){
    strcpy(buffer,"|");
    for(c = 0; c < 7; c++){
      strncat(buffer,&spot[c][r],1);
      strcat(buffer,"|");
    }
    //printf("%s\n",buffer);
    n = write(sock,buffer,strlen(buffer));
    n = read(sock, buffer, 255);
    bzero(buffer, 256);
  }
    n = write(sock,"done",5);
    n = read(sock,buffer,255);
}

//------------------Make Move-------------------//
void make_move(int sock, int choice, int id){
  int r = 0;
  int c = choice;
  int n;
  char player;
  char buffer[256];
  
  printf("%d\n",id);

  if(id == 0)
    player = 'R';
  else
    player = 'B';
  
  while(spot[c][r] == 'O' && r < 7){
    r++;
  }
  
  if(r == 0)
    n = write(sock, "Space is full", 14);
  else
    spot[c][r-1] = player;

  if(r != 0)
    n = write(sock, "ok", 3);
  id++;
  id  = id  % 2;
  *playerID = id;
  //printf("%d",*playerID);
}

bool check_win(char s[][7]){
  int c,r;
  char tmp;

  for(c = 0; c < 7; c ++)
    for(r = 6; r > 2; r--){
      tmp = s[c][r];
      if(s[c][r-1] == tmp && s[c][r-2] == tmp && s[c][r-3] == tmp
	 && tmp != 'O')
	return true;
    }
  for(r = 6; r > -1; r--)
    for(c = 0; c < 4; c++){
      tmp = s[c][r];
      if(s[c+1][r] == tmp && s[c+2][r] == tmp && s[c+3][r] == tmp
	 && tmp != 'O'){
	return true;
      }
    }
  for(c = 0; c < 4; c ++)
    for(r = 6; r > 2; r--){
      tmp = s[c][r];
      if(s[c+1][r-1] == tmp && s[c+2][r-2] == tmp && s[c+3][r-3] == tmp
	 && tmp != 'O')
	return true;
    }
  for(c = 6; c > 2; c --)
    for(r = 6; r > 2; r--){
      tmp = s[c][r];
      if(s[c-1][r-1] == tmp && s[c-2][r-2] == tmp && s[c-3][r-3] == tmp
	 && tmp != 'O')
	return true;
    }

  return false;
}
