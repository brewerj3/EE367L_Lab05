#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "switch2.h"
#include "net.h"
#include "packet.h"

enum switch_job_type {
    JOB_SEND_PKT_ALL_PORTS, JOB_FORWARD_PACKET
};

struct switch_job {
    enum switch_job_type type;
    struct packet *packet;
    int in_port_index;
    int out_port_index;
    struct switch_job *next;
};

/* Add a job to the job queue */
void s_job_q_add(struct switch_job_queue *j_q, struct switch_job *j) {
    if (j_q->head == NULL) {
        j_q->head = j;
        j_q->tail = j;
        j_q->occ = 1;
    } else {
        (j_q->tail)->next = j;
        j->next = NULL;
        j_q->tail = j;
        j_q->occ++;
    }
}

/* Remove job from the job queue, and return pointer to the job*/
struct switch_job *s_job_q_remove(struct switch_job_queue *j_q) {
    struct switch_job *j;

    if (j_q->occ == 0) return (NULL);
    j = j_q->head;
    j_q->head = (j_q->head)->next;
    j_q->occ--;
    return (j);
}

/* Initialize job queue */
void s_job_q_init(struct switch_job_queue *j_q) {
    j_q->occ = 0;
    j_q->head = NULL;
    j_q->tail = NULL;
}

int s_job_q_num(struct switch_job_queue *j_q) {
    return j_q->occ;
}


// Job queue operations
_Noreturn void switch_main(int host_id) {
    // State
    struct net_port *node_port_list;
    struct net_port **node_port;    // Array of pointers to node pointers
    int node_port_num;              // Number of node ports
    int localRootID = host_id;      // Switch initially makes itself the root of the tree
    int localRootDist = 0;          // Switch initially has itself as the root thus has distance 0
    int localParent = -1;           // should be node ID of parent in the tree. initially -1 because switch believes itself to be root

    int i, k, n;
    int dst;                        // Destination of a packet
    int controlCount = 0;           // Counts up to CONTROL_COUNT then resets and sends a control packet

    struct packet *in_packet;       // Incoming packet
    struct packet *new_packet;
    struct packet *new_packet2;

    struct net_port *p;
    struct switch_job *new_job;
    struct switch_job *new_job2;

    struct switch_job_queue job_q;

    // Create an array node_port to store the network link ports at the host.
    node_port_list = net_get_port_list(host_id);

    // Count the number of network link ports
    node_port_num = 0;
    for (p = node_port_list; p != NULL; p = p->next) {
        node_port_num++;
    }

    // Create memory space for the array
    node_port = (struct net_port **) malloc(node_port_num * sizeof(struct net_port *));

    // Create local port tree array
    enum yesNo localPortTree[node_port_num];

    // load ports into the array
    p = node_port_list;
    for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
        localPortTree[k] = NO; // all ports are initially not in the tree
    }

    // Need to initialize the lookup table
    struct Table lookupTable;
    //Fill out the isValid enum
    for (i = 0; i < MAX_LOOKUP_TABLE_SIZE + 2; i++) {
        lookupTable.isValid[i] = False;
    }

    // Initialize the job queue
    s_job_q_init(&job_q);

    while (1) {
        // NO need to get commands from the manager

        // Send control packets every so often
        controlCount++;
        if (controlCount >= CONTROL_COUNT) {
            controlCount = 0;

            // Create a control packet to send
            new_packet = (struct packet *) malloc(sizeof(struct packet));
            new_packet->src = (char) host_id;
            new_packet->type = (char) PKT_CONTROL_PACKET;
            new_packet->length = 0;
            new_packet->packetRootID = (char) localRootID;
            new_packet->packetRootDist = localRootDist;
            new_packet->packetSenderType = 'S';
            new_packet->dst = (char) 0;
            // set packetSenderChild when sending the packet

            // Create a new job to send the control packet
            new_job = (struct switch_job *) malloc(sizeof(struct switch_job));
            new_job->packet = new_packet;
            new_job->type = JOB_SEND_PKT_ALL_PORTS;
            s_job_q_add(&job_q, new_job);
        }

        // Scan all ports
        for (k = 0; k < node_port_num; k++) {

            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);

            if (n > 0) {     // If n > 0 there is a packet to process

                // Process control packets
                if (in_packet->type == (char) PKT_CONTROL_PACKET) {
                    if (in_packet->packetSenderType == 'S') {
                        if(host_id == 6) {
                            //printf("localParent = %i is node %i at node %i. localRootDist = %i localRootID = %i\n", localParent, (int) in_packet->src, host_id, localRootDist, localRootID);
                        }
                        if ((int) in_packet->packetRootID < localRootID) {
                            localRootID = (int) in_packet->packetRootID;
                            localParent = k;
                            localRootDist = in_packet->packetRootDist + 1;
                            printf("node %is localParent is on port %i and is node %i . localRootDist = %i localRootID = %i\n", host_id, localParent, (int) in_packet->src, localRootDist, localRootID);
                        } else if ((int) in_packet->packetRootID == localRootID) {
                            if (localRootDist > in_packet->packetRootDist + 1) {
                                localParent = k;
                                localRootDist = in_packet->packetRootDist + 1;
                                printf("node %is localParent is on port %i and is node %i . localRootDist = %i localRootID = %i\n", host_id, localParent, (int) in_packet->src, localRootDist, localRootID);
                            }
                        }
                    }
                    if (in_packet->packetSenderType == 'H') {
                        localPortTree[k] = YES;
                    } else if (in_packet->packetSenderType == 'S') {
                        if (localParent == k) {
                            localPortTree[k] = YES;
                        } else if (in_packet->packetSenderChild == 'Y') {
                            localPortTree[k] = YES;
                            printf("child\n");
                        } else {
                            localPortTree[k] = NO;
                        }
                    } else {
                        localPortTree[k] = NO;

                    }

                    // check if can add to lookup table
                    if (lookupTable.isValid[(int) in_packet->src] == False) {
                        // Can add to lookup table
                        lookupTable.isValid[(int) in_packet->src] = True;
                        lookupTable.portNumber[(int) in_packet->src] = k;
                    }
                    free(in_packet);
                } else {
                    // Create a new job to handle the packet
                    new_job = (struct switch_job *) malloc(sizeof(struct switch_job));
                    new_job->in_port_index = k;
                    new_job->packet = in_packet;
                    dst = (int) in_packet->dst;
                    int source = (int) in_packet->src;

                    printf("switch %i Handling packet from node %i traveling to node %i\n",host_id, source, dst);
                    // Check lookup table for the port dst;
                    if (lookupTable.isValid[dst] == True) {
                        // The lookup table already contains the port to forward to
                        new_job->type = JOB_FORWARD_PACKET;
                        new_job->out_port_index = lookupTable.portNumber[dst];
                    } else if (lookupTable.isValid[dst] == False) {
                        // The lookup table does not contain the port to forward to
                        new_job->type = JOB_SEND_PKT_ALL_PORTS;
                    }

                    // Check if a port can be added to the lookup table

                    if (lookupTable.isValid[source] == False) {
                        // Add the port to the lookup table
                        lookupTable.isValid[source] = True;
                        lookupTable.portNumber[source] = new_job->in_port_index;
#ifdef DEBUG
                        printf("Changing portNumber\n");
                        printf("source = %i\n", source);
                        printf("lookupTable.portNumber[source] = %i\n",new_job->in_port_index);
#endif
                    }

                    s_job_q_add(&job_q, new_job);
                }
            } else {
                free(in_packet);
            }   // Finished with in_packets

        }

        // Execute one job in the job queue
        if (s_job_q_num(&job_q) > 0) {

            // Get a new job from the job queue
            new_job = s_job_q_remove(&job_q);

            // Send packets
            switch (new_job->type) {
                // Send the packet on all ports
                case JOB_SEND_PKT_ALL_PORTS:
                    for (k = 0; k < node_port_num; k++) {
                        if (new_job->packet->type == (char) PKT_CONTROL_PACKET) {
                            if (localParent == k) {
                                new_job->packet->packetSenderChild = 'Y';
                            } else {
                                new_job->packet->packetSenderChild = 'N';
                            }
                            packet_send(node_port[k], new_job->packet);
                        } else {
                            if (localPortTree[k] == YES || localParent == -1) {
                                printf("switch %i sending on port %i from node %i to node %i\n",host_id, k, (int) new_job->packet->src, (int) new_job->packet->dst);
                                if(k != new_job->in_port_index) {
                                    packet_send(node_port[k], new_job->packet);
                                }
                            }
                        }
                    }
                    free(new_job->packet);
                    free(new_job);
                    break;

                    // Send the packet to one specific port
                case JOB_FORWARD_PACKET:
                    /*printf("forwarding:\n");
                    printf("packet->dst = %i\n", (int) new_job->packet->dst);
                    printf("packet->src = %i\n", (int) new_job->packet->src);
                    printf("new_job->out_port_index = %i\n", new_job->out_port_index);*/
                    //printf("forwarding from %i to %i:\n",host_id, (int) new_job->packet->dst);
                    packet_send(node_port[new_job->out_port_index], new_job->packet);
                    free(new_job->packet);
                    free(new_job);
                    break;
                default:
                    free(new_job->packet);
                    free(new_job);
            }
        }

        // Go to sleep for 10 ms
        usleep(TENMILLISEC);
    }


}