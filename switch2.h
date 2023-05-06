#ifndef SWITCH_H
#define SWITCH_H

#define MAX_LOOKUP_TABLE_SIZE 100

#define TENMILLISEC 10000   /* 10 millisecond sleep */

enum valid {
    False,
    True
};

enum host_job_type {
    JOB_SEND_PKT_ALL_PORTS,
    JOB_FORWARD_PACKET
};

struct host_job {
    enum host_job_type type;
    struct packet *packet;
    int in_port_index;
    int out_port_index;
    char fname_upload[100];
    int ping_timer;
    int file_upload_dst;
    struct host_job *next;
};


enum yesNo {
    YES,
    NO
};

struct job_queue {
    struct host_job *head;
    struct host_job *tail;
    int occ;
};

struct Table {
    enum valid isValid[MAX_LOOKUP_TABLE_SIZE];
    int destination[MAX_LOOKUP_TABLE_SIZE];
    int portNumber[MAX_LOOKUP_TABLE_SIZE];
};

_Noreturn void switch_main(int host_id);

#endif
