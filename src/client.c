/* This line at the top is necessary for compilation on the lab machine and many other Unix machines.
Please look up _XOPEN_SOURCE for more details. As well, if your code does not compile on the lab
machine please look into this as a a source of error. */
#define _XOPEN_SOURCE

/**
 * client.c
 *  CS165 Fall 2015
 *
 * This file provides a basic unix socket implementation for a client
 * used in an interactive client-server database.
 * The client receives input from stdin and sends it to the server.
 * No pre-processing is done on the client-side.
 *
 * For more information on unix sockets, refer to:
 * http://beej.us/guide/bgipc/output/html/multipage/unixsock.html
 **/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "common.h"
#include "message.h"
#include "utils.h"

#define DEFAULT_STDIN_BUFFER_SIZE 2048
 #define STATE_INFO_LEN 8
#define MAX_SIZE_LINE 256
#define MAX_SIZE_NAME 64

static char hdr_info[MAX_SIZE_LINE];
static int load_file(char* x, char* y, readstate* r);
static int save_header_info(char* x);
char end_token[] = "$BPRNT:";
char dtype_token[] = "$DTYPE:";
char dlen_token[] = "$DLEN:";

/**
 * connect_client()
 *
 * This sets up the connection on the client side using unix sockets.
 * Returns a valid client socket fd on success, else -1 on failure.
 *
 **/
int connect_client() {
    int client_socket = -1;
    size_t len;
    struct sockaddr_un remote;

    log_info("-- Attempting to connect...\n");

    if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        log_err("-- L%d: Failed to create socket.\n", __LINE__);
        return -1;
    }

    remote.sun_family = AF_UNIX;
    strncpy(remote.sun_path, SOCK_PATH, strlen(SOCK_PATH) + 1);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
    if (connect(client_socket, (struct sockaddr *)&remote, len) == -1) {
        log_err("-- client connect failed: ");
        return -1;
    }
    log_info("-- Client connected at socket: %d.\n", client_socket);
    return client_socket;
}


int process_payload(char* payload){
    char curtoken[MAX_SIZE_NAME];
    int bcontinue = 0;
    memcpy(curtoken, payload, strlen(dtype_token));
   
    int offset = 0, dlen=0, type=0, state=0;

    offset += strlen(dtype_token);

    memcpy(&type, payload+offset, sizeof(int));
    offset += sizeof(int);

    memcpy(curtoken, payload+offset, strlen(dlen_token));
    offset += strlen(dlen_token);

    memcpy(&dlen, payload+offset, sizeof(int));
    offset += sizeof(int);

    if (type == 1){
       int value;
       for(int i = 0; i < dlen; i++){
          memcpy(&value, payload+offset, sizeof(int));
          printf("%d\n", value);
          offset += sizeof(int);
       } 
    } else if (type == 2){
       long value = 0;
       for(int i = 0; i < dlen; i++){
          memcpy(&value, payload+offset, sizeof(long));
          printf("%ld\n", value);
          offset += sizeof(long);
       }
    } else if (type == 3 || type == 4){
       double value = 0;
       for(int i = 0; i < dlen; i++){
          memcpy(&value, payload+offset, sizeof(double));
          printf("%.2f\n", value);
          offset += sizeof(double);
       }       
    }         
    memcpy(curtoken, payload+offset, strlen(end_token));
    offset += strlen(end_token);
    memcpy(&state, payload+offset, sizeof(int));
    offset += sizeof(int);
    bcontinue = state == 2 ? 1: 0;
    return bcontinue;
}
void send_receive(int client_socket, message* send_message){
    int len = 0;
    char* ptr = NULL;
    int bcontinue = 1;
    message recv_message;

    // Send the message_header, which tells server payload size
    if (send(client_socket, send_message, sizeof(message), 0) == -1) {
        free(send_message->payload);
        log_err("Failed to send message header.");
        exit(1);
    }

    // Send the payload (query) to server
    if (send(client_socket, send_message->payload, send_message->length, 0) == -1) {
        free(send_message->payload);
        log_err("Failed to send query payload.");
        exit(1);
    }

    while(bcontinue == 1){
        // Always wait for server response (even if it is just an OK message)
        if ((len = recv(client_socket, &(recv_message), sizeof(message), 0)) > 0) {
            if ((recv_message.status == OK_WAIT_FOR_RESPONSE || recv_message.status == OK_DONE) &&
                (int) recv_message.length > 0) {
                // Calculate number of bytes in response package
                int num_bytes = (int) recv_message.length;
                char payload[num_bytes + 1];

                // Receive the payload and print it out
                if ((len = recv(client_socket, payload, num_bytes, 0)) > 0) {
                    payload[num_bytes] = '\0';
                    /*
                    if ((ptr = strstr(payload, "$PRNT:")) !=NULL){
                        int state = atoi(ptr+strlen("$PRNT:"));
                        *ptr = '\0';
                        printf("%s\n", payload);
                        bcontinue = state == 2 ? 1 : 0;
                    }
                    else{
                        printf("%s\n", payload);
                        bcontinue = 0;
                    }
                    */
                    
                    char curtoken[MAX_SIZE_NAME] = "";
                    memcpy(curtoken, payload, strlen(dtype_token));
                    if ((ptr = strstr(curtoken, "$DTYPE:")) !=NULL) {
                        bcontinue = process_payload(payload);
                        
                    } else if ((ptr = strstr(payload, "$PRNT:")) !=NULL){
                        int state = atoi(ptr+strlen("$PRNT:"));
                        *ptr = '\0';
                        printf("%s\n", payload);
                        bcontinue = state == 2 ? 1 : 0;
                    }
                    else{
                        printf("%s\n", payload);
                        bcontinue = 0;
                    }
                    
                }
            }
            else bcontinue = 0;
        }
        else {
            if (len < 0) {
                log_err("Failed to receive message.");
            }
            else {
                log_info("Server closed connection\n");
            }
            exit(1);
        }
    }
}

int main(void)
{
    int client_socket = connect_client();
    if (client_socket < 0) {
        exit(1);
    }

    message send_message;
    //message recv_message;

    // Always output an interactive marker at the start of each command if the
    // input is from stdin. Do not output if piped in from file or from other fd
    char* prefix = "";
    if (isatty(fileno(stdin))) {
        prefix = "db_client > ";
    }

    char *output_str = NULL;
    

    // Continuously loop and wait for input. At each iteration:
    // 1. output interactive marker
    // 2. read from stdin until eof.
    char read_buffer[DEFAULT_STDIN_BUFFER_SIZE] = "";
    send_message.payload = read_buffer;
    send_message.status = OK_DONE;

    while (printf("%s", prefix), output_str = fgets(read_buffer,
           DEFAULT_STDIN_BUFFER_SIZE, stdin), !feof(stdin)) {
        if (output_str == NULL) {
            log_err("fgets failed.\n");
            break;
        }

        // Only process input that is greater than 1 character.
        // Convert to message and send the message and the
        // payload directly to the server.
        send_message.length = strlen(read_buffer);
        if (send_message.length > 1) {

            if (strncmp(send_message.payload, "--", 2) != 0 && strstr(send_message.payload, "load") != NULL){
                message cur_msg;
                readstate rstate = READ_START;
                cur_msg.payload = malloc((DEFAULT_STDIN_BUFFER_SIZE + 1)*sizeof(char));

                while(load_file(send_message.payload, cur_msg.payload, &rstate) != -1){
                    cur_msg.length = strlen(cur_msg.payload);
                    cur_msg.status = OK_DONE;
                    send_receive(client_socket, &cur_msg);
                    memset(cur_msg.payload, 0, DEFAULT_STDIN_BUFFER_SIZE*sizeof(char));
                }
                free(cur_msg.payload);
                memset(send_message.payload, 0, DEFAULT_STDIN_BUFFER_SIZE*sizeof(char));
            }
            else {
                send_receive(client_socket, &send_message);
            }           
        }
    }
    close(client_socket);
    return 0;
}



int load_file(char* command, char* buffer, readstate* read_state){ 
    static FILE *fp = NULL;
    static char load_file_name[MAX_SIZE_LINE];

    char *token;
    char line[MAX_SIZE_LINE] = "";
    char cstate[STATE_INFO_LEN] ="";   
    char filename[MAX_SIZE_NAME] = "";
    int cur_len = 0;

    if (*read_state == READ_END) return -1;

    if (*read_state == READ_START){
        strcpy(filename, command + 4);
        token = trim_parenthesis(filename);
        token = trim_newline(token);
        token = trim_quotes(token);
  
        fp = fopen(token, "r");
        if (fp == NULL){
            //cs165_log(stdout,"could not open file %s:%s", filename ,strerror(errno));
            return -1;
        }
        strcpy(load_file_name, token);
        //first read the header and get the columns
        while(fgets(line, MAX_SIZE_LINE, fp) != NULL){
            if (line[0] == '\0') continue;
            if (save_header_info(line) == 0) break;
        }
    }
    if (fp == NULL || hdr_info[0] == '\0') return -1;
 
    sprintf(buffer, "%s(%s@dt:","load", hdr_info);
    cur_len = strlen(buffer) + STATE_INFO_LEN;
    *read_state = READ_END;
    //now read the data
    while(fgets(line, MAX_SIZE_LINE, fp) != NULL){
        if (line[0] == '\0') continue;
        if (cur_len + strlen(line) > DEFAULT_STDIN_BUFFER_SIZE){
            fseek(fp, -1*strlen(line), SEEK_CUR);
            *read_state = READ_CONTINUE;
            break;
        }
        cur_len += strlen(line);
        token = trim_newline(line);
        strcat(buffer, token);
        strcat(buffer, "$");

    }
    sprintf(cstate,"@rs:%d)",*read_state);
    strcat(buffer, cstate);

    buffer[strlen(buffer)] = '\0';
    if (*read_state == READ_END){
        fclose(fp);
        fp = NULL;   
    }
    return 0;
}

int save_header_info(char* line){
    char *tokenizer_copy;
   
    tokenizer_copy = malloc((strlen(line) + 1)*sizeof(char)); 
    strcpy(tokenizer_copy, line);
    tokenizer_copy = trim_quotes(tokenizer_copy);
    tokenizer_copy = trim_newline(tokenizer_copy);
    strcpy(hdr_info, tokenizer_copy);

    free(tokenizer_copy);
    return 0;
}

