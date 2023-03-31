#ifndef SWITCH_H
#define SWITCH_H

#define MAX_LOOKUP_TABLE_SIZE 100
enum valid {
    False,
    True
};

struct Table {
    enum valid isValid[MAX_LOOKUP_TABLE_SIZE];
    int destination[MAX_LOOKUP_TABLE_SIZE];
    int portNumber[MAX_LOOKUP_TABLE_SIZE];
};


#endif
