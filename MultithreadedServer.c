/**
 * @author : Ferhat Kortak 
 * School Number : 2015510099
 * e-mail : ferhat.kortak@ceng.deu.edu.tr
 * 2018/2019 Fall Semester
 * Operating System 
 * Homework II
 * Dept. Of Computer Engineering
 * 
 * Sources
 * https://github.com/CorySavit/Multithreaded-Web-Server/blob/master/server.c
 * https://github.com/pminkov/webserver/blob/master/server.c
 * http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
 * http://www.cse.chalmers.se/edu/year/2012/course/_courses_2011/EDA343/Assignments/Assignment1/Assignment1_presentation.pdf
 * 
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>

// CHANGE HERE -------------------------------------------
#define PATH "/home/cme2002/Desktop/serverfolder/" // Warning : PATH string must end with a slash / character 
// CHANGE HERE -------------------------------------------
#define MYPORT 2222

#define BUF_SIZE 1024
#define OK_IMAGE    "HTTP/1.0 200 OK\nContent-Type:image/gif\n\n"
#define NOTOK_301   "HTTP/1.0 301 Moved Permenantly\nContent-Type:text/html\n\n"
#define NOTOK_404   "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n<html><body><h1>File Not Found</h1></body></html>"
#define NOTOK_400   "HTTP/1.0 404 Bad Request\nContent-Type:text/html\n\n<html><body><h1>Bad Request</h1></body></html>"
#define NOTOK_503   "HTTP/1.0 503 Server is Busy\nContent-Type:text/html\n\n<html><body><h1>Server is Busy</h1></body></html>"
#define MAX_REQUEST 10

sem_t totalConnections;

void *connection(void *socket_desc)
{
    /*
    Steps of the connection function
    (0) Initializing the variables 
    (1) Receive the request, then store it in a buffer 
    (2) Identify the request type : GET, POST etc. {Parsing} 
    (3) Parse the requested file name and path 
    (4) Read requested file, prepare a buffer string varriable to SEND method
    (5) SEND the requested file to browser via HTTP 1.1 Protocol 
    */

    // Variables
    // File
    FILE *_file;
    char recv_buf[1024]; // received string
    char send_buf_temp[1024];
    char *send_buf; // Response
    char extension[4];
    char file_name[100];
    char *file_data; // ???
    char error_msg[50], output[100];
    char temp_filename[100];
    int outputsize = 0, recv_len = 0, file_size, temp_recv = 0;
    // To handle race condition
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    /* (0) Initializing the variables */
    int sock = *(int *)socket_desc;

    /* (1) Receive the requested message */
    // Read 1024 byte from the requested message which is created by a browser
    temp_recv = (recv(sock, recv_buf, sizeof(recv_buf), 0));
    //printf("Recv : %s", recv_buf); //Debug receive

    // Error Handling
    // For empty requests
    if (temp_recv < 0)
    {
        printf("Receive Error");
        exit(errno);
    }

    /* (2) Identify the request type : GET, POST etc. {Parsing} */
    // Parse the first 4 char to identify GET or NOT
    char command[4];
    strncpy(command, recv_buf, 4);

    if (strncmp(command, "GET", 3) == 0)
    { // Request type is GET

        // Parse the requested file type and name with PATH
        int j = 0, extension_flag = 0;
        int i = 5; // Start after GET command
        while (recv_buf[i] != ' ')
        {
            if (extension_flag == 1)
            {
                extension[j] = recv_buf[i];
                j++;
            }
            if (recv_buf[i] == '.')
                extension_flag = 1;
            
            file_name[i - 5] = recv_buf[i];
            i++;
        }
        file_name[i] = '\0';

        printf("FILENAME : %s\n", file_name);
        printf("EXTENSION : %s\n", extension);

        /* (4) Read requested file */
        if(strcmp(file_name,"favicon.ico")!=0){

           sprintf(temp_filename,"%s%s",PATH,file_name);
           strcpy(file_name,temp_filename);
         
           _file = fopen(file_name, "r");

            if (_file == NULL)
            {
                  // 404 NOT FOUND
                  char* notFound;
                  notFound = (char *)malloc(150);
                  strcpy(notFound, NOTOK_404);
                  printf("\n404 NOT FOUND\n");
                  send(sock, notFound, 150, 0);
                  free(notFound); //free the allocated memory for the send buffer
            }
            else
            {
                  fseek(_file, 0, SEEK_END);                 //seek to the end of the tfile
                  file_size = ftell(_file);                  //get the byte offset of the pointer(the size of the file)
                  fseek(_file, 0, SEEK_SET);                 //seek back to the begining of the file
                  file_data = (char *)malloc(file_size + 1); //allocate memmory for the file data
                  fread(file_data, 1, (file_size), _file);   //read the file data into a string
                  file_data[file_size] = '\0';

                  if (strcmp(extension, "html") == 0)
                  {
                     sprintf(send_buf_temp, "HTTP/1.1 200 OK\nContent Length: %d\nConnection: close\nContent-Type: text/html\n\n%s\n\n", file_size, file_data); //format and create string for output to client                                                                                                                      //copy to the correctly sized string                                                                                                                     //send the info to the client
                     int size_buf = strlen(send_buf_temp);
                     send_buf = (char *)malloc(size_buf);
                     strcpy(send_buf, send_buf_temp);
                     send(sock, send_buf, size_buf, 0);
                     //free(send_buf); //free the allocated memory for the send buffer

                  }
                  else if (strcmp(extension, "jpeg") == 0)
                  {
                     char img_buf[BUF_SIZE];
                     strcpy(img_buf, OK_IMAGE);
                     send(sock, img_buf, strlen(img_buf), 0);

                     for (i = 0; i < file_size; i++)
                     {
                        send(sock, &file_data[i], sizeof(char), 0);
                     }
                  }
                  pthread_mutex_unlock(&mutex);
            }
        }else{
            sem_post(&totalConnections);
            close(sock);
            pthread_exit(NULL);
        }
    }
    else
    {
        // 400 Bad Request 
        strcpy(error_msg, NOTOK_400);
        send(sock, error_msg, 50, 0);
    }
                   
    printf("END OF REQUEST\n\n");
    sem_post(&totalConnections);
    close(sock);
    //sleep(5);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int socket_num, new_socket;
    int connectionStatus[MAX_REQUEST];
    struct sockaddr_in socketaddress;
    pthread_t connection_threads[MAX_REQUEST];
    int i = 0;
    int currentRequests = 0;
    // Initialize Semaphores
    sem_init(&totalConnections, 0, 10);
    if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket Error\n");
        return errno;
    }

    socketaddress.sin_family = AF_INET;
    socketaddress.sin_addr.s_addr = INADDR_ANY;
    socketaddress.sin_port = htons(MYPORT);

    if (bind(socket_num, (struct sockaddr *)&socketaddress, sizeof(socketaddress)) < 0)
    {
        printf("Binding failed\n");
        return 1;
    }
    puts("Waiting for incoming connections");

    do
    {
        sem_getvalue(&totalConnections, &currentRequests);
        currentRequests = MAX_REQUEST - currentRequests;
         // Listen for a request
         if (listen(socket_num, 10) != 0)
         {
            printf("Listen Error\n");
         }
         printf("Current requests : %d \n", currentRequests);
         // Accept a request
         connectionStatus[currentRequests] = accept(socket_num, NULL, NULL);
        if(currentRequests<10)
        {
            if (connectionStatus[currentRequests] == 0)
            {
                printf("Connection Error\n");
            }
            else // Create a thread for request
            {
                sem_wait(&totalConnections);
                pthread_create(connection_threads + currentRequests, NULL, &connection, &connectionStatus[currentRequests]);
                //pthread_join(connection_threads[currentRequests], NULL);   
            }
        }else {
            printf("Server is too busy\n");
            char message_buffer[200]; // Response
            // 503 NOT FOUND
            strcpy(message_buffer, NOTOK_503);
            send(connectionStatus[currentRequests] , message_buffer, strlen(message_buffer), 0);
	    close(connectionStatus[currentRequests]);
        }
    } while (1);
    return 0;
}
/**
 * APPENDIX
 * 
 * recv : Upon successful completion, recv() shall return the length of the message in bytes
 *        Second parameter gives the request message
 * 
 * 200 OK
 * request succeeded, requested object later in this message
 * 
 * 301 Moved Permanently
 * requested object moved, new location specified later in this message (Location:)
 * 
 * 400 Bad Request
 * request message not understood by server
 * 
 * 404 Not Found
 *requested document not found on this server
 *
 * 
*/

/*
SHELL SCRIPT FILE

#!/bin/sh
google-chrome "http://localhost:2222/happy_dog.jpeg" "http://localhost:2222/sky.jpeg" "http://localhost:2222/index.html" "http://localhost:2222/index.html" "http://localhost:2222/index.html" "http://localhost:2222/index.html" "http://localhost:2222/index.html" "http://localhost:2222/index.html" "http://localhost:2222/index.html" "http://localhost:2222/index.html"
sleep 2
google-chrome "http://localhost:2222/sky.jpeg" "http://localhost:2222/happy-dog.jpeg"

// TO RUN
chmod +x /path/to/yourscript.sh
./yourscript.sh

// Warning :  Please do not forget change PORT NUMBER in shell script file!
*/
