/** server.c
 * CS165 Fall 2015
 *
 * This file provides a basic unix socket implementation for a server
 * used in an interactive client-server database.
 * The client should be able to send messages containing queries to the
 * server.  When the server receives a message, it must:
 * 1. Respond with a status based on the query (OK, UNKNOWN_QUERY, etc.)
 * 2. Process any appropriate queries, if applicable.
 * 3. Return the query response to the client.
 *
 * For more information on unix sockets, refer to:
 * http://beej.us/guide/bgipc/output/html/multipage/unixsock.html
 **/
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"
#include "cs165_api.h"
#include "message.h"
#include "utils.h"
#include "client_context.h"


static int is_server_running;

void send_data(int client_socket, message* send_message){
    // 3. Send status of the received message (OK, UNKNOWN_QUERY, etc)
    if (send(client_socket, send_message, sizeof(message), 0) == -1) {
        log_err(strerror(errno));
        log_err("Failed to send message.");
        free(send_message->payload);
        exit(1);
    }

    // 4. Send response of request
    if (send(client_socket, send_message->payload, send_message->length, 0) == -1) {
        log_err(strerror(errno));
        log_err("Failed to send message.");
        free(send_message->payload);
        exit(1);
    } 
}
/**
 * handle_client(client_socket)
 * This is the execution routine after a client has connected.
 * It will continually listen for messages from the client and execute queries.
 **/
void handle_client(int client_socket, ClientContext* client_context, int* tofree) {
    int done = 0;
    int length = 0;

    log_info("Connected to socket: %d.\n", client_socket);

    // Create two messages, one from which to read and one from which to receive
    message send_message;
    message recv_message;
   
    // Continually receive messages from client and execute queries.
    // 1. Parse the command
    // 2. Handle request if appropriate
    // 3. Send status of the received message (OK, UNKNOWN_QUERY, etc)
    // 4. Send response of request.
    do {
        length = recv(client_socket, &recv_message, sizeof(message), 0);
        if (length < 0) {
            log_err("Client connection closed!\n");
            exit(1);
        } else if (length == 0) {
            done = 1;
        }

        if (!done) {
            Status status;
            status.code = OK;

            char recv_buffer[recv_message.length + 1];
            length = recv(client_socket, recv_buffer, recv_message.length,0);
            recv_message.payload = recv_buffer;
            recv_message.payload[recv_message.length] = '\0';

            // 1. Parse command
            DbOperator* query = parse_command(recv_message.payload, &send_message, client_socket, client_context);
                
            send_message.payload = NULL;
            send_message.length = 0; 
            if (query && query->isbatched == 0) { 
                // 2. Handle request
                status = execute_DbOperator(query, &send_message);
                while ( status.code == INCOMPLETE){               
                    send_data(client_socket, &send_message);
                    status = execute_DbOperator(query, &send_message);
                }
            }
            send_data(client_socket, &send_message);
            if (query && query->type == CLOSE)*tofree = 0;
            db_operator_free(query, &is_server_running);
                 
            free(send_message.payload);            
        }
    } while (!done);
    //db_clear_client_context(client_context);
    log_info("Connection closed at socket %d!\n", client_socket);
    close(client_socket);
}

/**
 * setup_server()
 *
 * This sets up the connection on the server side using unix sockets.
 * Returns a valid server socket fd on success, else -1 on failure.
 **/
int setup_server() {
    int server_socket;
    size_t len;
    struct sockaddr_un local;

    log_info("Attempting to setup server...\n");

    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        log_err("L%d: Failed to create socket.\n", __LINE__);
        return -1;
    }

    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, SOCK_PATH, strlen(SOCK_PATH) + 1);
    unlink(local.sun_path);

    /*
    int on = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        log_err("L%d: Failed to set socket as reusable.\n", __LINE__);
        return -1;
    }
    */

    len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
    if (bind(server_socket, (struct sockaddr *)&local, len) == -1) {
        log_err("L%d: Socket failed to bind.\n", __LINE__);
        return -1;
    }

    if (listen(server_socket, 5) == -1) {
        log_err("L%d: Failed to listen on socket.\n", __LINE__);
        return -1;
    }

    return server_socket;
}

// Currently this main will setup the socket and accept a single client.
// After handling the client, it will exit.
// You will need to extend this to handle multiple concurrent clients
// and remain running until it receives a shut-down command.
int main(void)
{
    int server_socket = setup_server();
    if (server_socket < 0) {
        exit(1);
    }

    log_info("Waiting for a connection %d ...\n", server_socket);

    struct sockaddr_un remote;
    socklen_t t = sizeof(remote);
    int client_socket = 0;
    int to_free = 1;

    /* initialize the static variable for server status*/
    is_server_running = 1;

    // maintain a catalog manager for server for base columns for startup
    ClientContext* server_context = malloc(sizeof(ClientContext));
    server_context->chandle_slots = MAX_BASE_COL;
    server_context->chandles_in_use = 0;
    server_context->chandle_table = (GeneralizedColumnHandle*)calloc(server_context->chandle_slots, sizeof(GeneralizedColumnHandle));
    
    /* start database */
    db_startup(server_context);

    while(is_server_running){ 
        if ((client_socket = accept(server_socket, (struct sockaddr *)&remote, &t)) == -1) {
            log_err("L%d: Failed to accept a new connection.\n", __LINE__);
            exit(1);
        }
        // create the client context here
        ClientContext* client_context = malloc(sizeof(ClientContext));
        client_context->chandle_slots = MAX_VPOOL;
        client_context->chandles_in_use = 0;
        client_context->chandle_table = (GeneralizedColumnHandle*)calloc(client_context->chandle_slots, sizeof(GeneralizedColumnHandle));
        //copy the base columns into client's catalog manager
        copy_catalog(server_context, client_context);

        handle_client(client_socket, client_context, &to_free);

        copy_catalog(client_context, server_context);

        if (to_free == 1)db_clear_client_context(client_context);
        free(client_context);
        client_context = NULL;
    }
    
    return 0;
}
