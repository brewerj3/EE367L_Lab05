int net_init();

/// Get a linked list of manager ports on the managers side
struct man_port_at_man *net_get_man_ports_at_man_list();

/// Get the manager port given the host_id
struct man_port_at_host *net_get_host_port(int host_id);

/// Get a list of all nodes
struct net_node *net_get_node_list();

/// Get a list of all the ports connected to the host_id
struct net_port *net_get_port_list(int host_id);
