/**
 * file name: comp_decomp_data.h
 * author: minhnhut.n
 */

#ifndef COMP_DECOMP_DATA_H
#define COMP_DECOMP_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>    /* malloc / free  */
#include <stdio.h>     /* snprintf       */
#include <string.h>    /* strlen/strcpy/memcpy/strchr/strcmp */

#ifdef __cplusplus
extern "C" {
#endif

#define CMPD_KV_SEP   ':'   /* level-2 separator: key : value   */
#define CMPD_PAIR_SEP '|'   /* level-1 separator: pair | pair   */

/**
 * @brief Compress any number of (key, value) pairs into one heap buffer.
 *
 * Variadic: pass key/value arguments in order and terminate the list with
 * `(const char*)NULL`. Example:
 *      char* s = cmpds_compress("mode", "STA", "ssid", "Thoai Hanh",
 *                               "pass", "hanh12345", (const char*)NULL);
 * Returns a heap-allocated string the caller must free(), or NULL on failure.
 */
static inline char* cmpds_compress(const char* key, const char* value, ...) {
    va_list ap;
    va_start(ap, value);

    const char* k  = key;
    const char* v  = value;
    int    pairs   = 0;
    size_t len     = 1;      /* room for the nul terminator */
    while (k != NULL) {
        if (pairs > 0) {
            len += 1;                                /* '|' between pairs */
        }
        len += strlen(k);                            /* key              */
        len += 1;                                    /* ':'              */
        if (v != NULL) {
            len += strlen(v);                        /* value            */
        }
        pairs++;
        k = va_arg(ap, const char*);
        v = va_arg(ap, const char*);
    }
    va_end(ap);

    if (pairs == 0) {
        return NULL;
    }

    char* buf = malloc(len);
    if (buf == NULL) {
        return NULL;
    }

    va_start(ap, value);
    char* p = buf;
    k = key;
    v = value;
    pairs = 0;
    while (k != NULL) {
        if (pairs > 0) {
            *p++ = CMPD_PAIR_SEP;
        }
        size_t n = strlen(k);
        memcpy(p, k, n);
        p += n;
        *p++ = CMPD_KV_SEP;
        if (v != NULL) {
            n = strlen(v);
            memcpy(p, v, n);
            p += n;
        }
        pairs++;
        k = va_arg(ap, const char*);
        v = va_arg(ap, const char*);
    }
    *p = '\0';
    va_end(ap);

    return buf;
}

static inline bool cmpds_decompress_get(const char* payload, const char* key,
                                        char* out, size_t out_size) {
    if (payload == NULL || key == NULL || out == NULL || out_size == 0) {
        return false;
    }

    /* strtok_r mutates its input, so parse an owned copy. */
    size_t copy_len = strlen(payload) + 1;
    char*  copy     = malloc(copy_len);
    if (copy == NULL) {
        return false;
    }
    strcpy(copy, payload);

    bool found = false;
    char* save = NULL;
    char* pair = strtok_r(copy, "|", &save);        /* level 1: "k:v" */
    while (pair != NULL) {
        char* colon = strchr(pair, ':');             /* level 2: k : v  */
        if (colon != NULL) {
            *colon = '\0';
            if (strcmp(pair, key) == 0) {
                snprintf(out, out_size, "%s", colon + 1);
                found = true;
                break;
            }
        }
        pair = strtok_r(NULL, "|", &save);
    }

    free(copy);
    return found;
}

#ifdef __cplusplus
}
#endif

#endif /* COMP_DECOMP_DATA_H */