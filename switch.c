#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "switch.h"
#include "net.h"

#define MAX_BUF_SIZE 1024
#define MAX_CONNECTIONS 4
#define NUM_NODES 1

//Initialize the switch node
void switch_init(int id){
    printf("Initializing switch %d...\n", id);

    // Allocate memory for NUM_NODES network nodes
    struct net_node **g_net_node = (struct net_node **) malloc(sizeof(struct net_node *) * NUM_NODES);

    // Initialize the first network node
    for(int i = 0; i < NUM_NODES; i++) {
        g_net_node[i] = (struct net_node *) malloc(sizeof(struct net_node));
        g_net_node[i]->type = SWITCH;
        g_net_node[i]->id = 0;
        g_net_node[i]->next = NULL;
        g_net_node[i]->pipe_in = -1;
        g_net_node[0]->num_links = 0;
    }
    // Allocate memory for g_net_node[0]'s network links
    g_net_node[0]->link = (struct net_link **) malloc(sizeof(struct net_link *) * g_net_node[0]->num_links);

    // Initialize g_net_node[0]'s network links
    g_net_node[0]->link[0] = ; //It has to equal some network link?
}

void switch_main(int id){
    switch_init(id);
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