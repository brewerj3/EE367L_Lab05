#ifndef SWITCH_H
#define SWITCH_H

#define MAX_LOOKUP_TABLE_SIZE 100

enum valid {
    False, True
};

/// Switch job queue, used by the switch to determine what job to execute
struct switch_job_queue {
    struct switch_job *head;
    struct switch_job *tail;
    int occ;
};

/// Lookup table used to forward packets when the destination is stored in the table
struct Table {
    enum valid isValid[MAX_LOOKUP_TABLE_SIZE + 2];
    int portNumber[MAX_LOOKUP_TABLE_SIZE + 2];
};

/// The main function of the switch, loops until killed by the manager
_Noreturn void switch_main(int host_id);

#endif
