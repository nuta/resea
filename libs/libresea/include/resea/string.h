#ifndef __RESEA_STRING_H__
#define __RESEA_STRING_H__

#include <resea.h>

size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy_s(char *dst, size_t dst_len, const char *src);
char *strncpy_s(char *dst, size_t dst_len, const char *src, size_t copy_len);
int strncmp(const char *s1, const char *s2, size_t len);
int memcmp(const void *s1, const void *s2, size_t len);
void *memset(void *dst, int ch, size_t len);
// TODO: Use __builtin_memcpy. Perhaps it optimizes a constant-sized small copy.
void *memcpy(void *dst, const void *src, size_t len);
void *memcpy_s(void *dst, size_t dst_len, const void *src, size_t copy_len);

#define STRING_INITIAL_CAPACITY 8

/// An ASCII string.
struct string {
    /// The null-terminated string data.
    char *data;
    /// The length of the string.
    int len;
    /// The capacity of the data.
    int capacity;
};

typedef struct string *string_t;

string_t string_new(void);
void string_delete(string_t str);
string_t string_from(const char *cstr);
int string_len(string_t str);
const char *string_cstr(string_t str);
char string_get_char(string_t str, int index);
void string_set_char(string_t str, int index, char ch);
void string_append(string_t str, string_t other);
void string_append_cstr(string_t str, const char *cstr);
int string_find(string_t haystack, string_t needle);
int string_find_cstr(string_t haystack, const char *needle);
bool string_startswith(string_t str, string_t prefix);
bool string_startswith_cstr(string_t str, const char *prefix);

#endif
