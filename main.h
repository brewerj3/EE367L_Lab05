#define BCAST_ADDR 100
#define PAYLOAD_MAX 100
#define STRING_MAX 100
#define NAME_LENGTH 100
#define MAX_FILE_SIZE 1000

enum NetNodeType { /* Types of network nodes */
    HOST, SWITCH
};

enum NetLinkType { /* Types of linkls */
    PIPE, SOCKET
};

struct net_node { /* Network node, e.g., host or switch */
    enum NetNodeType type;
    int id;
    struct net_node *next;
    int pipe_in;
    int num_links;
    struct net_link **link;
};

struct net_port { /* port to communicate with another node */
    enum NetLinkType type;
    int pipe_host_id;
    int pipe_send_fd;
    int pipe_recv_fd;
    struct net_port *next;
};

struct net_link {
    enum NetLinkType type;
    int pipe_node0;
    int pipe_node1;
    int node1;
    int node2;
    struct net_port *port1;
    struct net_port *port2;
    struct net_link *next;
};

/* Packet sent between nodes  */
struct packet { /* struct for a packet */
    char src;
    char dst;
    char type;
    int length;
    char payload[PAYLOAD_MAX];
};

/* Types of packets */
#define PKT_PING_REQ            0
#define PKT_PING_REPLY          1
#define PKT_FILE_UPLOAD_START   2
#define PKT_FILE_UPLOAD_END     3
// Added
#define PKT_FILE_UPLOAD_MIDDLE  4
#define PKT_FILE_DOWNLOAD_REQ   5

/* Data structure to represent switch nodes */
/* struct switch_node {
    int id;
    int num_links;
    struct net_link *links;
    struct switch_node *next;
}; */

/* Data structure to represent links between switch nodes */
/* struct switch_link {
    int node1;
    int node2;
    int pipe_node1;
    int pipe_node2;
    struct switch_link *next;
}; */