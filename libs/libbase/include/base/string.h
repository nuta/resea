#ifndef __BASE_STRING_H__
#define __BASE_STRING_H__

#include <base/types.h>

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
