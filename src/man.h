/*
 * man.h
 */

#define MAN_MSG_LENGTH 1000


/*
 *  The next two structs are ports used to transfer commands
 *  and replies between the manager and hosts
 */

/// Linked list of ports at the host side
struct man_port_at_host {  /* Port located at the host */
    int host_id;                        ///< The id of the host
    int send_fd;                        ///< The file descriptor sending to the host
    int recv_fd;                        ///< The file descriptor receiving from the host
    struct man_port_at_host *next;      ///< Next port in the linked list
};

/// Linked list of ports at the manager side
struct man_port_at_man {  /* Port located at the man */
    int host_id;                    ///< The id of the host
    int send_fd;                    ///< The file descriptor sending to the manager
    int recv_fd;                    ///< The file descriptor receiving from the manager
    struct man_port_at_man *next;   ///< Next port in the linked list
};

/*
 * Main loop for the manager.
 */
/// The main function of the manager, loops until user quits, then kills all other processes
void man_main();
