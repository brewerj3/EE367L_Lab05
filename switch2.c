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
        for( k = 0; k < node_port_num; k++) {   // Scan all ports

            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);

            if(n > 0) {     // If n > 0 there is a packet to process

                // Create a new job to handle the packet
                new_job = (struct host_job *) malloc(sizeof(struct host_job));
                new_job->in_port_index = k;
                new_job->packet = in_packet;

                // Check lookup table for the port dst;
                if(lookupTable->isValid[ (int) in_packet->dst] != False) {
                    // The lookup table already contains the port to forward to
                    new_job->type = JOB_FORWARD_PACKET;
                    if(lookupTable->isValid[ (int) in_packet->src]) {
                        int location = (int) in_packet->src;
                        // Add the port to the lookup table
                        lookupTable->isValid[location] = True;
                        lookupTable->destination[location] = location;
                        lookupTable->portNumber[location] = k;
                    }
                    job_q_add(&job_q, new_job);
                } else if(lookupTable->isValid[ (int) in_packet->dst] == False) {
                    // The lookup table does not contain the port to forward to
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;

                    // Check if a port can be added to the lookup table
                    if(lookupTable->isValid[ (int) in_packet->src] == False) {
                        int location = (int) in_packet->src;
                        // Add the port to the lookup table
                        lookupTable->isValid[location] = True;
                        lookupTable->destination[location] = location;
                        lookupTable->portNumber[location] = k;
                    }
                }

            } else {
                free(in_packet);
            }   // Finished with in_packets

            // Execute one job in the job queue

            if(job_q_num(&job_q) > 0) {

                // Get a new job from the job queue
                new_job = job_q_remove(&job_q);

                // Send packets
                switch(new_job->type) {
                    case JOB_SEND_PKT_ALL_PORTS:

                        break;
                    case JOB_FORWARD_PACKET:
                }
            }
        }
    }


}

//Initialize the switch node


/*void switch_init(int id){
        printf("Initializing switch %d...\n", id);
}

void switch_main(int id){
        printf("Starting switch %d...\n", id);

        char buf[MAX_BUF_SIZE];
        int nbytes;

        //Read from the switch's input pipe, and write to all output pipes
        while ((nbytes = read(g_net_node[id] -> pipe_in, buf, MAX_BUF_SIZE)) > 0) {
                for (int i = 0; i < g_net_node[id] -> num_links; i++) {
                        int pipe_out;
                        if (g_net_node[id] -> link[i] -> node1 == id) {
                                //Output port is node2's input port
                                pipe_out = g_net_node[g_net_node[id] -> link[i] -> node2] -> pipe_in;
                        }
                        else if (g_net_node[id] -> link[i] -> node2 == id) {
                                //Output port is node1's input port
                                pipe_out = g_net_node[g_net_node[id] -> link[i] -> node1] -> pipe_in;
                        }
                        else {
                                //Link is not connected to this switch, skip
                                continue;
                        }
                        //Write the data to the output port
                        write(pipe_out, buf, nbytes);
                }
        }

        //Close the switch's input pipe and all output pipes
        close(g_net_node[id] -> pipe_in);
        for (int i = 0; i < g_net_node[id] -> num_links; i++) {
                int pipe_out;
                if (g_net_node[id] -> link[i] -> node1 == id) {
                        pipe_out = g_net_node[g_net_node[id] -> link[i] -> node2] -> pipe_in;
                }
                else if (g_net_node[id] -> link[i] -> node2 == id) {
                        pipe_out = g_net_node[g_net_node[id] -> link[i] -> node1] -> pipe_in;
                }
                else {
                        continue;
                }
                close(pipe_out);
        }

        printf("Exiting switch %d...\n", id);
}
*/