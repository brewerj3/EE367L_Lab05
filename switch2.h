#ifndef SWITCH_H
#define SWITCH_H

#define MAX_LOOKUP_TABLE_SIZE 100

enum valid {
    False, True
};

struct switch_job_queue {
    struct switch_job *head;
    struct switch_job *tail;
    int occ;
};

struct Table {
    enum valid isValid[MAX_LOOKUP_TABLE_SIZE + 1];
    int portNumber[MAX_LOOKUP_TABLE_SIZE + 1];
};

_Noreturn void switch_main(int host_id);

#endif
