/* Definitions and prototypes for the link (link.c)
 */


/// Receive packet on port
int packet_recv(struct net_port *port, struct packet *p);

/// Send packet on port
void packet_send(struct net_port *port, struct packet *p);
