#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include "common.h"


char** strings;

pthread_mutex_t* mutexes;


void *ServerEcho(void *args)
{
    int clientFileDescriptor=(int)args;
    char str[COM_BUFF_SIZE];
    int ret = read(clientFileDescriptor,str,20);
    while(ret != 0){
        ret = read(clientFileDescriptor,str,20);
        printf("reading from client:%s\n",str);
        write(clientFileDescriptor,str,20);
    }

    close(clientFileDescriptor);
    return NULL;
}


void* InitStringArray(int size){
    //size: number of strings in array

    strings = malloc(sizeof(char*) * size);

    mutexes = malloc(sizeof(pthread_mutex_t) * size);

    for(int i = 0;i<size;i++){
        //initialize each string
        strings[i] = malloc(sizeof(char) * COM_BUFF_SIZE);
        pthread_mutex_init((mutexes + i),NULL);
    }
}


int main(int argc, char* argv[])
{

    int size = atoi(argv[1]);
    int 
    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;
    int i;
    pthread_t t[20];

    sock_var.sin_addr.s_addr=inet_addr("127.0.0.1");
    sock_var.sin_port=3000;
    sock_var.sin_family=AF_INET;
    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        while(1)        //loop infinity
        {
            for(i=0;i<20;i++)      //can support 20 clients at a time
            {
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                printf("Connected to client %d\n",clientFileDescriptor);
                pthread_create(&t[i],NULL,ServerEcho,(void *)(long)clientFileDescriptor);
            }
        }
        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }
    return 0;
}