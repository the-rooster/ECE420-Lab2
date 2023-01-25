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

pthread_mutex_t lock;

struct arg_struct {
    char **strings;
    long clientFileDescriptor;
};

/**
“XXX-Y-SSSSSS”, where “XXX” is the position, “Y”
is 0 or 1 indicating whether it is a read request (1 for read and 0 for write) and
“SSSSSS” is the string to be written*/
void *HandleClientRequest(void *void_args)
{
    double start, end;

    double total_time = 0.0;
    double transaction_count = 0.0;
    struct arg_struct* args = (struct arg_struct*) void_args;
    //printf("FD: %ld \n", args->clientFileDescriptor);
    ClientRequest* client_request = malloc(sizeof(ClientRequest));
    char * return_str = (char*) malloc(COM_BUFF_SIZE*sizeof(char));
    int ret = 0;
    while (1) {
      char msg[COM_BUFF_SIZE];
      ret = read(args->clientFileDescriptor,msg,COM_BUFF_SIZE);
      if (ret == 0) {
        free(return_str);
        //printf("CLOSING %ld \n", args->clientFileDescriptor);
        close(args->clientFileDescriptor);
        free(args);
        free(client_request);
        if (transaction_count != 0){
            double* average = malloc(sizeof(double));
            //printf("Time sent %f\n", total_time / transaction_count);
            *average = total_time / transaction_count;
            pthread_exit(average);
        }
        pthread_exit(NULL);
      }
      //printf("Parsing message msg: %s \n", msg );
      ParseMsg(msg, client_request);
      //printf("Parsed message msg: %s cli_request_pos: %d , cli_request_msg: %s\n", msg, client_request->pos, client_request->msg );
      GET_TIME(start);
      pthread_mutex_lock(&lock);
      if (client_request->is_read == 1) {
        //printf("Reading at pos %d\n",client_request->pos);
        getContent(return_str,client_request->pos,args->strings);
        write(args->clientFileDescriptor,return_str,COM_BUFF_SIZE);
      } else {
        //printf("Writing at pos %d\n",client_request->pos);
        char *tmp = client_request->msg;
        setContent(tmp, client_request->pos, args->strings);
        write(args->clientFileDescriptor,client_request->msg,COM_BUFF_SIZE);
      }
      //printf("Writing at pos %d\n",client_request->pos);
      pthread_mutex_unlock(&lock);
      GET_TIME(end);

      total_time = total_time + (end - start);
      transaction_count = transaction_count + 1;
    }
}

int main(int argc, char* argv[])
{
    struct sockaddr_in sock_var;
    int serverFileDescriptor=socket(AF_INET,SOCK_STREAM,0);
    long clientFileDescriptor;
    int i;
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        //printf("\n mutex init failed\n");
        return 1;
    }

    //printf("Reading Args\n");
    int n = atoi(*(argv + 1));
    in_addr_t ip = inet_addr(argv[2]);
    int port = atoi(*(argv + 3));
    //printf("Init threads\n");

    pthread_t t[COM_NUM_REQUEST];
    //printf("Init Strings\n");
    char **strings = (char**) malloc(n * sizeof(char*));

    for (i = 0; i < n; i++) {
      strings[i] = (char*) malloc(COM_BUFF_SIZE*sizeof(char));
      sprintf(strings[i],"String %d: the initial value",i);
    }
    
    sock_var.sin_addr.s_addr=ip;
    sock_var.sin_port=port;
    sock_var.sin_family=AF_INET;



    if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        //printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        while(1) {
            double avgs[COM_NUM_REQUEST] = {0.0};
            for(i=0;i<COM_NUM_REQUEST;i++)      //can support COM_NUM_REQUEST clients at a time
            {
                //printf("Connecting to client %ld\n",clientFileDescriptor);
                struct arg_struct* args = malloc(sizeof(struct arg_struct));
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                args->clientFileDescriptor = clientFileDescriptor; args->strings = strings; 
                pthread_create(&t[i],NULL,HandleClientRequest,(void *) args);
                //printf("Connected to client %ld\n",clientFileDescriptor);
            }
            int k;
            for(k=0;k<COM_NUM_REQUEST;k++){
                double* avg;

                pthread_join(t[k],&avg);
                //printf("Time received %f\n", *avg);
                avgs[k] = *avg;
                //printf("Time in array %f\n", avgs[k]);
                if (k == (COM_NUM_REQUEST - 1)){
                    saveTimes(avgs,COM_NUM_REQUEST);
                }
              
            }
            //printf("DONE ITERATION");
        }

        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }
    return 0;
}