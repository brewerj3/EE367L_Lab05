#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "main.h"
#include "packet.h"
#include "net.h"
#include "host.h"


// This sends the packet to the net_port.
void packet_send(struct net_port *port, struct packet *p) {
    char msg[PAYLOAD_MAX + 4];
    int i;

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
        // @TODO Do socket stuff
        msg[0] = (char) p->src;     // The source of the packet being sent, (the host id)
        msg[1] = (char) p->dst;     // The destination of the packet being sent
        msg[2] = (char) p->type;    // The type of packet being sent
        msg[3] = (char) p->length;  // The length of the packet being sent

        // This adds the payload to the rest of the packet
        for (i = 0; i < p->length; i++) {
            msg[i + 4] = p->payload[i];
        }

        if (msg != NULL) {
            send(port->TCP_port_recv, msg, strlen(msg), 0);
        }
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
    }

    // Return the read error code
    return (n);
}
