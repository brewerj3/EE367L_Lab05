#ifndef SWITCH_H
#define SWITCH_H

struct net_node **g_net_node;

/* Initialize the switch node */
void switch_init(int id);

/* The main routine for the switch node */
void switch_main(int id);

#endif