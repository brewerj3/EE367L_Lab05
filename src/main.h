#define BCAST_ADDR 100
#define PAYLOAD_MAX 100
#define STRING_MAX 100
#define NAME_LENGTH 100
#define MAX_FILE_SIZE 1000
#define TENMILLISEC 10000   /* 10 millisecond sleep */
#define CONTROL_COUNT 5

enum yesNo {
    YES, NO
};

/// The types of network nodes
enum NetNodeType { /* Types of network nodes */
    HOST, SWITCH, SERVER
};

/// The types of network links
enum NetLinkType { /* Types of links */
    PIPE, SOCKET
};

/// Used to make a linked list of all nodes in the network, and store their type and Id
struct net_node { /* Network node, e.g., host or switch */
    enum NetNodeType type;
    int id;
    struct net_node *next;
};

/// All the information a port needs to communicate
struct net_port { /* port to communicate with another node */
    enum NetLinkType type;  ///< The type of port, PIPE, or SOCKET
    int pipe_host_id;       ///< The host_id the port is connected to
    int pipe_send_fd;       ///< The file descriptor used to send information
    int pipe_recv_fd;       ///< The file descriptor used to recieve information
    char sendPortNumber[100];
    char recvPortNumber[100];
    char sendDomain[100];
    char recvDomain[100];
    int recvSockfd;
    struct net_port *next;  ///< Points to the next port
};

/* Packet sent between nodes  */
/// Defines a packet that is sent between nodes
struct packet { /* struct for a packet */
    char src;                   ///< The id of the source node
    char dst;                   ///< The id of the destination node
    char type;                  ///< The type of packet
    int length;                 ///< The length of the packet
    char payload[PAYLOAD_MAX];  ///< The payload of the packet, maxed at 100 bytes
    char packetRootID;          ///< The believed root of the switch tree
    int packetRootDist;         ///< The distance to the believed root of the switch tree
    char packetSenderType;      ///< The type of node sending the packet
    char packetSenderChild;     ///< Stores if the sender of the packet is a child of the recipient or not
};

/* Types of packets */

#define PKT_PING_REQ            0
#define PKT_PING_REPLY          1
#define PKT_FILE_UPLOAD_START   2
#define PKT_FILE_UPLOAD_END     3
// Added
#define PKT_FILE_UPLOAD_MIDDLE  4
#define PKT_FILE_DOWNLOAD_REQ   5

#define PKT_CONTROL_PACKET      6

#define PKT_DNS_REGISTER        7
#define PKT_DNS_REGISTER_REPLY  8

#define PKT_DNS_LOOKUP          9
#define PKT_DNS_LOOKUP_REPLY    10