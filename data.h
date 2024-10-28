#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED

#include <stdint.h>
#include <stddef.h>


#define MAXSTRINGLEN                    1024

#define get_and_swap(data, result, leftover, size) (getBE##size(data, result, leftover))

int getBE(void **data, uint8_t *result, ssize_t *leftover);
int getBE16(void **data, uint16_t *result, ssize_t *leftover);
int getBE32(void **data, uint32_t *result, ssize_t *leftover);
int getBE64(void **data, uint64_t *result, ssize_t *leftover);
int flag_string(int flags, char *buffer);


struct field {
    uint16_t tag;
    uint16_t length;
    uint32_t time1;
    uint32_t time2;
}__attribute__((packed));

struct header {
    uint16_t length;
    struct field field; 
}__attribute__((packed));

struct data {
    uint32_t length;
    char * value;
}__attribute__((packed));

struct principal {
    uint32_t name_type;
    uint32_t comp_count;
    struct data *realm;
    struct data **components;
}__attribute__((packed));


struct keyblock {
    uint16_t enctype;
    struct data data;
};

 struct address {
    uint16_t addrtype;
    struct data data;
};

struct addresses {
    uint32_t count;
    struct address *addresses;
};

struct authdatas {
    uint32_t count;
    struct authdata *authdatas;
};

struct authdata {
    uint16_t ad_type;
    struct data data;
};

struct credential {
    struct principal client;
    struct principal server;
    struct keyblock keyblock;
    uint32_t authtime;
    uint32_t starttime;
    uint32_t endtime;
    uint32_t renew_till;
    uint8_t is_skey;
    uint32_t ticket_flags;
    struct addresses addresses;
    struct authdata authdata;
    struct data ticket;
    struct data second_ticket;
};
#endif
