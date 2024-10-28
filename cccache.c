#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
#include <errno.h>
#include <sys/mman.h>
#include "data.h"
#include <time.h>

#define BUFFERSIZE 1024
#define ENABLE_ERROR_LOG    0

#if ENABLE_ERROR_LOG
#define LOG(x, ...) \
do \
    { \
        printf("%s: ", __progname); \
        printf(x, __VA_ARGS__); \
    } \
while (0)
#else
#define LOG(x, ...) \
do ;\
while (0)
#endif

extern const char* __progname;

void print_bytes(void *data, ssize_t size) {
    char test[size];
    ssize_t i;

    memcpy(test, data, size);
    for (i = 0; i < size; i++) {
        printf("%d%s", test[i], (((i+1) % 2) > 0) ? "" : "\n");
    }
}

// Converts date from epoc to a string
void convert_epoch_h(uint32_t *time, const char *what) {
    if (*time == 0)
        printf("\t\t%s: %d\n", what, *time);
    else {
        time_t t = (long) *time;
        char timebuf[100];
        struct tm  ts;
        ts = *localtime(&t);
        strftime(timebuf, sizeof(timebuf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
        printf("\t\t%s: %s\n", what, timebuf);
    }
}

// Get the size of the file.
// It takes a file descriptor.
int get_file_size(int fd) {
    int ret;
    long length = lseek(fd, 0, SEEK_END);

    if (length < 0) {
        int err = errno;
        printf("Error while seeking to the end of the file (%s)\n", strerror(err));
        ret = -1;
        goto fail;
    }

    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) {
        int err = errno;
        printf("Error while seeking to the beginning of the file (%s)\n", strerror(err));
        ret = -1;
        goto fail;
    }
return length;
fail:
    return ret;
}

// Check the file header
void check_file_header(void **data, ssize_t *size) {
    int rc;
    uint8_t result;

    rc = getBE(data, &result, size);
    if (rc < 0) {
        printf("Error while reading file header number\n");
        exit(EXIT_FAILURE);
    }
    if (result != 5) {
        printf("The file doesn't seem to be a ccache file\n");
        exit(EXIT_FAILURE);
    }
    LOG("File header number: %d\n", result);
    rc = getBE(data, &result, size);
    if (rc < 0) {
        printf("Error while reading file header version\n");
        exit(EXIT_FAILURE);
    }
    if (result != 4) {
        printf("The file doesn't seem to be a ccache file version 4\n");
        printf("This program works only with version 4\n");
        exit(EXIT_FAILURE);
    }
    LOG("File header version: %d\n", result);
}

void check_field(void **data, ssize_t *size) {
    int rc;
    uint16_t tag;
    uint16_t length;
    uint32_t time1;
    uint32_t time2;

    rc = get_and_swap(data, &tag, size, 16);
    if (rc < 0) {
        printf("Error while reading field tag\n");
        exit(EXIT_FAILURE);
    }
    LOG("Field tag: %d\n", tag);

    rc = get_and_swap(data, &length, size, 16);
    if (rc < 0) {
        printf("Error while reading field length\n");
        exit(EXIT_FAILURE);
    }
    LOG("Field length: %d\n", length);

    rc = get_and_swap(data, &time1, size, 32);
    if (rc < 0) {
        printf("Error while reading first time\n");
        exit(EXIT_FAILURE);
    }
    LOG("Field length: %d\n", time1);

    rc = get_and_swap(data, &time2, size, 32);
    if (rc < 0) {
        printf("Error while reading second time\n");
        exit(EXIT_FAILURE);
    }
    LOG("Field length: %d\n", time2);
}

void check_header(void **data, ssize_t *size) {
    int rc;
    uint16_t length;

    rc = get_and_swap(data, &length, size, 16);
    if (rc < 0) {
        printf("Error while reading header length\n");
        exit(EXIT_FAILURE);
    }
    LOG("Header length: %d\n", length);
    check_field(data, size);
}


void check_data(void **data, struct data *result, ssize_t *size) {
    int rc;
    uint32_t length;

    rc = get_and_swap(data, &length, size, 32);
    if (rc < 0) {
        printf("Error while reading data length\n");
        exit(EXIT_FAILURE);
    }
    LOG("    Data length: %d\n", length);

    result->length = length;
    result->value = (char *) malloc(length);
    memcpy(result->value, *data, length);
    *data = (uint8_t *)*data + length;
    *size -= length;
    LOG("Data value: %.*s\n", result->length, result->value);
}

int check_principal(void **data, struct principal *princ, ssize_t *leftover) {
    int rc; 
    ssize_t i;
    uint32_t name_type;
    uint32_t count;
    struct data *realm;

    rc = get_and_swap(data, &name_type, leftover, 32);
    if (rc < 0) {
        printf("Error while reading principal name type\n");
        return -1;
    }
    LOG("  Principal name type: %d\n", name_type);
    princ->name_type = name_type;

    rc = get_and_swap(data, &count, leftover, 32);
    if (rc < 0) {
        printf("Error while reading principal components count\n");
        return -1;
    }
    LOG("  Principal components count %d\n", count);
    princ->comp_count = count;

    //realm
    realm = (struct data *)malloc(sizeof(struct data));
    if (!realm) {
        printf("Error allocating memory for the realm\n");
        return -1;
    }
    check_data(data, realm, leftover);
    princ->realm = realm;

    //components
    struct data *comp[count];
    for (i = 0; i < count; i++) {
        comp[i] = (struct data *)malloc(sizeof(struct data));
        check_data(data, comp[i], leftover);
    }
    princ->components = comp;
    return 0;
}

void check_keyblock(void **data, struct principal *princ, ssize_t *size) {
    int rc;
    uint16_t enc_type;
    struct data *dt;
    LOG("-- Keyblock%s\n", "");
    rc = get_and_swap(data, &enc_type, size, 16);
    if (rc < 0) {
        printf("Error while reading enc type type\n");
        exit(EXIT_FAILURE);
    }
    LOG("Enc type: 0x%x\n", enc_type);
    dt = (struct data *)malloc(sizeof(struct data));
    check_data(data, dt, size);
}

int get_default_principal(void **data, struct principal *princ, ssize_t *size) {
    return check_principal(data, princ, size);
}

int check_address(void **data, struct address *addr, ssize_t *size) {
    int rc;
    uint16_t addr_type;
    struct data * dt;

    LOG("-- address%s\n", "");
    rc = get_and_swap(data, &addr_type, size, 16);
    if (rc < 0) {
        printf("Error while reading address type type\n");
        exit(EXIT_FAILURE);
    }
    LOG("address type: 0x%x\n", addr_type);
    dt = (struct data *) malloc(sizeof(struct data));
    check_data(data, dt, size);
    printf("address type: 0x%x value: %s\n", addr_type, dt->value);
    return 0;
}

int check_addresses(void **data, struct addresses *addrs, ssize_t *size) {
    int rc;
    ssize_t i;
    uint32_t count;
    struct address *addr;

    LOG("-- addresses%s\n", "");
    rc = get_and_swap(data, &count, size, 32);
    if (rc < 0) {
        printf("Error while reading enc type type\n");
        exit(EXIT_FAILURE);
    }
    LOG("count: %d\n", count);
    for (i = 0; i < count; i++){
        addr = (struct address *) malloc(sizeof(struct address));
        check_address(data, addr, size);
    }
    return 0;
}


int check_authdata(void **data, struct authdata *addrs, ssize_t *size) {
    int rc;
    uint16_t ad_type;
    struct data * dt; 

    LOG("-- auth data%s\n", "");
    rc = get_and_swap(data, &ad_type, size, 16);
    if (rc < 0) {
        printf("Error while reading address type type\n");
        exit(EXIT_FAILURE);
    }
    LOG("address type: 0x%x\n", ad_type);
    dt = (struct data *) malloc(sizeof(struct data));
    check_data(data, dt, size);
    printf("address type: 0x%x value: %s\n", ad_type, dt->value);
    return 0;
}

int check_authdatas(void **data, struct authdatas *addrs, ssize_t *size) {
    int rc;
    ssize_t i;
    uint32_t count;
    struct authdata *auth;

    LOG("-- auth datas%s\n", "");
    rc = get_and_swap(data, &count, size, 32);
    if (rc < 0) {
        printf("Error while reading auth_datatas count type\n");
        return -1;
    }
    LOG("count: %d\n", count);
    for (i = 0; i < count; i++){
        auth = (struct authdata *) malloc(sizeof(struct authdata));
        check_authdata(data, auth, size);
    }
    return 0;
}


int check_credentials(void **data, ssize_t *size) {
    int ret;
    ssize_t i;
    printf("-- Credentials\n");
    printf("%-5s\t%-40s\t\t\t\t%s\n", "num", "Client", "Server");
    i = 0;
    while (*size > 0 && *size > sizeof(struct credential)) {
        uint32_t auth_time = 0, start_time = 0, end_time = 0;
        uint32_t renew_till = 0, ticket_flags = 0;
        uint8_t is_skey = 0;
        printf("%-5zd\t", i);
        struct principal *client;
        struct data *second_ticket;
        struct data *ticket;
        struct authdatas *auths;
        struct addresses *addrs;

        client = (struct principal *) malloc(sizeof(struct principal));
        ret = check_principal(data, client, size);
        if(ret < 0) {
            return -1;
        }
        printf("client: %.*s/%.*s@%.*s\t\t", client->components[0]->length, client->components[0]->value, 
                                           client->components[1]->length, client->components[1]->value,
                                           client->realm->length, client->realm->value);

        struct principal *server;
        server = (struct principal *) malloc(sizeof(struct principal));
        check_principal(data, server, size);
        if (ret < 0) {
            return -1;
        }
        printf("server: %.*s/%.*s@%.*s\n", server->components[0]->length, server->components[0]->value, 
                                           server->components[1]->length, server->components[1]->value,
                                           server->realm->length, server->realm->value);

        check_keyblock(data, server, size);

        ret = get_and_swap(data, &auth_time, size, 32);
        if (ret < 0) {
            printf("Error while reading auth time\n");
            exit(EXIT_FAILURE);
        }
        convert_epoch_h(&auth_time, "Auth time");

        ret = get_and_swap(data, &start_time, size, 32);
        if (ret < 0) {
            printf("Error while reading start time\n");
            exit(EXIT_FAILURE);
        }
        convert_epoch_h(&start_time, "Start time");

        ret = get_and_swap(data, &end_time, size, 32);
        if (ret < 0) {
            printf("Error while reading end time\n");
            exit(EXIT_FAILURE);
        }
        convert_epoch_h(&end_time, "End time");

        ret = get_and_swap(data, &renew_till, size, 32);
        if (ret < 0) {
            printf("Error while reading renew_till date\n");
            exit(EXIT_FAILURE);
        }
        convert_epoch_h(&renew_till, "Renew till");

        ret = getBE(data, &is_skey, size);
        if (ret < 0) {
            printf("Error while reading is_key\n");
            exit(EXIT_FAILURE);
        }
        printf("\t\tis_key: %d\n", is_skey);

        ret = get_and_swap(data, &ticket_flags, size, 32);
        if (ret < 0) {
            printf("Error while reading is_key\n");
            exit(EXIT_FAILURE);
        }
        char * buffer = malloc(MAXSTRINGLEN);
        flag_string(ticket_flags, buffer);
        printf("\t\tFlags: %x (%s)\n", ticket_flags, buffer);

        addrs = (struct addresses *) malloc(sizeof(struct addresses));
        check_addresses(data, addrs, size);
        if (ret < 0) {
            return -1;
        }

        auths = (struct authdatas *) malloc(sizeof(struct authdatas));
        check_authdatas(data, auths, size);
        if (ret < 0) {
            return -1;
        }

        ticket = (struct data *) malloc(sizeof(struct data));
        check_data(data, ticket, size);

        second_ticket = (struct data *) malloc(sizeof(struct data));
        check_data(data, second_ticket, size);

        printf("\n");
        // Clean up before restarting
        free(client);
        free(server);
        free(addrs);
        free(auths);
        free(ticket);
        free(second_ticket);
        i++;
    }
    return 0;
}

void usage(char *exe) {
    printf("Usage: %s [-v] ccache_file\n", exe);
}

int main(int argc, char *argv[]) {
    ssize_t size;
    int ret, fd, err, opt, verbose = 0; 
    char *buffer;
    char *filename;
    struct principal *princ;

    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
        case 'v':
            verbose++;
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    filename = argv[optind];
    if (!filename) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        err = errno;
        printf("Error opening the file %s: %s\n", filename, strerror(err));
        return EXIT_FAILURE;
    }

    size = get_file_size(fd);
    LOG("File size: %zd\n", size);
    if (size < 0) {
        return EXIT_FAILURE;
    }

    buffer = (char *) malloc(size);
    if (!buffer) {
        printf("Error allocating memory for the ccache file %s\n", filename);
        return EXIT_FAILURE;
    }
    read(fd, buffer, size);
    void *data = mmap(0, size, PROT_READ, MAP_PRIVATE|MAP_LOCKED|MAP_POPULATE, fd, 0);
    void *dataptr = data;
    if(dataptr == MAP_FAILED) {
        goto fail_close;
    }
    // File header
    LOG("dataptr addr (mmap): %p\n", dataptr);
    check_file_header(&dataptr, &size);
    check_header(&dataptr, &size);

    // Get the default principal
    princ = (struct principal *) malloc(sizeof(struct principal));
    ret = get_default_principal(&dataptr, princ, &size);
    if(ret < 0) {
        goto fail_close;
    }
    printf("Default principal: %s/%s@%s\n", princ->components[0]->value, princ->components[1]->value, princ->realm->value);
    
    // Get the credentials
    check_credentials(&dataptr, &size);

    munmap(dataptr, size); 
    close(fd);
    return EXIT_SUCCESS;

fail_close:
    munmap(dataptr, size); 
    close(fd);
    return EXIT_FAILURE;
}
