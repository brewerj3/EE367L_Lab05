#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "packet.h"
#include "net.h"

#define MAX_NAME_LENGTH 50


struct name_buf {
    char name[MAX_NAME_LENGTH];
    int name_length;
    int src;
};

void job_q_add(struct server_job_queue *j_q, struct server_job *j) {
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
struct server_job *job_q_remove(struct server_job_queue *j_q) {
    struct server_job *j;

    if (j_q->occ == 0) return (NULL);
    j = j_q->head;
    j_q->head = (j_q->head)->next;
    j_q->occ--;
    return (j);
}

/* Initialize job queue */
void job_q_init(struct server_job_queue *j_q) {
    j_q->occ = 0;
    j_q->head = NULL;
    j_q->tail = NULL;
}

int job_q_num(struct server_job_queue *j_q) {
    return j_q->occ;
}

_Noreturn void server_main(int host_id) {
    if (host_id != 100) {
        printf("Invalid host_id: %i\n", host_id);
        exit(0);
    }

    struct net_port *node_port_list;
    struct net_port **node_port;    // Array of pointers to node pointers
    int node_port_num;              // Number of node ports

    int localRootID = host_id;
    int localRootDist = 0;
    int localParent = -1;
    int controlCount = 0;

    int i, k, n;
    int dst;                        // Packet destination

    struct packet *in_packet;
    struct packet *new_packet;

    struct net_port *p;
    struct server_job *new_job;
    struct server_job *new_job2;

    struct server_job_queue job_q;

    // Create the DNS naming table
    char namingTable[NAMING_TABLE_SIZE][MAX_NAME_LENGTH + 1];

    // Make all entries start with the null character
    for (i = 0; i < NAMING_TABLE_SIZE; i++) {
        memset(namingTable[i], 0, MAX_NAME_LENGTH - 1);
    }

    // Create an array node_port to store the network link ports at the host.
    node_port_list = net_get_port_list(100);

    // Count the number of network link ports
    node_port_num = 0;
    for (p = node_port_list; p != NULL; p = p->next) {
        node_port_num++;
    }

    // Create memory space for the array
    node_port = (struct net_port **) malloc(node_port_num * sizeof(struct net_port *));

    /* Load ports into the array */
    p = node_port_list;

    for (k = 0; k < node_port_num; k++) {
        node_port[k] = p;
        p = p->next;
    }

    // Initialize the job queue
    job_q_init(&job_q);

    // Create register attempt response
    enum registerAttempt successFailure;

    // Start main loop
    while (1) {

        // Send control packets every 40 milliseconds
        controlCount++;
        if (controlCount > 4) {
            controlCount = 0;

            // Create control packet
            new_packet = (struct packet *) malloc(sizeof(struct packet));
            new_packet->src = (char) host_id;
            new_packet->type = (char) PKT_CONTROL_PACKET;
            new_packet->length = 4;
            new_packet->payload[0] = (char) localRootID;
            new_packet->payload[1] = (char) localRootDist;
            new_packet->payload[2] = 'D';
            // Set packetSenderChild when sending the packet

            // Create a new job to send the packet, then add to queue
            new_job = (struct server_job *) malloc(sizeof(struct server_job));
            new_job->packet = new_packet;
            new_job->type = JOB_SEND_PKT_ALL_PORTS;
            job_q_add(&job_q, new_job);
        }

        // Get packets from incoming links and translates to jobs
        // Put jobs in job queue
        for (k = 0; k < node_port_num; k++) {
            in_packet = (struct packet *) malloc(sizeof(struct packet));
            n = packet_recv(node_port[k], in_packet);

            if ((n > 0) && ((int) in_packet->dst == host_id)) {
                // Handle packets that are not control packets
                new_job = (struct server_job *) malloc(sizeof(struct server_job));
                new_job->in_port_index = k;
                new_job->packet = in_packet;

                // Switch statement to handle packet types
                switch (in_packet->type) {
                    case (char) PKT_PING_REQ:
                        new_job->type = JOB_PING_SEND_REPLY;
                        job_q_add(&job_q, new_job);
                        break;
                    case (char) PKT_PING_REPLY:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_UPLOAD_START:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_UPLOAD_END:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_UPLOAD_MIDDLE:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_FILE_DOWNLOAD_REQ:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_CONTROL_PACKET:     // The DNS Server does not process control packets
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_DNS_REGISTER:    // This adds a job to register a new domain name
                        new_job->type = JOB_REGISTER_NEW_DOMAIN;
                        job_q_add(&job_q, new_job);
                        break;
                    case (char) PKT_DNS_REGISTER_REPLY:
                        free(in_packet);
                        free(new_job);
                        break;
                    case (char) PKT_DNS_LOOKUP:
                        new_job->type = JOB_DNS_PING_REQ;
                        job_q_add(&job_q, new_job);
                        break;
                    default:
                        free(in_packet);
                        free(new_job);
                }
            } else {
                free(in_packet);
            }
        }

        // Execute one job in the job queue
        if (job_q_num(&job_q) > 0) {

            // Get a new job from the job queue
            new_job = job_q_remove(&job_q);

            // Process the job
            switch (new_job->type) {
                case JOB_SEND_PKT_ALL_PORTS:
                    for (k = 0; k < node_port_num; k++) {
                        packet_send(node_port[k], new_job->packet);
                    }
                    free(new_job->packet);
                    free(new_job);
                    break;
                case JOB_PING_SEND_REPLY:
                    /* Create ping reply packet */
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->dst = new_job->packet->src;
                    new_packet->src = (char) host_id;
                    new_packet->type = PKT_PING_REPLY;
                    new_packet->length = 0;

                    /* Create job for the ping reply */
                    new_job2 = (struct server_job *) malloc(sizeof(struct server_job));
                    new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                    new_job2->packet = new_packet;

                    /* Enter job in the job queue */
                    job_q_add(&job_q, new_job2);

                    /* Free old packet and job memory space */
                    free(new_job->packet);
                    free(new_job);
                    break;
                case JOB_REGISTER_NEW_DOMAIN:
                    // Attempt to add new Domain name to naming table
                    if(strlen(new_job->packet->payload) > MAX_NAME_LENGTH) {
                        successFailure = NAME_TOO_LONG;
                    } else if (namingTable[new_job->packet->src][0] != '\0') {
                        successFailure = ALREADY_REGISTERED;
                    } else {
                        successFailure = SUCCESS;
                        for(i = 0; i < MAX_NAME_LENGTH; i++) {
                            if(new_job->packet->payload[i] == ' ') {
                                successFailure = INVALID_NAME;
                                break;
                            }
                        }
                    }
                    if(successFailure == SUCCESS) {
                        strcpy(namingTable[(int) new_job->packet->src], new_job->packet->payload);
                    }

                    // Create DNS registration reply packet
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->dst = new_job->packet->src;
                    new_packet->src = (char) host_id;
                    new_packet->type = PKT_DNS_REGISTER_REPLY;

                    // Create Job for DNS reply
                    new_job2 = (struct server_job *) malloc(sizeof(struct server_job));
                    new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                    new_job2->packet = new_packet;

                    switch(successFailure) {
                        case SUCCESS:
                            new_packet->length = 1;
                            strcpy(new_packet->payload, "S\0");
                            job_q_add(&job_q, new_job2);
                            break;
                        case NAME_TOO_LONG:
                            new_packet->length = 2;
                            strcpy(new_packet->payload, "FN\0");
                            job_q_add(&job_q, new_job2);
                            break;
                        case INVALID_NAME:
                            new_packet->length = 2;
                            strcpy(new_packet->payload, "FI\0");
                            job_q_add(&job_q, new_job2);
                            break;
                        case ALREADY_REGISTERED:
                            new_packet->length = 2;
                            strcpy(new_packet->payload, "FA\0");
                            job_q_add(&job_q, new_job2);
                            break;
                        default:
                            free(new_packet);
                            free(new_job2);
                    }

                    free(new_job->packet);
                    free(new_job);
                    break;
                case JOB_DNS_PING_REQ:
                    for(i = 0; i < NAMING_TABLE_SIZE; i++) {
                        if(strcmp(new_job->packet->payload, namingTable[i]) == 0) {
                            break;
                        }
                    }

                    // Create reply packet
                    new_packet = (struct packet *) malloc(sizeof(struct packet));
                    new_packet->dst = new_job->packet->src;
                    new_packet->src = (char) host_id;
                    new_packet->type = PKT_DNS_LOOKUP_REPLY;
                    if(i > NAMING_TABLE_SIZE) {
                        new_packet->length = 5;
                        strcpy(new_packet->payload, "FAIL\0");
                    } else {
                        new_packet->length = 1;
                        new_packet->payload[0] = (char) i;
                    }

                    // Create job for DNS lookup reply
                    new_job2 = (struct server_job *) malloc(sizeof(struct server_job));
                    new_job2->type = JOB_SEND_PKT_ALL_PORTS;
                    new_job2->packet = new_packet;

                    // Add job to job queue
                    job_q_add(&job_q, new_job2);
                    free(new_job->packet);
                    free(new_job);
                    break;
                default:
                    free(new_job->packet);
                    free(new_job);
            }
        }

        // The host goes to sleep for 10 ms
        usleep(TENMILLISEC);
    }
}