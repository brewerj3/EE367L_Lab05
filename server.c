#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "packet.h"
#include "net.h"
#include "host.h"

_Noreturn void server_main(int host_id) {
    if (host_id != 100) {
        printf("Invalid host_id: %i\n", host_id);
        exit(0);
    }

    struct net_port *node_port_list;
    struct net_port **node_port;    // Array of pointers to node pointers
    int node_port_num;              // Number of node ports

    int localRootID = host_id;
    int localRootDist = 0;
    int localParent = -1;
    int controlCount = 0;

    int i, k, n;
    int dst;                        // Packet destination

    struct packet *in_packet;
    struct packet *new_packet;

    struct net_port *p;
    struct host_job *new_job;

    struct job_queue job_q;

    // Create the DNS naming table
    char namingTable[NAMING_TABLE_SIZE][MAX_NAME_LENGTH + 1];
    // Make all entries start with the null character
    for (i = 0; i < NAMING_TABLE_SIZE; i++) {
        namingTable[i][0] = '\0';
    }

    // Create an array node_port to store the network link ports at the host.
    node_port_list = net_get_port_list(100);

    // Count the number of network link ports
    node_port_num = 0;
    for (p = node_port_list; p != NULL; p = p->next) {
        node_port_num++;
    }

    // Create memory space for the array
    node_port = (struct net_port **) malloc(node_port_num * sizeof(struct net_port *));

    /* Load ports into the array */
    p = node_port_list;

    for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
    }

    // Initialize the job queue
    job_q_init(&job_q);

    // Start main loop
    while (1) {

        // Send control packets every 40 milliseconds
        controlCount++;
        if (controlCount > 4) {
            controlCount = 0;

            // Create control packet
            new_packet = (struct packet *) malloc(sizeof(struct packet));
            new_packet->src = (char) host_id;
            new_packet->dst = (char) dst; // @TODO this is not yet set, fix this
            new_packet->type = (char) PKT_CONTROL_PACKET;
            new_packet->length = 4;
            new_packet->payload[0] = (char) localRootID;
            new_packet->payload[1] = (char) localRootDist;
            new_packet->payload[2] = 'D';
            // Set packetSenderChild when sending the packet

            // Create a new job to send the packet, then add to queue
            new_job = (struct host_job *) malloc(sizeof(struct host_job));
            new_job->packet = new_packet;
            new_job->type = JOB_SEND_PKT_ALL_PORTS;
            job_q_add(&job_q, new_job);
        }

        // Get packets from incoming links and translates to jobs
        // Put jobs in job queue
        for (k = 0; k < node_port_num; k++) {
            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);

            if ((n > 0) && ((int) in_packet->dst == host_id)) {
                // Handle packets that are not control packets
                new_job = (struct host_job *) malloc(sizeof(struct host_job));
                new_job->in_port_index = k;
                new_job->packet = in_packet;

                // Switch statement to handle packet types
                switch (in_packet->type) {
                    case (char) PKT_PING_REQ:
                        new_job->type = JOB_PING_SEND_REPLY;
                        job_q_add(&job_q, new_job);
                        break;
                    case (char) PKT_PING_REPLY:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_UPLOAD_START:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_UPLOAD_END:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_UPLOAD_MIDDLE:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_DOWNLOAD_REQ:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_CONTROL_PACKET:     // The DNS Server does not process control packets
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_REGISTER_DOMAIN:
                        // @TODO make this work
                        break;
                    case (char) PKT_REGISTRATION_RECEIPT:
                        // @TODO return to registration requester success or failure
                        break;
                    default:
                        free(in_packet);
                        free(new_job);
                }
            } else {
                free(in_packet);
            }
        }

    }
}