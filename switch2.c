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
#include "host.h"
#include "packet.h"

void addEntryLookupTable(struct packet *in_packet) {

}

// Job queue operations
_Noreturn void switch_main(int host_id) {
    // State
    struct net_port *node_port_list;
    struct net_port **node_port;    // Array of pointers to node pointers
    int node_port_num;              // Number of node ports

    int i, k, n;
    int dst;

    struct packet *in_packet;       // Incoming packet
    struct packet *new_packet;
    struct packet *new_packet2;

    struct net_port *p;
    struct host_job *new_job;
    struct host_job *new_job2;

    struct job_queue job_q;

    // Create an array node_port to store the network link ports at the host.
    node_port_list = net_get_port_list(host_id);

    // Count the number of network link ports
    node_port_num = 0;
    for(p = node_port_list; p != NULL; p = p->next) {
        node_port_num++;
    }

    // Create memory space for the array
    node_port = (struct net_port **) malloc(node_port_num * sizeof(struct net_port *));

    // load ports into the array
    p = node_port_list;
    for(k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
    }

    // Need to initialize the lookup table
    struct Table *lookupTable = (struct Table *) malloc(sizeof(struct Table));
    //Fill out the isValid enum
    for(i = 0; i < MAX_LOOKUP_TABLE_SIZE; i++) {
        lookupTable->isValid[i] = False;
    }

    // Initialize the job queue
    job_q_init(&job_q);

    while(1) {
        // No need to get commands from the manager

        // Scan all ports
        for( k = 0; k < node_port_num; k++) {

            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);

            if(n > 0) {     // If n > 0 there is a packet to process

                // Create a new job to handle the packet
                new_job = (struct host_job *) malloc(sizeof(struct host_job));
                new_job->in_port_index = k;
                new_job->packet = in_packet;
                dst = (int) in_packet->dst;
                // Check lookup table for the port dst;
                if(lookupTable->isValid[dst] == True) {
                    // The lookup table already contains the port to forward to
                    new_job->type = JOB_FORWARD_PACKET;
                    new_job->out_port_index = lookupTable->portNumber[dst];
                } else if(lookupTable->isValid[dst] == False) {
                    // The lookup table does not contain the port to forward to
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                }

                // Check if a port can be added to the lookup table
                if(lookupTable->isValid[ (int) in_packet->src] == False) {
                    int location = (int) in_packet->src;
                    // Add the port to the lookup table
                    lookupTable->isValid[location] = True;
                    lookupTable->destination[location] = location;
                    lookupTable->portNumber[location] = new_job->in_port_index;
                }
                job_q_add(&job_q, new_job);

            } else {
                free(in_packet);
            }   // Finished with in_packets

        }

        // Execute one job in the job queue
        if(job_q_num(&job_q) > 0) {

            // Get a new job from the job queue
            new_job = job_q_remove(&job_q);

            // Send packets
            switch(new_job->type) {
                // Send the packet on all ports
                case JOB_SEND_PKT_ALL_PORTS:
                    for (k = 0; k < node_port_num; k++ ) {
                        if(k == (int) new_job->packet->src) {
                            continue;
                        }
                        packet_send(node_port[k], new_job->packet);
                    }
                    free(new_job->packet);
                    free(new_job);
                    break;

                    // Send the packet to one specific port
                case JOB_FORWARD_PACKET:
                    packet_send(node_port[new_job->out_port_index], new_job->packet);
                    free(new_job->packet);
                    free(new_job);
                    break;
            }
        }

        // Go to sleep for 10 ms
        usleep(TENMILLISEC);
    }


}