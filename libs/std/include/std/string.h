#ifndef __STD_STRING_H__
#define __STD_STRING_H__

#include <types.h>

/// A string or bytes container.
typedef struct {
    /// The data buffer. It is always temrinated by '\0' regardless of whether
    /// the data is bytes or string, i.e. buf[len] == '\0' always holds. It
    /// pretty useful when you print a string in printf functions.
    ///
    /// Please note that data may contain '\0'.
    uint8_t *buf;
    /// The length of data (excluding the trailing '\0').
    size_t len;
    /// The allocated size of buf (excluding the trailing '\0').
    size_t capacity;
} *string_t;

string_t string_new(void);
string_t string_from_bytes(const void *data, size_t len);
string_t string_from_cstr(const char *cstr);
void string_delete(string_t str);
string_t string_dup(string_t str);
unsigned string_hash(string_t str);
size_t string_len(string_t str);
uint8_t *string_data(string_t str);
bool string_is_empty(string_t str);
bool string_equals(string_t str1, string_t str2);
bool string_equals_bytes(string_t str, const void *data, size_t len);
bool string_equals_cstr(string_t str, const char *cstr);
void string_append(string_t str, string_t other);
void string_append_bytes(string_t str, const void *data, size_t len);
void string_append_cstr(string_t str, const char *cstr);
int string_findfrom(string_t str, string_t needle, int from);
int string_findfrom_bytes(string_t str, const void *data, size_t len, int from);
int string_findfrom_cstr(string_t str, const char *cstr, int from);
int string_find(string_t str, string_t needle);
int string_find_bytes(string_t str, const void *data, size_t len);
int string_find_cstr(string_t str, const char *cstr);
bool string_contains(string_t str, string_t needle);
bool string_contains_bytes(string_t str, const void *data, size_t len);
bool string_contains_cstr(string_t str, const char *cstr);

#endif
