#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#include "main.h"
#include "packet.h"
#include "net.h"
#include "host.h"


// This sends the packet to the net_port.
void packet_send(struct net_port *port, struct packet *p) {
    char msg[PAYLOAD_MAX + 4];
    int i;
    static int alreadyConnected = 0;    // 0 is false
    if (port->type == PIPE) {
        // The first 4 parts of the array contain information about the packet
        msg[0] = (char) p->src;     // The source of the packet being sent, (the host id)
        msg[1] = (char) p->dst;     // The destination of the packet being sent
        msg[2] = (char) p->type;    // The type of packet being sent
        msg[3] = (char) p->length;  // The length of the packet being sent

        // This adds the payload to the rest of the packet
        for (i = 0; i < p->length; i++) {
            msg[i + 4] = p->payload[i];
        }

        // This actually sends the packet
        write(port->pipe_send_fd, msg, p->length + 4);
    } else if(port->type == SOCKET) {
        printf("sending packet through a socket\n");
        // @TODO Do socket stuff
        msg[0] = (char) p->src;     // The source of the packet being sent, (the host id)
        msg[1] = (char) p->dst;     // The destination of the packet being sent
        msg[2] = (char) p->type;    // The type of packet being sent
        msg[3] = (char) p->length;  // The length of the packet being sent

        // This adds the payload to the rest of the packet
        for (i = 0; i < p->length; i++) {
            msg[i + 4] = p->payload[i];
        }

        //create sockfd and connect to the port
        int rv, sockfd;
        struct addrinfo hints, *servinfo, *p1;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if((rv = getaddrinfo(port->sendDomain, port->sendPortNumber, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            exit(1);
        }

        for(p1 = servinfo; p1 != NULL; p1 = p1->ai_next) {
            if((sockfd = socket(p1->ai_family, p1->ai_socktype, p1->ai_protocol)) == -1) {
                perror("sending packet: socket");
                continue;
            }
            if (connect(sockfd, p1->ai_addr, p1->ai_addrlen) == -1) {
                perror("sending packet: connect");
                close(sockfd);
                continue;
            }
            printf("socket worked?\n");
            break;
        }

        if (p1 == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            //return;
        }

        // Send the packet
        write(sockfd, msg, p->length + 4);

        // Close the socket when done
        close(sockfd);
    }

}

// This receives a packet
int packet_recv(struct net_port *port, struct packet *p) {
    char msg[PAYLOAD_MAX + 4];
    int n;
    int i;

    if (port->type == PIPE) {
        // n is an error code given by the read function
        n = read(port->pipe_recv_fd, msg, PAYLOAD_MAX + 4);
        if (n > 0) {
            p->src = (char) msg[0];             // The host id of the source
            p->dst = (char) msg[1];             // The host id of the intended destination of the packet
            p->type = (char) msg[2];            // The type of packet
            p->length = (int) msg[3];           // The length of the packet
            for (i = 0; i < p->length; i++) {   // This puts the payload from the message into the packet
                p->payload[i] = msg[i + 4];
            }
        }
    } else if(port->type == SOCKET) {
        // @TODO Do socket stuff
        // Use accept to accept connection
        int new_fd = accept(port->recvSockfd, NULL, 0);
        if(new_fd == -1) {
            // If new_fd equals -1 then there is no connect attempt
            //perror("accept");
            return 0;
        }

        n = read(new_fd, msg, PAYLOAD_MAX + 4, 0);
        close(new_fd);
        if(n > 0) {
            p->src = (char) msg[0];             // The host id of the source
            p->dst = (char) msg[1];             // The host id of the intended destination of the packet
            p->type = (char) msg[2];            // The type of packet
            p->length = (int) msg[3];           // The length of the packet
            for (i = 0; i < p->length; i++) {   // This puts the payload from the message into the packet
                p->payload[i] = msg[i + 4];
            }
        }
    }

    // Return the read error code
    return (n);
}
