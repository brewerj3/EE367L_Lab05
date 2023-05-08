#ifndef EE367L_LAB05_SERVER_H
#define EE367L_LAB05_SERVER_H

#define NAMING_TABLE_SIZE 256


enum server_job_type {
    JOB_SEND_PKT_ALL_PORTS, JOB_PING_SEND_REPLY, JOB_REGISTER_NEW_DOMAIN, JOB_DNS_PING_REQ
};


enum registerAttempt {
    SUCCESS, NAME_TOO_LONG, INVALID_NAME, ALREADY_REGISTERED
};

struct server_job {
    enum server_job_type type;
    struct packet *packet;
    int in_port_index;
    int out_port_index;
    struct server_job *next;
};
struct server_job_queue {
    struct server_job *head;
    struct server_job *tail;
    int occ;
};

//void job_q_init(struct server_job_queue *j_q);

//int job_q_num(struct server_job_queue *j_q);

_Noreturn void server_main(int host_id);

#endif //EE367L_LAB05_SERVER_H
