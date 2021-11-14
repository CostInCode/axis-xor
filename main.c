#include <arpa/inet.h>
#include <capture.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>
#define MAX_CLIENTS 5
#define MY_PORT 9002

void *connection_handler(void *socket_descriptor);

int server_socket;
int client_socket;

int main() {
  pthread_t connection_thread;

  srand(time(NULL));

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(MY_PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;

  bind(server_socket, (struct sockaddr *)&server_address,
       sizeof(server_address));
  listen(server_socket, MAX_CLIENTS);

  // for each client connecting to server, create a thread
  while ((client_socket = accept(server_socket, NULL, NULL)) > 0) {
    if ((pthread_create(&connection_thread, NULL, connection_handler, NULL)) >
        0)
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

  modulusDouble = strtod(modulus, &mod);

  char exponent[10];
  char *exp;
  recv(client_socket, exponent, 10, 0);

  double public_exponent;
  public_exponent = strtod(exponent, &exp);

  // create xor key as a random number of 3 digits
  int xor_key = rand() % 300 + 100;
  char keyString[3];
  sprintf(keyString, "%d\n", xor_key);

  syslog(LOG_INFO, "XORRR: %d\n", xor_key);
  double key = (double)xor_key;

  // encrypt rsa/xor_key and sends it
  long double power_res = pow(key, public_exponent);
  double result = fmod(power_res, modulusDouble);
  int encryptedXorKey = (int)result;
  char encodedKeyString[10];
  sprintf(encodedKeyString, "%d\n", encryptedXorKey);
  write(client_socket, encodedKeyString, sizeof(encodedKeyString));

  /************ IMAGE ********************/

  // first receive size of specs // java specs.length and sleep(500)
  char decodedSpecsSize[3];
  recv(client_socket, decodedSpecsSize, 3, 0);
  int specsSize = strtol(decodedSpecsSize, NULL, 10);

  // now receive specs
  char encSpecs[40];
  recv(client_socket, encSpecs, 40, 0);

  // decrypt specifications
  int i;
  for (i = 0; encSpecs[i] != '\0'; i++) {
    encSpecs[i] ^= keyString[i % sizeof(keyString)];
  }

  media_frame *frame;
  media_stream *stream;
  void *data;
  size_t size;

  stream = capture_open_stream(IMAGE_JPEG, encSpecs);

  for (;;) {

    frame = capture_get_frame(stream);
    sizeImg = capture_frame_size(frame);
    data = capture_frame_data(frame);

    if (size == NULL)
      return;

    // send encoded image size
    sizeImg ^= xor_key;
    char *sizeImgStrEnc[100];
    sprintf(sizeImgStrEnc, "%zu\n", sizeImg);
    write(client_socket, sizeImgStrEnc, sizeof(sizeImgStrEnc));

    /************************** send Encoded IMAGE ****************************/
    int row = 0;
    unsigned char rowData[sizeImg];
    for (row = 0; row < sizeImg; row++) {
      rowData[row] = ((unsigned char *)data)[row];
      rowData[row] ^= xor_key;
    }
    if (rowData == NULL)
      return;

    write(client_socket, rowData, sizeof(rowData));
    capture_frame_free(frame);
  }

  capture_close_stream(stream);
  close(client_socket);
}