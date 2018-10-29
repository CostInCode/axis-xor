#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <capture.h>

#define MAX_SIZE 50
#define MAX_CLIENTS 5
#define MY_PORT 9002


void *connection_handler(void *socket_descriptor);

int server_socket;
int client_socket;

int main()
{
    pthread_t connection_thread;

    // CONNECT THE SERVER
   

    // create server socket tcp
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(MY_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    // listen for connections
    listen(server_socket, MAX_CLIENTS);

    // for each client connecting to server, create a thread
    while((client_socket = accept(server_socket, NULL, NULL)) > 0) {
        if((pthread_create(&connection_thread, NULL, connection_handler, NULL)) > 0)
            return 1;
    }
    //pthread_exit(0);
    return 0;
}


void *connection_handler(void *mythread) {

    char client_message[2000];
    char specifications[2000];
    
        
    int read_size;
   
        
    // while server keeps receiving message from client
    while((read_size= recv(client_socket, client_message , 2000 , 0)) > 0) 
    {

        sprintf(specifications, "%s", client_message);
        media_frame  *frame;
        media_stream *stream;
        void *data;
        size_t size;
        stream = capture_open_stream(IMAGE_JPEG, specifications);
        syslog(LOG_INFO, specifications);

        // get size and data
        frame = capture_get_frame(stream);
        size = capture_frame_size(frame);
        data = capture_frame_data(frame);

        char *mysize[100];
        // format the size to be sent
        sprintf(mysize,"%zu\n", size);
        syslog(LOG_INFO, mysize);
        // send size to client
        write(client_socket, mysize, sizeof(mysize));

        // prepare data array[size]
        int row = 0;
        unsigned char rowData[size];
        for(row = 0; row < size; row++) {
            rowData[row] = ((unsigned char *)data)[row];
        }

        // send photo to client
        int ret = write(client_socket, rowData, sizeof(rowData));
        syslog(LOG_INFO, "SENT = %d",ret);
       
    
        capture_frame_free(frame);
        capture_close_stream(stream); 
    }
    
    

    
    // close the server
    //close(client_socket);
    //pthread_exit(0);
   
}



