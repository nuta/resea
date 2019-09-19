#include <base/string.h>
#include <base/malloc.h>

string_t string_new(void) {
    string_t str = malloc(sizeof(struct string));
    str->data = malloc(STRING_INITIAL_CAPACITY);
    str->len = 0;
    str->capacity = STRING_INITIAL_CAPACITY;
    return str;
}

void string_delete(string_t str) {
    free(str->data);
}

string_t string_from(const char *cstr) {
    string_t str = string_new();
    string_append_cstr(str, cstr);
    return str;
}

int string_len(string_t str) {
    return str->len;
}

const char *string_cstr(string_t str) {
    return str->data;
}

char string_get_char(string_t str, int index) {
    assert(index < str->len);
    return str->data[index];
}

void string_set_char(string_t str, int index, char ch) {
    assert(index < str->len);
    str->data[index] = ch;
}

void string_resize(string_t str, int new_capacity) {
    assert(new_capacity > str->capacity);
    char *old_data = str->data;
    char *new_data = malloc(new_capacity);
    memcpy(new_data, old_data, str->len + 1 /* Including '\0'. */);
    str->data = new_data;
    str->capacity = new_capacity;
    free(old_data);
}

void string_append(string_t str, string_t other) {
    string_append_cstr(str, other->data);
}

void string_append_cstr(string_t str, const char *cstr) {
    int cstr_len = strlen(cstr);

    if (str->capacity < str->len + cstr_len + 1) {
        string_resize(str, (str->len + cstr_len + 1) * 2);
    }

    for (int i = 0; cstr[i] != '\0'; i++) {
        str->data[str->len] = cstr[i];
        str->len++;
    }

    str->data[str->len] = '\0';
}

int string_find(string_t haystack, string_t needle) {
    return string_find_cstr(haystack, needle->data);
}

int string_find_cstr(string_t haystack, const char *needle) {
    const char *cstr = haystack->data;
    for (int start = 0; cstr[start] != '\0'; start++) {
        bool matches = true;
        for (int i = 0; cstr[start + i] != '\0' && needle[i] != '\0'; i++) {
            if (cstr[start + i] != needle[i]) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return start;
        }
    }

    return -1;
}

bool string_startswith(string_t str, string_t prefix) {
    return string_startswith_cstr(str, prefix->data);
}

bool string_startswith_cstr(string_t str, const char *prefix) {
    return string_find_cstr(str, prefix) == 0;
}

