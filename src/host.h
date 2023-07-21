/*
 * host.h
 */

// Added JOB_FILE_UPLOAD_MIDDLE
/// Jobs performed by a host node
enum host_job_type {
    JOB_SEND_PKT_ALL_PORTS,         ///< Send the packet on all non manager ports
    JOB_PING_SEND_REPLY,            ///< Send a reply to a ping request
    JOB_PING_WAIT_FOR_REPLY,        ///< Wait for a ping reply
    JOB_FILE_UPLOAD_SEND,           ///< Upload a file to a host
    JOB_FILE_UPLOAD_RECV_START,     ///< Receive the start of a upload from a host
    JOB_FILE_UPLOAD_RECV_END,       ///< Receive the end of a upload from a host
    JOB_FILE_UPLOAD_RECV_MIDDLE,    ///< Receive the middle of a upload from a host
    JOB_DNS_REGISTER_WAIT_FOR_REPLY,    ///< Wait for a reply from the DNS server
    JOB_DNS_LOOKUP_WAIT_FOR_REPLY,      ///< Wait for a lookup request response from the DNS server
    JOB_DNS_PING_WAIT_FOR_REPLY,        ///< Wait for a DNS ping response
    JOB_DNS_DOWNLOAD_WAIT_FOR_REPLY,    ///< Wait for DNS lookup response, if name exists, try download the file from the host pointed to by the server
};

/// Contains information needed to store a job in the job queue
struct host_job {
    enum host_job_type type;    ///< The type of host_job
    struct packet *packet;      ///< An attached packet for the job
    int in_port_index;          ///< The port the packet came in from
    int out_port_index;         ///< The port to send the packet on
    char fname_download[100];   ///< Name of the file to download
    char fname_upload[100];     ///< Name of the file to upload
    int ping_timer;             ///< Timer counts down to wait for ping response
    int file_upload_dst;        ///< The destination of the file to upload
    struct host_job *next;      ///< The next job in the job queue
};

/// The structure of the job queue
struct host_job_queue {
    struct host_job *head;
    struct host_job *tail;
    int occ;
};

_Noreturn void host_main(int host_id);
