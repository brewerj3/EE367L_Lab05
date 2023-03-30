#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "switch.h"

#define MAX_BUF_SIZE 1024

/* Initialize the switch node */
void switch_init(int id){
        printf("Initializing switch %d...\n", id);
}

/* The main routine for the switch node */
void switch_main(int id){
        printf("Starting switch %d...\n", id);

        char buf[MAX_BUF_SIZE];
        int nbytes;

        // Read from the switch's input pipe, and write to all output pipes
        while ((nbytes = read(g_net_node[id].pipe_in, buf, MAX_BUF_SIZE)) > 0) {
                for (int i = 0; i < g_net_node[id].num_links; i++) {
                        write(g_net_node[id].link[i].pipe_node1, buf, nbytes);
                }
        }

        // Close the switch's input and output pipes
        close(g_net_node[id].pipe_in);
        for (int i = 0; i < g_net_node[id].num_links; i++) {
                close(g_net_node[id].link[i].pipe_node1);
        }

        printf("Exiting switch %d...\n", id);
}
