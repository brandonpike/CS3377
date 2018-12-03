#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/*
    CONCURRENT SERVER: THREAD EXAMPLE
    Must be linked with the "pthread" library also, e.g.:
       cc -o example example.c -lnsl -lsocket -lpthread 

    This program creates a connection socket, binds a name to it, then
    listens for connections to the sockect.  When a connection is made,
    it accepts messages from the socket until eof, and then waits for
    another connection...

    This is an example of a CONCURRENT server -- by creating threads several
    clients can be served at the same time...

    This program has to be killed to terminate, or alternately it will abort in
    120 seconds on an alarm...
    
    
    Each thread will log its activity of each transaction received and processed (with date/time of the day and its 
    server PID and its thread TID). 
    
    
*/

char *logfileName;

#define PORTNUMBER 10011

struct serverParm {
           int connectionDesc;
       };

void *serverThread(void *parmPtr) {

#define PARMPTR ((struct serverParm *) parmPtr)
    int recievedMsgLen;
    char messageBuf[1025];
    /* Server thread code to deal with message processing */
    printf("DEBUG: connection made, connectionDesc=%d\n",
            PARMPTR->connectionDesc);
    if (PARMPTR->connectionDesc < 0) {
        printf("Accept failed\n");
        return(0);    /* Exit thread */
    }
    
    /* Receive messages from sender... */
    char cmdOutput[1025];
    while ((recievedMsgLen=
            read(PARMPTR->connectionDesc,messageBuf,sizeof(messageBuf)-1)) > 0) 
    {
        recievedMsgLen[messageBuf] = '\0';
        strtok(messageBuf, "\n");
        printf("Message: %s\n",messageBuf);
        if((messageBuf[0] == 'E' || messageBuf[0] == 'e') && (messageBuf[1] == 'X' || messageBuf[1] == 'x') && 
            (messageBuf[2] == 'I' || messageBuf[2] == 'i') && (messageBuf[3] == 'T' || messageBuf[3] == 't')){
          exit(1);
        }
        
        int fout = open(logfileName, O_WRONLY | O_APPEND, 777);
        //FILE *fout = fopen(logfileName, "a");
        pthread_t tid = pthread_self();
        char touchFile[20];
        char dateOut[30] = "date >> "; strcat(dateOut, logfileName);
        pid_t pid = getpid();
        system(dateOut);
        //output to log
        char p[12]={0x0}; sprintf(p,"%11d", pid);
        char t[12]={0x0}; sprintf(t,"%lu", tid);
        write(fout, "Process ID: ", strlen("Process ID: ")); write(fout, p, sizeof(p));
        write(fout, "\nThread ID: ", strlen("\nThread ID: ")); write(fout, t, sizeof(t));
        write(fout, "\nCommand: ", strlen("\nCommand: ")); write(fout, &messageBuf, strlen(messageBuf));
        write(fout, "\n", strlen("\n"));
        
        //create temporary file
        strcat(messageBuf, " > temp.txt");
        sprintf(touchFile, messageBuf);
        system(touchFile);
        FILE *fp; long lSize; char *buffer;
        fp = fopen ( "temp.txt" , "rb" );
        if(fp){
          fseek( fp , 0L , SEEK_END);
          lSize = ftell( fp );
          if(lSize > 0){
            rewind( fp );
            /* allocate memory for entire content */
            buffer = calloc( 1, lSize+1 );
            if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);
            /* copy the file into the buffer */
            if( 1!=fread( buffer , lSize, 1 , fp) )
              fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);
              
            sprintf(cmdOutput, buffer);
          }
        }
        fclose(fp);
        free(buffer);
        char RLoutput[strlen(cmdOutput)+1];
        sprintf(RLoutput, cmdOutput); RLoutput[strlen(cmdOutput)] = NULL; write(fout, RLoutput, sizeof(RLoutput));
        write(fout, "\n", strlen("\n"));
        //remove the temporry file
        system("rm temp.txt");
        
        if (write(PARMPTR->connectionDesc,cmdOutput,sizeof(cmdOutput)-1) < 0) {
               perror("Server: write error");
               return(0);
           }
    }
    close(PARMPTR->connectionDesc);  /* Avoid descriptor leaks */
    free(PARMPTR);                   /* And memory leaks */
    return(0);                       /* Exit thread */
}

main (int argc, char **argv) {
    int listenDesc;
    struct sockaddr_in myAddr;
    struct serverParm *parmPtr;
    int connectionDesc;
    pthread_t threadID;
    
    if (argc != 3) {
      perror("Usage: ./serverName <Server Port> <Log Filename>"); 
      exit(1);
    }
    
    logfileName = argv[2];
    int fout = open(logfileName, O_WRONLY | O_CREAT | O_TRUNC, 777);
     close(fout); chmod(logfileName, strtol("0777", 0, 8));

    /* For testing purposes, make sure process will terminate eventually */
    alarm(120);  /* Terminate in 120 seconds */

    /* Create socket from which to read */
    if ((listenDesc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("open error on socket");
        exit(1);
    }

    /* Create "name" of socket */
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = htons((int) strtol(argv[1], (char **)NULL, 10));
        
    if (bind(listenDesc, (struct sockaddr *) &myAddr, sizeof(myAddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    /* Start accepting connections.... */
    /* Up to 5 requests for connections can be queued... */
    listen(listenDesc,5);

    while (1) /* Do forever */ {
        /* Wait for a client connection */
        connectionDesc = accept(listenDesc, NULL, NULL);

        /* Create a thread to actually handle this client */
        parmPtr = (struct serverParm *)malloc(sizeof(struct serverParm));
        parmPtr->connectionDesc = connectionDesc;
        if (pthread_create(&threadID, NULL, serverThread, (void *)parmPtr) 
              != 0) {
            perror("Thread create error");
            close(connectionDesc);
            close(listenDesc);
            exit(1);
        }

        printf("Parent ready for another connection\n");
    }
}