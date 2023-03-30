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

//Initialize the switch node
void switch_init(int id){
        printf("Initializing switch %d...\n", id);
}

void switch_main(int id){
        printf("Starting switch %d...\n", id);

        char buf[MAX_BUF_SIZE];
        int nbytes;

        //Read from the switch's input pipe, and write to all output pipes
        while ((nbytes = read(g_net_node[id].pipe_in, buf, MAX_BUF_SIZE)) > 0) {
                for (int i = 0; i < g_net_node[id].num_links; i++) {
                        int pipe_out;
                        if (g_net_node[id].link[i].node1 == id) {
                                //Output port is node2's input port
                                pipe_out = g_net_node[g_net_node[id].link[i].node2].pipe_in;
                        }
                        else if (g_net_node[id].link[i].node2 == id) {
                                //Output port is node1's input port
                                pipe_out = g_net_node[g_net_node[id].link[i].node1].pipe_in;
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
        close(g_net_node[id].pipe_in);
        for (int i = 0; i < g_net_node[id].num_links; i++) {
                int pipe_out;
                if (g_net_node[id].link[i].node1 == id) {
                        pipe_out = g_net_node[g_net_node[id].link[i].node2].pipe_in;
                }
                else if (g_net_node[id].link[i].node2 == id) {
                        pipe_out = g_net_node[g_net_node[id].link[i].node1].pipe_in;
                }
                else {
                        continue;
                }
                close(pipe_out);
        }

        printf("Exiting switch %d...\n", id);
}
