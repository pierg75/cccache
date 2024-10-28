#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
#include <sys/mman.h>
#include "data.h"

#define get_and_swap(data, result, leftover, size) (getBE##size(data, result, leftover))


#define TKT_FLG_FORWARDABLE             0x40000000
#define TKT_FLG_FORWARDED               0x20000000
#define TKT_FLG_PROXIABLE               0x10000000
#define TKT_FLG_PROXY                   0x08000000
#define TKT_FLG_MAY_POSTDATE            0x04000000
#define TKT_FLG_POSTDATED               0x02000000
#define TKT_FLG_INVALID                 0x01000000
#define TKT_FLG_RENEWABLE               0x00800000
#define TKT_FLG_INITIAL                 0x00400000
#define TKT_FLG_PRE_AUTH                0x00200000
#define TKT_FLG_HW_AUTH                 0x00100000
#define TKT_FLG_TRANSIT_POLICY_CHECKED  0x00080000
#define TKT_FLG_OK_AS_DELEGATE          0x00040000
#define TKT_FLG_ENC_PA_REP              0x00010000
#define TKT_FLG_ANONYMOUS               0x00008000


int test_flag(unsigned value, unsigned flag) {
    return (value & flag);
}

/* The buffer passed as argument must have memory allocated */
int flag_string(int flags, char *buffer) {
	int len;
    memset(buffer,0,MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_FORWARDABLE))
		strlcat(buffer, "FORWARDABLE|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_FORWARDED))
		strlcat(buffer, "FORWARDED|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_PROXIABLE))
		strlcat(buffer, "PROXIABLE|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_PROXY))
		strlcat(buffer, "PROXY|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_MAY_POSTDATE))
		strlcat(buffer, "MAY_POSTDATE|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_POSTDATED))
		strlcat(buffer, "POSTDATED|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_INVALID))
		strlcat(buffer, "INVALID|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_RENEWABLE))
		strlcat(buffer, "RENEWABLE|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_INITIAL))
		strlcat(buffer, "INITIAL|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_PRE_AUTH))
		strlcat(buffer, "PRE_AUTH|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_HW_AUTH))
		strlcat(buffer, "HW_AUTH|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_TRANSIT_POLICY_CHECKED))
		strlcat(buffer, "TRANSIT_POLICY_CHECKED|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_OK_AS_DELEGATE))
		strlcat(buffer, "OK_AS_DELEGATE|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_ENC_PA_REP))
		strlcat(buffer, "ENC_PA_REP|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_ENC_PA_REP))
		strlcat(buffer, "ENC_PA_REP|", MAXSTRINGLEN);
	if (test_flag(flags, TKT_FLG_ANONYMOUS))
		strlcat(buffer, "ANONYMOUS|", MAXSTRINGLEN);
	len = strlen(buffer);
	if (len)
		buffer[len - 1] = '\0';
	else
		strlcat(buffer, "0", MAXSTRINGLEN);
    return 0;
}


/* Get a single byte. This will advance the pointer of a byte and decrease th
 * leftover of the same amonut
 */
int getBE(void **data, uint8_t *result, ssize_t *leftover) {
    char * dataptr;
    ssize_t length = 1;
    dataptr = *data;
    if (*leftover < length) {
        return -1;
    }
    *result = *dataptr;
    *data += length;
    *leftover -= length;
    return 0; 
}

/* Get 16 bits (2 bytes) of data from a pointer and swap it (the ccache is in big
 * endian). This will advance the pointer of 2 bytes and decrease the leftover
 * of the same amount.
 */
int getBE16(void **data, uint16_t *result, ssize_t *leftover) {
    uint16_t tmp;
    memcpy(&tmp, *data, sizeof(tmp));
    ssize_t length = sizeof(tmp);
    if (*leftover < length) {
        return -1;
    }
    *result = bswap_16(tmp);
    *data += length;
    *leftover -= length;
    return 0; 
}

/* Get 32 bits (4 bytes) data from a pointer and swap it (the ccache is in big
 * endian). This will advance the pointer of 4 bytes and decrease the leftover
 * of the same amount.
 */
int getBE32(void **data, uint32_t *result, ssize_t *leftover) {
    uint32_t tmp;
    memcpy(&tmp, *data, sizeof(tmp));
    ssize_t length = sizeof(tmp);
    if (*leftover < length) {
        return -1;
    }
    *result = bswap_32(tmp);
    *data += length;
    *leftover -= length;
    return 0; 
}

/* Get a 64 bits (8 bytes) data from a pointer and swap it (the ccache is in big
 * endian). This will advance the pointer of 8 bytes and decrease the leftover
 * of the same amount.
 */
int getBE64(void **data, uint64_t *result, ssize_t *leftover) {
    uint64_t tmp;
    memcpy(&tmp, *data, sizeof(tmp));
    ssize_t length = sizeof(tmp);
    if (*leftover < length) {
        return -1;
    }
    *result = bswap_64(tmp);
    *data += length;
    *leftover -= length;
    return 0; 
}

