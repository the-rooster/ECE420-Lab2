#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include "common.h"
#include "timer.h"

char** strings;

pthread_rwlock_t* read_write_lock;

pthread_mutex_t time_write_lock;




void *ServerThread(void *args)
{
    int clientFileDescriptor=(int)args;
    char str[COM_BUFF_SIZE];
    char resp[COM_BUFF_SIZE];
    int ret;
    double start, end;

    double total_time = 0.0;
    double transaction_count = 0.0;



    
  //printf("STARTING CLIENT THREAD\n");
    ClientRequest* req = malloc(sizeof(ClientRequest));

  //printf("HERE?\n");
    while(1){
        ret = read(clientFileDescriptor,str,COM_BUFF_SIZE);

        // printf("CLIENT SENT: %s\n",str);
      //printf("HERE2?\n");
        if (ret == 0){
          //printf("CONNECTION CLOSED\n");
            //EOF reached
            close(clientFileDescriptor);

            // pthread_mutex_lock(&time_write_lock);

            // saveTimes(times,transaction_count);

            // pthread_mutex_unlock(&time_write_lock);
            if (transaction_count != 0){
                double* average = malloc(sizeof(double));
                *average = total_time / transaction_count;
                pthread_exit(average);
            }
            else{
                pthread_exit(NULL);
            }

        }       

        ParseMsg(str,req);
        

        GET_TIME(start);
        //start timer


        if(req->is_read){
            pthread_rwlock_rdlock(&read_write_lock[req->pos]);
          //printf("READING\n");
            getContent(resp,req->pos,strings);
        }
        else{
          //printf("WRITING\n");
            pthread_rwlock_wrlock(&read_write_lock[req->pos]);
            setContent(req->msg,req->pos,strings);
            memcpy(resp,req->msg,COM_BUFF_SIZE);
        }
        pthread_rwlock_unlock(&read_write_lock[req->pos]);

        GET_TIME(end);

        total_time = total_time + (end - start);
        // printf("TOTAL TIME: %f",total_time);
        transaction_count = transaction_count + 1;



      //printf("reading from client:%s\n",str);
      //printf("responding with: %s\n",resp);
        write(clientFileDescriptor,resp,COM_BUFF_SIZE);


    }

    close(clientFileDescriptor);
    return NULL;
}


void* InitStringArray(int size){
    //size: number of strings in array

    strings = malloc(sizeof(char*) * size);

    read_write_lock = malloc(sizeof(pthread_rwlock_t) * size);
    int i;
    for(i = 0;i<size;i++){
        //initialize each string
        strings[i] = malloc(sizeof(char) * COM_BUFF_SIZE);
        
        
        pthread_rwlock_init((read_write_lock + i),NULL);
    }

    //set initial values of strings

    for(i = 0;i<size;i++){
        char str[COM_BUFF_SIZE];
        sprintf(str,"String %d: the initial value",i);
        setContent(str,i,strings);
    }
}


int main(int argc, char* argv[])
{

    int size = atoi(argv[1]);
    int port = atoi(argv[3]);


    double avgs[COM_NUM_REQUEST] = {0.0};

    //initialize array of strings
    InitStringArray(size);

    //initialize write lock for saving times
    pthread_mutex_init(&time_write_lock,NULL);

  //printf("STARTING SERVER ON PORT %d WITH STRING ARRAY SIZE %d WITH IP %s\n",port,size,argv[2]);

    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;
    int i;
    pthread_t t[COM_NUM_REQUEST];

    sock_var.sin_addr.s_addr=inet_addr(argv[2]);
    sock_var.sin_port=port;
    sock_var.sin_family=AF_INET;

    if(0 > setsockopt(serverFileDescriptor,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int))){
      printf("sockopts failed!\n");
    }

    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
      //printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        while(1)        //loop infinity
        {

            for(i=0;i<COM_NUM_REQUEST;i++)      //can support COM_NUM_REQUEST threads at a time
            {
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
              //printf("Connected to client %d\n",clientFileDescriptor);
                pthread_create(&t[i],NULL,ServerThread,(void *)(long)clientFileDescriptor);
              //printf("THREAD %d STARTED\n",clientFileDescriptor);
            }

            for(i=0;i<COM_NUM_REQUEST;i++)      //can support COM_NUM_REQUEST threads at a time
            {
                double* avg;

                pthread_join(t[i],&avg);

                avgs[i] = *avg;

                free(avg);
                
            }

            saveTimes(avgs,COM_NUM_REQUEST);

            
        }
        close(serverFileDescriptor);
    }
    else{
      //printf("socket creation failed\n");
    }
    return 0;
}