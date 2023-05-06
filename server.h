#ifndef EE367L_LAB05_SERVER_H
#define EE367L_LAB05_SERVER_H

#define MAX_NAME_LENGTH 50
#define NAMING_TABLE_SIZE 256
#define TENMILLISEC 10000   /* 10 millisecond sleep */


enum host_job_type {
    JOB_SEND_PKT_ALL_PORTS,
    JOB_PING_SEND_REPLY,
    JOB_REGISTER_NEW_DOMAIN,
    JOB_DNS_PING_REQ
};

enum yesNo {
    YES,
    NO
};

enum registerAttempt {
    SUCCESS,
    NAME_TOO_LONG,
    INVALID_NAME,
    ALREADY_REGISTERED
};

struct host_job {
    enum host_job_type type;
    struct packet *packet;
    int in_port_index;
    int out_port_index;
    struct host_job *next;
};
struct job_queue {
    struct host_job *head;
    struct host_job *tail;
    int occ;
};

void job_q_init(struct job_queue *j_q);

void job_q_add(struct job_queue *j_q, struct host_job *j);

int job_q_num(struct job_queue *j_q);

struct host_job *job_q_remove(struct job_queue *j_q);

_Noreturn void server_main(int host_id);

#endif //EE367L_LAB05_SERVER_H
