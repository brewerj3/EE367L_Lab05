#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "main.h"
#include "packet.h"
#include "net.h"
#include "host.h"

//#define DEBUG

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

#ifdef DEBUG    // This is intended to print out the contents of the packet being sent when DEBUG is active
        printf("Packet being sent")
        printf("PACKET SEND, src=%d dst=%d p-src=%d p-dst=%d\n",
              (int) msg[0],
              (int) msg[1],
              (int) p->src,
              (int) p->dst);
#endif
        // This actually sends the packet
        write(port->pipe_send_fd, msg, p->length + 4);
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

#ifdef DEBUG // This is intended to print out the contents of the packet being received when DEBUG is active
  printf("PACKET RECV, src=%d dst=%d p-src=%d p-dst=%d\n",
              (int) msg[0],
              (int) msg[1],
              (int) p->src,
              (int) p->dst);
#endif
        }
    }

    return (n);
}
