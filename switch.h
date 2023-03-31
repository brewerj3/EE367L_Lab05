#ifndef SWITCH_H
#define SWITCH_H

/* Initialize the switch node */
struct net_node **switch_init(int id);

/* The main routine for the switch node */
void switch_main(int id);

#endif