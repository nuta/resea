#include <std/string.h>
#include <std/malloc.h>
#include <cstring.h>

static void expand(string_t str, size_t len) {
    if (len <= str->capacity) {
        return;
    }

    str->buf = realloc(str->buf, len + 1);
    str->capacity = len;
}

string_t string_new(void) {
    string_t str = malloc(sizeof(*str));
    str->buf = malloc(1);
    str->capacity = 0;
    str->len = 0;
    str->buf[0] = '\0';
    return str;
}

string_t string_from_bytes(const void *data, size_t len) {
    string_t str = malloc(sizeof(*str));
    str->buf = malloc(len + 1);
    str->capacity = len;
    str->len = len;
    memcpy(str->buf, data, len);
    str->buf[len] = '\0';
    return str;
}

string_t string_from_cstr(const char *cstr) {
    return string_from_bytes(cstr, strlen(cstr));
}

void string_delete(string_t str) {
    free(str->buf);
    free(str);
}

uint8_t *string_data(string_t str) {
    return str->buf;
}

size_t string_len(string_t str) {
    return str->len;
}

bool string_is_empty(string_t str) {
    return str->len == 0;
}

bool string_equals(string_t str1, string_t str2) {
    return string_equals_bytes(str1, str2->buf, str2->len);
}

bool string_equals_bytes(string_t str, const void *data, size_t len) {
    return str->len == len && !memcmp(str->buf, data, len);
}

bool string_equals_cstr(string_t str, const char *cstr) {
    return string_equals_bytes(str, cstr, strlen(cstr));
}

void string_append(string_t str, string_t other) {
    return string_append_bytes(str, other->buf, other->len);
}

void string_append_bytes(string_t str, const void *data, size_t len) {
    expand(str, str->len + len);
    memcpy(&str->buf[str->len], data, len);
    str->len += len;
    str->buf[str->len] = '\0';
}

void string_append_cstr(string_t str, const char *cstr) {
    return string_append_bytes(str, cstr, strlen(cstr));
}

int string_findfrom(string_t str, string_t needle, int from) {
    return string_findfrom_bytes(str, needle->buf, needle->len, from);
}

int string_findfrom_bytes(string_t str, const void *data, size_t len, int from) {
    while (from + len <= str->len) {
        if (!memcmp(&str->buf[from], data, len)) {
            return from;
        }

        from++;
    }

    return -1;
}

int string_findfrom_cstr(string_t str, const char *cstr, int from) {
    return string_findfrom_bytes(str, cstr, strlen(cstr), from);
}

int string_find(string_t str, string_t needle) {
    return string_find_bytes(str, needle->buf, needle->len);
}

int string_find_bytes(string_t str, const void *data, size_t len) {
    return string_findfrom_bytes(str, data, len, 0);
}

int string_find_cstr(string_t str, const char *cstr) {
    return string_find_bytes(str, cstr, strlen(cstr));
}

bool string_contains(string_t str, string_t needle) {
    return string_find_bytes(str, needle->buf, needle->len) >= 0;
}

bool string_contains_bytes(string_t str, const void *data, size_t len) {
    return string_find_bytes(str, data, len) >= 0;
}

bool string_contains_cstr(string_t str, const char *cstr) {
    return string_find_bytes(str, cstr, strlen(cstr)) >= 0;
}
