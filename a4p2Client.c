#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 10010 /*port*/

int
main(int argc, char **argv) 
{
 int sockfd;
 struct sockaddr_in servaddr;
 char sendline[MAXLINE], recvline[MAXLINE];

 // alarm(300);  // to terminate after 300 seconds
	
 //basic check of the arguments
 //additional checks can be inserted
 if (argc != 4) {
  perror("Usage: ./clientName <Server IP> <Server Port> <Log Filename>"); 
  exit(1);
 }
 
 char *logfileName = argv[3];
 int fout = open(logfileName, O_WRONLY | O_CREAT | O_TRUNC, 777);
 close(fout); chmod(logfileName, strtol("0777", 0, 8));
 fout = open(logfileName, O_WRONLY | O_APPEND, 777);
	
 //Create a socket for the client
 //If sockfd<0 there was an error in the creation of the socket
 if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  perror("Problem in creating the socket");
  exit(2);
 }
	
 //Creation of the socket
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = inet_addr(argv[1]);
 servaddr.sin_port =  htons((int) strtol(argv[2], (char **)NULL, 10));
 //servaddr.sin_port =  htons(SERV_PORT); //convert to big-endian order
	
 //Connection of the client to the socket 
 if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
  perror("Problem in connecting to the server");
  exit(3);
 }
  
  char hostNameOut[30] = "hostname >> "; strcat(hostNameOut, logfileName);
  char dateOut[30] = "date >> "; strcat(dateOut, logfileName);
  
 while (fgets(sendline, MAXLINE, stdin) != NULL) {
	system(hostNameOut); system(dateOut);
  send(sockfd, sendline, strlen(sendline), 0);
  char SLoutput[strlen(sendline)+1];
  sprintf(SLoutput, sendline); SLoutput[strlen(sendline)] = NULL; write(fout, SLoutput, sizeof(SLoutput));
  system("hostname; date"); 
	if((sendline[0] == 'E' || sendline[0] == 'e') && (sendline[1] == 'X' || sendline[1] == 'x') && 
      (sendline[2] == 'I' || sendline[2] == 'i') && (sendline[3] == 'T' || sendline[3] == 't')){
    exit(4);
  }
  if (recv(sockfd, recvline, MAXLINE,0) == 0){
   //error: server terminated prematurely
   perror("The server terminated prematurely"); 
   exit(4);
  }
  printf("%s", "String received from the server:\n");
  fputs(recvline, stdout);
  char RLoutput[strlen(recvline)+1];
  sprintf(RLoutput, recvline); RLoutput[strlen(recvline)] = NULL; write(fout, RLoutput, sizeof(RLoutput));
  fputs("\n\n", stdout); write(fout, "\n", 1);
 }
  close(fout);
 exit(0);
}