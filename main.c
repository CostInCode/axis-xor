#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <math.h>
#include <stdio.h>
#include <capture.h>


#define MAX_CLIENTS 5
#define MY_PORT 9002


void *connection_handler(void *socket_descriptor);

int server_socket;
int client_socket;

int main()
{
    pthread_t connection_thread;

    srand(time(NULL));

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(MY_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    listen(server_socket, MAX_CLIENTS);

    // for each client connecting to server, create a thread
    while((client_socket = accept(server_socket, NULL, NULL)) > 0) {
        if((pthread_create(&connection_thread, NULL, connection_handler, NULL)) > 0)
            return 1;
    }
    return 0;
}


void *connection_handler(void *mythread) {


    /************ ENCRYPTION ********************/
    // get PUBLIC KEY from client
    char modulus[100];
    recv(client_socket, modulus, 100, 0);

    char *mod;
    double modulusDouble;
   
    modulusDouble =  strtod(modulus, &mod);

    char exponent[10];
    char *exp;
    recv(client_socket, exponent, 10, 0);
   
    double public_exponent;
    public_exponent = strtod(exponent, &exp);
    

    // create xor key as a random number of 3 digits
    int xor_key = rand() % 300 + 100;

    syslog(LOG_INFO, "XORRR: %d\n", xor_key);
    double key = (double) xor_key;

    // encrypt rsa/xor_key and sends it
    long double power_res = pow(key, public_exponent);
    double result = fmod(power_res, modulusDouble);
    
    int encryptedXorKey = (int) result;
    char encodedKeyString[10];
    sprintf(encodedKeyString, "%d\n", encryptedXorKey);

    write(client_socket, encodedKeyString, sizeof(encodedKeyString));

    /************ IMAGE ********************/

    char client_message[30];
    recv(client_socket, client_message , 26, 0);

    char decrypted[26];

   /* int i;
    for(i = 0; i < 27; i++) {
        decrypted[i] = client_message[i] ^ encodedKeyString[i % sizeof(encodedKeyString)];
    }*/
    
    media_frame  *frame;
    media_stream *stream;
    void *data;
    size_t size;

    stream = capture_open_stream(IMAGE_JPEG, client_message);

    for(;;) {

        frame = capture_get_frame(stream);
        size = capture_frame_size(frame);
        data = capture_frame_data(frame);

        if(size == NULL) return;

        char *sizefh[100];
        sprintf(sizefh,"%zu\n", size);
   
        size_t sizemg = size;

        char *mysize[100];
        size ^= xor_key;
        sprintf(mysize,"%zu\n", size);
        write(client_socket, mysize, sizeof(mysize));
        
        /************************** IMAGE SENDING ****************************/
        int row = 0;
        unsigned char rowData[sizemg];
        for(row = 0; row < sizemg; row++) {
            rowData[row] = ((unsigned char *)data)[row];
            rowData[row] ^= xor_key;
        }
        if(rowData == NULL) return;

        write(client_socket, rowData, sizeof(rowData));
        //syslog(LOG_INFO, "SENT IMAGE");
        capture_frame_free(frame); 
    }
    
    capture_close_stream(stream); 
    close(client_socket);
}