/*test
 * host.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "net.h"
#include "man.h"
#include "host.h"
#include "packet.h"

#define MAX_FILE_BUFFER 1000
#define MAX_MSG_LENGTH 100
#define MAX_DIR_NAME 100
#define MAX_FILE_NAME 100
#define PKT_PAYLOAD_MAX 100
#define MAX_DOMAIN_NAME 50

/* Types of packets */

struct file_buf {
    char name[MAX_FILE_NAME];           ///< Holds the name of a file
    int name_length;                    ///< Holds the length of the file name
    char buffer[MAX_FILE_BUFFER + 1];   ///< Holds the first 1000 bytes of the file
    int head;
    int tail;
    int occ;
    FILE *fd;
};


/*
 * File buffer operations
 */

/* Initialize file buffer data structure */
void file_buf_init(struct file_buf *f) {
    f->head = 0;
    f->tail = MAX_FILE_BUFFER;
    f->occ = 0;
    f->name_length = 0;
}

/*
 * Get the file name in the file buffer and store it in name
 * Terminate the string in name with tne null character
 */
void file_buf_get_name(struct file_buf *f, char name[]) {
    int i;

    for (i = 0; i < f->name_length; i++) {
        name[i] = f->name[i];
    }
    name[f->name_length] = '\0';
}

/*
 *  Put name[] into the file name in the file buffer
 *  length = the length of name[]
 */
void file_buf_put_name(struct file_buf *f, char name[], int length) {
    int i;

    for (i = 0; i < length; i++) {
        f->name[i] = name[i];
    }
    f->name_length = length;
}

/*
 *  Add 'length' bytes n string[] to the file buffer
 */
int file_buf_add(struct file_buf *f, char string[], int length) {
    int i = 0;

    while (i < length && f->occ < MAX_FILE_BUFFER) {
        f->tail = (f->tail + 1) % (MAX_FILE_BUFFER + 1);
        f->buffer[f->tail] = string[i];
        i++;
        f->occ++;
    }
    return (i);
}

/*
 *  Remove bytes from the file buffer and store it in string[]
 *  The number of bytes is length.
 */
int file_buf_remove(struct file_buf *f, char string[], int length) {
    int i = 0;

    while (i < length && f->occ > 0) {
        string[i] = f->buffer[f->head];
        f->head = (f->head + 1) % (MAX_FILE_BUFFER + 1);
        i++;
        f->occ--;
    }

    return (i);
}


/*
 * Operations with the manager
 */

int get_man_command(struct man_port_at_host *port, char msg[], char *c) {

    int n;
    int i;
    int k;

    n = read(port->recv_fd, msg, MAN_MSG_LENGTH); /* Get command from manager */
    if (n > 0) {  /* Remove the first char from "msg" */
        for (i = 0; msg[i] == ' ' && i < n; i++);
        *c = msg[i];
        i++;
        for (; msg[i] == ' ' && i < n; i++);
        for (k = 0; k + i < n; k++) {
            msg[k] = msg[k + i];
        }
        msg[k] = '\0';
    }
    return n;

}

/*
 * Operations requested by the manager
 */

/* Send back state of the host to the manager as a text message */
void reply_display_host_state(struct man_port_at_host *port, char dir[], int dir_valid, int host_id) {
    int n;
    char reply_msg[MAX_MSG_LENGTH];

    if (dir_valid == 1) {
        n = sprintf(reply_msg, "%s %d", dir, host_id);
    } else {
        n = sprintf(reply_msg, "None %d", host_id);
    }

    write(port->send_fd, reply_msg, n);
}



/* Job queue operations */

/* Add a job to the job queue */
void h_job_q_add(struct host_job_queue *j_q, struct host_job *j) {
    if (j_q->head == NULL) {
        j_q->head = j;
        j_q->tail = j;
        j_q->occ = 1;
    } else {
        (j_q->tail)->next = j;
        j->next = NULL;
        j_q->tail = j;
        j_q->occ++;
    }
}

/* Remove job from the job queue, and return pointer to the job*/
struct host_job *h_job_q_remove(struct host_job_queue *j_q) {
    struct host_job *j;

    if (j_q->occ == 0) return (NULL);
    j = j_q->head;
    j_q->head = (j_q->head)->next;
    j_q->occ--;
    return (j);
}

/* Initialize job queue */
void h_job_q_init(struct host_job_queue *j_q) {
    j_q->occ = 0;
    j_q->head = NULL;
    j_q->tail = NULL;
}

int h_job_q_num(struct host_job_queue *j_q) {
    return j_q->occ;
}

/*
 *  Main
 */

_Noreturn void host_main(int host_id) {

/* State */
    char dir[MAX_DIR_NAME];
    int dir_valid = 0;

    char man_msg[MAN_MSG_LENGTH];
    char man_reply_msg[MAN_MSG_LENGTH];
    char man_cmd;
    struct man_port_at_host *man_port;  // Port to the manager

    struct net_port *node_port_list;
    struct net_port **node_port;  // Array of pointers to node ports
    int node_port_num;            // Number of node ports

    int ping_reply_received;
    int dns_register_received;
    int dns_lookup_received;
    int dnsLookupResponse;

    int localRootID = host_id;
    int localRootDist = 0;
    int controlCount = 0;

    int i, k, n, j;
    int dst;
    char name[MAX_FILE_NAME];
    char domainName[MAX_DOMAIN_NAME];
    char string[PKT_PAYLOAD_MAX + 1];
    char buffer[MAX_FILE_BUFFER + 1];
    char dnsRegisterBuffer[MAX_DOMAIN_NAME + 1];
    memset(dnsRegisterBuffer, 0, MAX_DOMAIN_NAME + 1);
    dnsRegisterBuffer[0] = '\0';
    char dnsLookupBuffer[MAX_DOMAIN_NAME + 1];
    memset(dnsLookupBuffer, 0, MAX_DOMAIN_NAME + 1);
    dnsLookupBuffer[0] = '\0';

    FILE *fp;

    struct packet *in_packet; /* Incoming packet */
    struct packet *new_packet;
    struct packet *new_packet2;

    struct net_port *p;
    struct host_job *new_job;
    struct host_job *new_job2;

    struct host_job_queue job_q;

    struct file_buf f_buf_upload;
    struct file_buf f_buf_download;

    file_buf_init(&f_buf_upload);
    file_buf_init(&f_buf_download);

/*
 * Initialize pipes
 * Get link port to the manager
 */

    man_port = net_get_host_port(host_id);

/*
 * Create an array node_port[ ] to store the network link ports
 * at the host.  The number of ports is node_port_num
 */
    node_port_list = net_get_port_list(host_id);
    //printf("host_id = %i\n",host_id);

    /*  Count the number of network link ports */
    node_port_num = 0;
    for (p = node_port_list; p != NULL; p = p->next) {
        node_port_num++;
    }

    /* Create memory space for the array */
    node_port = (struct net_port **) malloc(node_port_num * sizeof(struct net_port *));

    // Create local port tree array
    //enum yesNo localPortTree[node_port_num];

    /* Load ports into the array */
    p = node_port_list;

    for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
    }

/* Initialize the job queue */
    h_job_q_init(&job_q);

    while (1) {

        // Send control packets every so often
        controlCount++;
        if (controlCount >= CONTROL_COUNT) {
            controlCount = 0;

            // Create a control packet to send
            new_packet = (struct packet *) malloc(sizeof(struct packet));
            new_packet->src = (char) host_id;
            new_packet->type = (char) PKT_CONTROL_PACKET;
            new_packet->length = 0;
            new_packet->packetSenderType = 'H';
            new_packet->packetSenderChild = 'Y';
            new_packet->dst = (char) 0;

            // Create a new job to send the control packet
            new_job = (struct host_job *) malloc(sizeof(struct host_job));
            new_job->packet = new_packet;
            new_job->type = JOB_SEND_PKT_ALL_PORTS;
            h_job_q_add(&job_q, new_job);
        }

        /* Execute command from manager, if any */

        /* Get command from manager */
        n = get_man_command(man_port, man_msg, &man_cmd);

        /* Execute command */
        if (n > 0) {
            switch (man_cmd) {
                case 's':
                    reply_display_host_state(man_port, dir, dir_valid, host_id);
                    break;

                case 'm':
                    dir_valid = 1;
                    for (i = 0; man_msg[i] != '\0'; i++) {
                        dir[i] = man_msg[i];
                    }
                    dir[i] = man_msg[i];
                    break;

                case 'p': // Sending ping request
                    // Create new ping request packet
                    sscanf(man_msg, "%d", &dst);
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->src = (char) host_id;
                    new_packet->dst = (char) dst;
                    new_packet->type = (char) PKT_PING_REQ;
                    new_packet->length = 0;
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->packet = new_packet;
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                    h_job_q_add(&job_q, new_job);

                    new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                    ping_reply_received = 0;
                    new_job2->type = JOB_PING_WAIT_FOR_REPLY;
                    new_job2->ping_timer = 10;
                    h_job_q_add(&job_q, new_job2);
                    break;

                case 'u': /* Upload a file to a host */
                    sscanf(man_msg, "%d %s", &dst, name);
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->type = JOB_FILE_UPLOAD_SEND;
                    new_job->file_upload_dst = dst;
                    for (i = 0; name[i] != '\0'; i++) {
                        new_job->fname_upload[i] = name[i];
                    }
                    new_job->fname_upload[i] = '\0';
                    h_job_q_add(&job_q, new_job);

                    break;
                case 'd': // Request a file to be downloaded
                    sscanf(man_msg, "%d %s", &dst, name);

                    // Create a new packet
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->src = (char) host_id;
                    new_packet->dst = (char) dst;
                    new_packet->type = (char) PKT_FILE_DOWNLOAD_REQ;
                    for (i = 0; name[i] != '\0'; i++) {
                        new_packet->payload[i] = name[i];
                    }
                    new_packet->payload[i] = '\0';
                    new_packet->length = i;

                    // Create a new job to send the packet
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->packet = new_packet;
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                    h_job_q_add(&job_q, new_job);
                    break;
                case 'r':   // Register a domain name with the DNS server
                    sscanf(man_msg, "%s", domainName);

                    // Create a new packet
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->src = (char) host_id;
                    new_packet->dst = (char) 100;  // DNS server always has id 100
                    new_packet->type = (char) PKT_DNS_REGISTER;
                    for (i = 0; domainName[i] != '\0'; i++) {
                        new_packet->payload[i] = domainName[i];
                    }
                    new_packet->payload[i] = '\0';
                    new_packet->length = i;

                    // Create a job to send the packet
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->packet = new_packet;
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                    h_job_q_add(&job_q, new_job);

                    // Create a second job to wait for reply
                    new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                    dns_register_received = 0;
                    memset(dnsRegisterBuffer, 0, MAX_DOMAIN_NAME);
                    new_job2->type = JOB_DNS_REGISTER_WAIT_FOR_REPLY;
                    new_job2->ping_timer = 40;
                    h_job_q_add(&job_q, new_job2);
                    break;
                case 'l':   // lookup a host id by their hostname
                    sscanf(man_msg, "%s", domainName);

                    // Create a new packet
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->src = (char) host_id;
                    new_packet->dst = 100;  // DNS server always has id 100
                    new_packet->type = (char) PKT_DNS_LOOKUP;
                    for (i = 0; domainName[i] != '\0'; i++) {
                        new_packet->payload[i] = domainName[i];
                    }
                    new_packet->payload[i] = '\0';
                    new_packet->length = i;

                    // Create a job to send the packet
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->packet = new_packet;
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                    h_job_q_add(&job_q, new_job);

                    // Create a second job to wait for reply
                    new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                    dns_lookup_received = 0;
                    memset(dnsLookupBuffer, 0, MAX_DOMAIN_NAME);
                    strcpy(new_job2->fname_download, domainName);
                    new_job2->type = JOB_DNS_LOOKUP_WAIT_FOR_REPLY;
                    new_job2->ping_timer = 40;
                    h_job_q_add(&job_q, new_job2);
                    break;
                case 'P':   // Ping a host by their domain name
                    sscanf(man_msg, "%s", domainName);
                    // Create a new packet
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->src = (char) host_id;
                    new_packet->dst = 100;  // DNS server always has id 100
                    new_packet->type = (char) PKT_DNS_LOOKUP;
                    for (i = 0; domainName[i] != '\0'; i++) {
                        new_packet->payload[i] = domainName[i];
                    }
                    new_packet->payload[i] = '\0';
                    new_packet->length = i;

                    // Create a job to send the packet
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->packet = new_packet;
                    new_job->type = JOB_SEND_PKT_ALL_PORTS;
                    h_job_q_add(&job_q, new_job);

                    // Create a second job to wait for reply
                    new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                    dns_lookup_received = 0;
                    new_job2->type = JOB_DNS_PING_WAIT_FOR_REPLY;
                    new_job2->ping_timer = 40;
                    h_job_q_add(&job_q, new_job2);
                    break;
                case 'D':   // Download from a host by giving their domain name
                    sscanf(man_msg, "%s %s", domainName, name);
                    //printf("s1 = %s\n",domainName);
                    //printf("s2 = %s\n",name); @TODO execution passes here

                    // Create a packet to lookup their domain name
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->src = (char) host_id;
                    new_packet->dst = 100;  // DNS server always has id 100
                    new_packet->type = (char) PKT_DNS_LOOKUP;
                    for (i = 0; domainName[i] != '\0'; i++) {
                        new_packet->payload[i] = domainName[i];
                    }
                    new_packet->payload[i] = '\0';
                    new_packet->length = i;

                    // Create a job to send the packet
                    new_job = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job->packet = new_packet;
                    new_job->type = (char) JOB_SEND_PKT_ALL_PORTS;
                    h_job_q_add(&job_q, new_job);

                    // Create a second job to wait for reply
                    new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                    dns_lookup_received = 0;
                    strcpy(new_job2->fname_download, name);
                    new_job2->type = JOB_DNS_DOWNLOAD_WAIT_FOR_REPLY;
                    new_job2->ping_timer = 40;
                    h_job_q_add(&job_q, new_job2);
                    break;
                default:;
            }
        }

        /*
         * Get packets from incoming links and translate to jobs
         * Put jobs in job queue
         */

        for (k = 0; k < node_port_num; k++) { /* Scan all ports */

            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);                   // This reads incoming packets

            if ((n > 0) && (in_packet->type == (char) PKT_CONTROL_PACKET)) {
                free(in_packet);
            } else if ((n > 0) && ((int) in_packet->dst == host_id)) {
                new_job = (struct host_job *) malloc(sizeof(struct host_job));
                new_job->in_port_index = k;
                new_job->packet = in_packet;
                switch (in_packet->type) {
                    /* Consider the packet type */

                    /*
                     * The next two packet types are
                     * the ping request and ping reply
                     */
                    case (char) PKT_PING_REQ:
                        new_job->type = JOB_PING_SEND_REPLY;
                        h_job_q_add(&job_q, new_job);
                        break;

                    case (char) PKT_PING_REPLY:
                        ping_reply_received = 1;
                        free(in_packet);
                        free(new_job);
                        break;

                        /*
                         * The next two packet types
                         * are for the upload file operation.
                         *
                         * The first type is the start packet
                         * which includes the file name in
                         * the payload.
                         *
                         * The second type is the end packet
                         * which carries the content of the file
                         * in its payload
                         */

                    case (char) PKT_FILE_UPLOAD_START:
                        new_job->type = JOB_FILE_UPLOAD_RECV_START;
                        h_job_q_add(&job_q, new_job);
                        break;

                    case (char) PKT_FILE_UPLOAD_END:
                        new_job->type = JOB_FILE_UPLOAD_RECV_END;
                        h_job_q_add(&job_q, new_job);
                        break;

                        // Added by Joshua Brewer
                    case (char) PKT_FILE_UPLOAD_MIDDLE:         // Packet containing the middle contents of a file
                        new_job->type = JOB_FILE_UPLOAD_RECV_MIDDLE;
                        h_job_q_add(&job_q, new_job);
                        break;

                    case (char) PKT_FILE_DOWNLOAD_REQ:           // Start a upload
                        new_job->type = JOB_FILE_UPLOAD_SEND;

                        for (i = 0; in_packet->payload[i] != '\0'; i++) {
                            new_job->fname_upload[i] = in_packet->payload[i];
                        }
                        new_job->fname_upload[i] = '\0';
                        new_job->file_upload_dst = (int) in_packet->src;
                        h_job_q_add(&job_q, new_job);
                        break;
                    case (char) PKT_DNS_REGISTER_REPLY:
                        dns_register_received = 1;
                        j = sprintf(dnsRegisterBuffer, "%s", in_packet->payload);
                        dnsRegisterBuffer[n] = '\0';
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_DNS_LOOKUP_REPLY:
                        dns_lookup_received = 1;
                        j = sprintf(dnsLookupBuffer, "%s", in_packet->payload);
                        dnsLookupBuffer[j] = '\0';
                        free(in_packet);
                        free(new_job);
                        break;
                    default:
                        free(in_packet);
                        free(new_job);
                }
            } else {
                free(in_packet);
            }
        }

        /*
         * Execute one job in the job queue
         */

        if (h_job_q_num(&job_q) > 0) {

            /* Get a new job from the job queue */
            new_job = h_job_q_remove(&job_q);


            /* Send packet on all ports */
            switch (new_job->type) {

                /* Send packets on all ports */
                case JOB_SEND_PKT_ALL_PORTS:
                    for (k = 0; k < node_port_num; k++) {
                        packet_send(node_port[k], new_job->packet);
                    }
                    free(new_job->packet);
                    free(new_job);
                    break;

                    /* The next three jobs deal with the pinging process */
                case JOB_PING_SEND_REPLY:
                    /* Send a ping reply packet */

                    /* Create ping reply packet */
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->dst = new_job->packet->src;
                    new_packet->src = (char) host_id;
                    new_packet->type = PKT_PING_REPLY;
                    new_packet->length = 0;

                    /* Create job for the ping reply */
                    new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                    new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                    new_job2->packet = new_packet;

                    /* Enter job in the job queue */
                    h_job_q_add(&job_q, new_job2);

                    /* Free old packet and job memory space */
                    free(new_job->packet);
                    free(new_job);
                    break;

                case JOB_PING_WAIT_FOR_REPLY:
                    /* Wait for a ping reply packet */

                    if (ping_reply_received == 1) {
                        n = sprintf(man_reply_msg, "Ping acked!");
                        man_reply_msg[n] = '\0';
                        write(man_port->send_fd, man_reply_msg, n + 1);
                        free(new_job);
                    } else if (new_job->ping_timer > 1) {
                        new_job->ping_timer--;
                        h_job_q_add(&job_q, new_job);
                    } else { /* Time out */
                        n = sprintf(man_reply_msg, "Ping time out!");
                        man_reply_msg[n] = '\0';
                        write(man_port->send_fd, man_reply_msg, n + 1);
                        free(new_job);
                    }
                    break;

                    /* The next three jobs deal with uploading a file */

                    /* This job is for the sending host */
                case JOB_FILE_UPLOAD_SEND:
                    /* Open file */
                    if (dir_valid == 1) {
                        n = sprintf(name, "./%s/%s", dir, new_job->fname_upload);
                        name[n] = '\0';
                        fp = fopen(name, "r");
                        if (fp != NULL) {
                            //printf("Successfully opened file named: %s\n", name);
                            /*
                             * Create first packet which
                             * has the file name
                             */
                            new_packet = (struct packet *) malloc(sizeof(struct packet));
                            new_packet->dst = (char) new_job->file_upload_dst;
                            new_packet->src = (char) host_id;
                            new_packet->type = PKT_FILE_UPLOAD_START;
                            for (i = 0; new_job->fname_upload[i] != '\0'; i++) {
                                new_packet->payload[i] = new_job->fname_upload[i];
                            }
                            new_packet->length = i;

                            /*
                             * Create a job to send the packet
                             * and put it in the job queue
                             */
                            new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                            new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                            new_job2->packet = new_packet;
                            h_job_q_add(&job_q, new_job2);

                            // fread returns the number of characters in the file
                            n = fread(buffer, sizeof(char), MAX_FILE_BUFFER, fp);
                            fclose(fp);
                            buffer[n] = '\0'; // NULL terminate buffer

                            // If n is larger than the max message length it must be split up into multiple packets
                            while (n > MAX_MSG_LENGTH) {
                                // Split up packets

                                // Put 100 characters of buffer into string
                                strncpy(string, buffer, MAX_MSG_LENGTH);

                                // shift buffer 100 places
                                for (int j = 0; buffer[j] != '\0' || j < MAX_MSG_LENGTH; j++) {
                                    buffer[j] = buffer[j + MAX_MSG_LENGTH];
                                }
                                n = n - MAX_MSG_LENGTH;

                                // Now send the middle packets using string as the payload

                                // First define the packet structure
                                new_packet = (struct packet *) malloc(sizeof(struct packet));
                                new_packet->dst = new_job->file_upload_dst;
                                new_packet->src = (char) host_id;
                                new_packet->type = PKT_FILE_UPLOAD_MIDDLE;
                                // Packet is now created

                                // Fill the payload
                                for (i = 0; i < MAX_MSG_LENGTH; i++) {
                                    new_packet->payload[i] = string[i];
                                }
                                new_packet->length = MAX_MSG_LENGTH;

                                // Add the job to the job queue
                                new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                                new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                                new_job2->packet = new_packet;
                                h_job_q_add(&job_q, new_job2);
                            }

                            /*
                             * Create the end packet which
                             * has the file contents
                             */
                            new_packet = (struct packet *) malloc(sizeof(struct packet));
                            new_packet->dst = new_job->file_upload_dst;
                            new_packet->src = (char) host_id;
                            new_packet->type = PKT_FILE_UPLOAD_END;

                            for (i = 0; i < n; i++) {
                                new_packet->payload[i] = buffer[i];
                            }

                            new_packet->length = n;

                            /*
                             * Create a job to send the packet
                             * and put the job in the job queue
                             */

                            new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                            new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                            new_job2->packet = new_packet;
                            h_job_q_add(&job_q, new_job2);

                            free(new_job);
                        } else {
                            /* Didn't open file */
                            // Make this do something
                            printf("Failed to open file named: %s\n", name);
                        }
                    }
                    break;

                    /* The next two jobs are for the receiving host */

                case JOB_FILE_UPLOAD_RECV_START:

                    /* Initialize the file buffer data structure */
                    file_buf_init(&f_buf_upload);

                    /*
                     * Transfer the file name in the packet payload
                     * to the file buffer data structure
                     */
                    file_buf_put_name(&f_buf_upload, new_job->packet->payload, new_job->packet->length);

                    free(new_job->packet);
                    free(new_job);
                    break;

                case JOB_FILE_UPLOAD_RECV_MIDDLE:
                    // This adds the data to the file buffer and then frees the packet and job
                    file_buf_add(&f_buf_upload, new_job->packet->payload, new_job->packet->length);

                    free(new_job->packet);
                    free(new_job);
                    break;

                case JOB_FILE_UPLOAD_RECV_END:

                    /*
                     * Download packet payload into file buffer
                     * data structure
                     */
                    file_buf_add(&f_buf_upload, new_job->packet->payload, new_job->packet->length);

                    free(new_job->packet);
                    free(new_job);

                    if (dir_valid == 1) {

                        /*
                         * Get file name from the file buffer
                         * Then open the file
                         */
                        file_buf_get_name(&f_buf_upload, string);
                        n = sprintf(name, "./%s/%s", dir, string);
                        name[n] = '\0';
                        fp = fopen(name, "w");

                        if (fp != NULL) {
                            /*
                             * Write contents in the file
                             * buffer into file
                             */

                            while (f_buf_upload.occ > 0) {
                                n = file_buf_remove(&f_buf_upload, string, PKT_PAYLOAD_MAX);
                                string[n] = '\0';
                                n = fwrite(string, sizeof(char), n, fp);
                            }

                            fclose(fp);
                        }
                    }
                    break;
                case JOB_DNS_REGISTER_WAIT_FOR_REPLY:
                    // Wait for the DNS registration to reply
                    if (dns_register_received == 1) {
                        if (strncmp(dnsRegisterBuffer, "S", 1) == 0) {
                            n = sprintf(man_reply_msg, "Successfully registered domain name");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else if (strncmp(dnsRegisterBuffer, "FN", 2) == 0) {
                            n = sprintf(man_reply_msg, "Failed to register: Name too long");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else if (strncmp(dnsRegisterBuffer, "FI", 2) == 0) {
                            n = sprintf(man_reply_msg, "Failed to register: Name is Invalid");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else if (strncmp(dnsRegisterBuffer, "FA", 2) == 0) {
                            n = sprintf(man_reply_msg, "Failed to register: Already registered");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else {
                            n = sprintf(man_reply_msg, "Failed to parse register response");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        }
                        memset(dnsRegisterBuffer, 0, MAX_DOMAIN_NAME);
                    } else if (new_job->ping_timer > 1) {
                        new_job->ping_timer--;
                        h_job_q_add(&job_q, new_job);
                    } else { /* Time out */
                        n = sprintf(man_reply_msg, "DNS registration time out!");
                        man_reply_msg[n] = '\0';
                        write(man_port->send_fd, man_reply_msg, n + 1);
                        free(new_job);
                    }
                    break;
                case JOB_DNS_LOOKUP_WAIT_FOR_REPLY:
                    if (dns_lookup_received == 1) {
                        if (strncmp(dnsLookupBuffer, "FAIL", 4) == 0) {
                            n = sprintf(man_reply_msg, "DNS lookup failed.");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else {
                            dnsLookupResponse = (int) dnsLookupBuffer[0];
                            n = sprintf(man_reply_msg, "DNS lookup response %i.", dnsLookupResponse);
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        }
                        memset(dnsLookupBuffer, 0, MAX_DOMAIN_NAME);
                    } else if (new_job->ping_timer > 1) {
                        new_job->ping_timer--;
                        h_job_q_add(&job_q, new_job);
                    } else { /* Time out */
                        n = sprintf(man_reply_msg, "DNS lookup time out!");
                        man_reply_msg[n] = '\0';
                        write(man_port->send_fd, man_reply_msg, n + 1);
                        free(new_job);
                    }
                    break;
                case JOB_DNS_PING_WAIT_FOR_REPLY:
                    if (dns_lookup_received == 1) {
                        if (strncmp(dnsLookupBuffer, "FAIL", 4) == 0) {
                            n = sprintf(man_reply_msg, "DNS lookup failed.");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else {
                            // Ping host using returned host id
                            dnsLookupResponse = (int) dnsLookupBuffer[0];

                            // Create new packet to request ping
                            new_packet = (struct packet *) malloc(sizeof(struct packet));
                            new_packet->src = (char) host_id;
                            new_packet->dst = (char) dnsLookupResponse;
                            new_packet->type = (char) PKT_PING_REQ;
                            new_packet->length = 0;

                            // Free JOB_DNS_PING_WAIT_FOR_REPLY
                            free(new_job);

                            // Create job to send ping request
                            new_job = (struct host_job *) malloc(sizeof(struct host_job));
                            new_job->packet = new_packet;
                            new_job->type = JOB_SEND_PKT_ALL_PORTS;
                            h_job_q_add(&job_q, new_job);

                            // Create job to wait for ping request
                            new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                            ping_reply_received = 0;
                            new_job2->type = JOB_PING_WAIT_FOR_REPLY;
                            new_job2->ping_timer = 10;
                            h_job_q_add(&job_q, new_job2);
                        }
                        memset(dnsLookupBuffer, 0, MAX_DOMAIN_NAME);
                        dns_lookup_received = 0;
                    } else if (new_job->ping_timer > 1) {
                        new_job->ping_timer--;
                        h_job_q_add(&job_q, new_job);
                    } else { /* Time out */
                        n = sprintf(man_reply_msg, "DNS lookup time out!");
                        man_reply_msg[n] = '\0';
                        write(man_port->send_fd, man_reply_msg, n + 1);
                        free(new_job);
                    }
                    break;
                case JOB_DNS_DOWNLOAD_WAIT_FOR_REPLY:
                    if (dns_lookup_received == 1) {
                        if (strncmp(dnsLookupBuffer, "FAIL", 4) == 0) {
                            n = sprintf(man_reply_msg, "DNS lookup failed.");
                            man_reply_msg[n] = '\0';
                            write(man_port->send_fd, man_reply_msg, n + 1);
                            free(new_job);
                        } else {
                            // download from host using returned host id
                            dnsLookupResponse = (int) dnsLookupBuffer[0];

                            // Create new packet to request download
                            new_packet = (struct packet *) malloc(sizeof(struct packet));
                            new_packet->src = (char) host_id;
                            new_packet->dst = (char) dnsLookupResponse;
                            new_packet->type = (char) PKT_FILE_DOWNLOAD_REQ;
                            for (i = 0; new_job->fname_download[i] != '\0'; i++) {
                                new_packet->payload[i] = new_job->fname_download[i];
                            }
                            new_packet->payload[i] = '\0';
                            new_packet->length = i;

                            // Free JOB_DNS_DOWNLOAD_WAIT_FOR_REPLY
                            free(new_job);

                            // Create job to send ping request
                            new_job2 = (struct host_job *) malloc(sizeof(struct host_job));
                            new_job2->packet = new_packet;
                            new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                            h_job_q_add(&job_q, new_job2);
                        }
                        memset(dnsLookupBuffer, 0, MAX_DOMAIN_NAME);
                        dns_lookup_received = 0;
                    } else if (new_job->ping_timer > 1) {
                        new_job->ping_timer--;
                        h_job_q_add(&job_q, new_job);
                    } else { /* Time out */
                        n = sprintf(man_reply_msg, "DNS lookup time out!");
                        man_reply_msg[n] = '\0';
                        write(man_port->send_fd, man_reply_msg, n + 1);
                        free(new_job);
                    }
                    break;
            }
        }


        /* The host goes to sleep for 10 ms */
        usleep(TENMILLISEC);

    } /* End of while loop */

}
