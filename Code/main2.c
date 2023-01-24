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


void *ServerThread(void *args)
{
    int clientFileDescriptor=(int)args;
    char str[COM_BUFF_SIZE];
    char resp[COM_BUFF_SIZE];
    int ret;

    
    printf("STARTING CLIENT THREAD\n");
    ClientRequest* req = malloc(sizeof(ClientRequest));

    printf("HERE?\n");
    while(1){
        ret = read(clientFileDescriptor,str,COM_BUFF_SIZE);

        // printf("CLIENT SENT: %s\n",str);
        printf("HERE2?\n");
        if (ret == 0){
            printf("CONNECTION CLOSED\n");
            //EOF reached
            close(clientFileDescriptor);
            pthread_exit(NULL);
        }       

        ParseMsg(str,req);

        printf("HERE?3\n");
        //do mutex stuff here!

        pthread_mutex_lock(&mutexes[req->pos]);

        printf("MADE IT PAST THE LOCK!\n");
        if(req->is_read){
            
            printf("READING\n");
            getContent(resp,req->pos,strings);
        }
        else{
            printf("WRITING\n");
            setContent(req->msg,req->pos,strings);
            memcpy(resp,req->msg,COM_BUFF_SIZE);
        }
        pthread_mutex_unlock(&mutexes[req->pos]);


        printf("HERE4?\n");
        printf("reading from client:%s\n",str);
        printf("responding with: %s\n",resp);
        write(clientFileDescriptor,resp,COM_BUFF_SIZE);


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

    //set initial values of strings

    for(int i = 0;i<size;i++){
        char str[COM_BUFF_SIZE];
        sprintf(str,"String %d: the initial value",i);
        setContent(str,i,strings);
    }
}


int main(int argc, char* argv[])
{

    int size = atoi(argv[1]);
    int port = atoi(argv[3]);

    //initialize array of strings
    InitStringArray(size);

    printf("STARTING SERVER ON PORT %d WITH STRING ARRAY SIZE %d WITH IP %s\n",port,size,argv[2]);

    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;
    int i;
    pthread_t t[COM_NUM_REQUEST];

    sock_var.sin_addr.s_addr=inet_addr(argv[2]);
    sock_var.sin_port=port;
    sock_var.sin_family=AF_INET;
    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        while(1)        //loop infinity
        {

            for(i=0;i<COM_NUM_REQUEST;i++)      //can support COM_NUM_REQUEST threads at a time
            {
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                printf("Connected to client %d\n",clientFileDescriptor);
                pthread_create(&t[i],NULL,ServerThread,(void *)(long)clientFileDescriptor);
                printf("THREAD %d STARTED\n",clientFileDescriptor);
            }

            for(i=0;i<COM_NUM_REQUEST;i++){
                pthread_join(t[i],NULL);
            }
            
        }
        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }
    return 0;
}