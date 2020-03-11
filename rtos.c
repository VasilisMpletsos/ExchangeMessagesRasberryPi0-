#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <netdb.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include<arpa/inet.h>
#include<time.h>
#define port 2288 



/*
TODO LIST:
1. Make report
2. End!!!!!


Notes:  Dont forget << gcc rtos.c -o rtos -lpthread >>

*/


//Global Variables
char table[2000][1000]; //I think this is a logic length for each message
bool flag = false;
char *IPtable[4] = {"10.0.85.23","10.0.87.07","10.0.87.42","10.0.85.59"};
int lines; //Knows how many messages we have so far!;




//Declarations
char *Message();
short SocketCreate(void);
int SocketConnect(int hSocket ,char *IP);
int SocketSend(int hSocket,char* Rqst,short lenRqst);
int SocketReceive(int hSocket,char* Rsp,short RvcSize);
int BindCreatedSocket(int hSocket);
void createfile(void);
void readData(void);
void ReconstructList(void);
void writeMessage(char *update);
void createStatistic(void);
void writeStatistic(char *statistic);

//Threads on for server one for client in order to work simultaneously 
void *myThreadClient(void *vargp) 
{ 
	int *myid = (int *)vargp;
	printf("Client Started\n");
	
	//Client Creation
	int hSocket, read_size;
	struct sockaddr_in server;
	char ReceivedMessage[277] = {0} ;
	memset(ReceivedMessage,0,sizeof(ReceivedMessage));
	for(int i=0;i<4;i++){
		hSocket = SocketCreate();
		if(hSocket == -1)
		{
			printf("Could not create socket\n");
			exit(1);
		}
		printf("Socket is created -- Client\n");
		if ((SocketConnect(hSocket,IPtable[i])) != -1)
		{
			writeStatistic(IPtable[i]);
			printf("Connect found with ip %s -- Client\n",IPtable[i]);
			read_size = SocketReceive(hSocket , ReceivedMessage , 200);
			printf("Server Response %s \n",ReceivedMessage);
			
			//Search for duplicate
			bool duplicate = false;
			for(int i=0;i<lines;i++)
			{
				if (strncmp(ReceivedMessage, table[i], strlen(ReceivedMessage)) == 0)
				{
					duplicate = true;
				}
			}

			// If not cast it into the data file;
			if (duplicate == false)
			{
				if(lines<2000){
					writeMessage(ReceivedMessage); //Writes Last message
				}else{
					ReconstructList(); //It pushes the list up 
					writeMessage(ReceivedMessage); //Writes Last message
				}
			}
			memset(ReceivedMessage,0,sizeof(ReceivedMessage));	
			readData(); //Updates the table
			
			//Socket Close and TCP sequence shutdown
			close(hSocket);
			shutdown(hSocket,0);
			shutdown(hSocket,1);
			shutdown(hSocket,2);
		}else{
			printf("Didn't find ip %s -- Client\n",IPtable[i]);
		}
	}
}


void *myThreadServer(void *vargp) 

{ 
	int loops=0;
	bool first_time = true;
	int *myid = (int *)vargp;
	printf("Server Started\n");
	//Server Creation
	int socket_desc , sock , clientLen , read_size;
	struct sockaddr_in server , client;
	
	//Create socket		
	socket_desc = SocketCreate();
	if (socket_desc == -1)
	{
		printf("Could not create socket");
		exit(1);
	}else{
		printf("Socket created -- Server\n");
	}
		
	if( BindCreatedSocket(socket_desc) < 0)
	{
		
		 printf("bind failed.");
		 exit(1);
	}
	printf("Bind done -- Server\n");
	listen(socket_desc , 3); //To have 3 pending queues!
	while(1)
	{
		printf("Waiting for incoming connections...\n");
		clientLen = sizeof(struct sockaddr_in);
		//accept connection from an incoming client
		char client_message[1000] = {0};
		sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&clientLen);
		if (sock < 0)
		{
			printf("accept failed");
			exit(1);
		}else{
			if(first_time == true)
			{	
				strcpy(client_message,Message());
				first_time = false;
			}else{
				if(loops<lines)
				{
					strcpy(client_message,table[loops]);
					loops++;
				}else{
					first_time = true;
					loops = 0;
					strcpy(client_message,table[loops]);
				}
				
				
			}
			printf("Connection accepted -- Server\n");
			if( send(sock , client_message , strlen(client_message) , 0) < 0)
			{
				printf("Send failed -- Server");
				exit(1);
			}else{
				printf("Send Done -- Server");
			}
			memset(client_message,0,sizeof(client_message));
			close(sock);
			sleep(1);
			}
	}
	
   	 return NULL; 
} 


//Here we start the program
int main()
{
	createfile(); //If there is no data file . create it!
	readData();  //Retrieves so far contracted table of messages
	createStatistic(); //Create file for statistics
	pthread_t thread_id,thread_id2;
	pthread_create(&thread_id2, NULL, myThreadServer, (void *)&thread_id2);
	while(true){
		pthread_create(&thread_id, NULL, myThreadClient, (void *)&thread_id);
		pthread_join(thread_id, NULL);
		sleep(1);
	}
	
    exit(0);
}

char *Message()
{
	//Decleration of data
	int32_t AEM = 8687;
	int32_t tAEM = 8707;
	char *under = "_";
	char *part1;
	char *part2;
	char *part3;
	char message[256] = "Hello There Vasilis";
	char *send;
	char merged[277]={0};

	//Merging Message required to send
	sprintf(merged,"%d_%d_%lu_%s", AEM, tAEM, time(NULL), message);
	char *str_to_ret = malloc (sizeof (char) * strlen(merged));
	strcpy(str_to_ret,merged);
	return str_to_ret;
}

short SocketCreate(void)
{

        short hSocket;
        printf("Create the socket\n");
        hSocket = socket(AF_INET, SOCK_STREAM, 0);
        return hSocket;
}

int SocketConnect(int hSocket ,char *IP)
{

        int iRetval=-1;
        int ServerPort = (int)port;
        struct sockaddr_in remote={0};
        remote.sin_addr.s_addr = inet_addr(IP); 
        remote.sin_family = AF_INET;
        remote.sin_port = htons(ServerPort);
        iRetval = connect(hSocket , (struct sockaddr *)&remote , sizeof(struct sockaddr_in));
        return iRetval;
}

int SocketSend(int hSocket,char* Rqst,short lenRqst)

{

        int shortRetval = -1;
        struct timeval tv;
        tv.tv_sec = 20;  /* 20 Secs Timeout */
        tv.tv_usec = 0;  
        if(setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(tv)) < 0)
        {
          printf("Time Out\n");
          return -1;
        }
        shortRetval = send(hSocket , Rqst , lenRqst , 0);
        return shortRetval;
 }
 
 int SocketReceive(int hSocket,char* Rsp,short RvcSize)
{

	int shortRetval = -1;
	struct timeval tv;
	tv.tv_sec = 20;  /* 20 Secs Timeout */
	tv.tv_usec = 0;  
	if(setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(tv)) < 0)
	{
	 printf("Time Out\n");
	  return -1;

	}
	shortRetval = recv(hSocket, Rsp , RvcSize , 0);
	return shortRetval;
 }
 
 int BindCreatedSocket(int hSocket)
{

	int iRetval=-1;
	int ClientPort = (int)port;
	struct sockaddr_in  remote={0};


	remote.sin_family = AF_INET; /* Internet address family */
	remote.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	remote.sin_port = htons(ClientPort); /* Local port */
	iRetval = bind(hSocket,(struct sockaddr *)&remote,sizeof(remote));

        return iRetval;
}

void createfile(void)
{
	FILE *fptr;
	fptr = fopen("data.txt", "rb+");
	if(fptr == NULL) //if file does not exist, create it
	{
		fptr = fopen("data.txt", "wb");
	}
	return;
}

void readData(void)
{
	FILE *myfile = fopen("data.txt", "r");
    if (myfile == NULL) {
        printf("Cannot open file.\n");
        return;
    }
    else {
		int n_lines = 0;              
		while(fgets(table[n_lines], 2000, myfile)!=NULL){ 
			strtok(table[n_lines], "\n");
			printf("%s\n", table[n_lines]);	
			n_lines++;
		}
		lines = n_lines;
		fclose(myfile);
	}
	return;
}

void writeMessage(char *update)
{
	 FILE *fp = fopen("data.txt","a");
	 if ( fp == 0 )return;
	 fprintf(fp, "%s", update);
	 fprintf(fp,"\n");
	 fclose(fp);
	 return;
}

void ReconstructList(void)
{
	printf("Reconstructing List");
	FILE *fp = fopen("data.txt","w");
	if ( fp == 0 )return;
	for(int i=0;i<lines-2;i++)
	{
			stpcpy(table[i],table[i+1]);
			fprintf(fp, "%s", table[i]);
			fprintf(fp,"\n");
	}
	stpcpy(table[lines-2],table[lines-1]);
	fprintf(fp, "%s", table[lines-2]);
	fprintf(fp,"\n");
	fclose(fp);
	return;
}


void createStatistic(void)
{
	FILE *fptr;
	fptr = fopen("statistics.txt", "rb+");
	if(fptr == NULL) //if file does not exist, create it
	{
		fptr = fopen("statistics.txt", "wb");
	}
	return;
}

void writeStatistic(char *statistic)
{
	FILE *fp = fopen("statistics.txt","a");
	if ( fp == 0 )return;
	char merged2[277]={0};
	sprintf(merged2,"%s %lu", statistic, time(NULL));
	fprintf(fp, "%s", merged2);
	fprintf(fp,"\n");
	fclose(fp);
	return;
}