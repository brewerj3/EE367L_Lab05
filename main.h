#define BCAST_ADDR 100
#define PAYLOAD_MAX 100
#define STRING_MAX 100
#define NAME_LENGTH 100
#define MAX_FILE_SIZE 1000
#define TENMILLISEC 10000   /* 10 millisecond sleep */
#define CONTROL_COUNT 20

enum yesNo {
    YES, NO
};

enum NetNodeType { /* Types of network nodes */
    HOST, SWITCH, SERVER
};

enum NetLinkType { /* Types of links */
    PIPE, SOCKET
};

struct net_node { /* Network node, e.g., host or switch */
    enum NetNodeType type;
    int id;
    struct net_node *next;
};

struct net_port { /* port to communicate with another node */
    enum NetLinkType type;
    int pipe_host_id;
    int pipe_send_fd;
    int pipe_recv_fd;
    char sendPortNumber[100];
    char recvPortNumber[100];
    char sendDomain[100];
    char recvDomain[100];
    int recvSockfd;
    struct net_port *next;
};

/* Packet sent between nodes  */

struct packet { /* struct for a packet */
    char src;
    char dst;
    char type;
    int length;
    char payload[PAYLOAD_MAX];
    char packetRootID;
    int packetRootDist;
    char packetSenderType;
    char packetSenderChild;
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