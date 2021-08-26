#ifndef __RENDERER_H__
#define __RENDERER_H__

// -----------------------------------------------------------------------------
// FIXME: include <list.h> instead
#ifndef __LIST_H__
#    define LIST_CONTAINER(head, container, field)                             \
        ((container *) ((vaddr_t) (head) -offsetof(container, field)))
#    define LIST_FOR_EACH(elem, list, container, field)                        \
        for (container *elem = LIST_CONTAINER((list)->next, container, field), \
                       *__next = NULL;                                         \
             (&elem->field != (list)                                           \
              && (__next =                                                     \
                      LIST_CONTAINER(elem->field.next, container, field)));    \
             elem = __next)

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

typedef struct list_head list_t;
typedef struct list_head list_elem_t;

// Inserts a new element between `prev` and `next`.
static inline void list_insert(list_elem_t *prev, list_elem_t *next,
                               list_elem_t *new) {
    new->prev = prev;
    new->next = next;
    next->prev = new;
    prev->next = new;
}

// Initializes a list.
static inline void list_init(list_t *list) {
    list->prev = list;
    list->next = list;
}

// Invalidates a list element.
static inline void list_nullify(list_elem_t *elem) {
    elem->prev = NULL;
    elem->next = NULL;
}

// Removes a element from the list.
static inline void list_remove(list_elem_t *elem) {
    if (!elem->next) {
        // The element is not in a list.
        return;
    }

    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    // Invalidate the element as they're no longer in the list.
    list_nullify(elem);
}

// Appends a element into the list.
static inline void list_push_back(list_t *list, list_elem_t *new_tail) {
    list_insert(list->prev, list, new_tail);
}
#endif
// -----------------------------------------------------------------------------

// Represents a color in RGBA.
typedef uint32_t color_t;

struct canvas;
typedef struct canvas *canvas_t;

enum canvas_format {
    CANVAS_FORMAT_ARGB32,
};

enum cursor_shape {
    CURSOR_POINTER,
};

enum icon_type {
    ICON_CURSOR = 0,
    NUM_ICON_TYPES = 1,
};

struct surface_ops;
struct surface {
    list_elem_t next;
    struct surface_ops *ops;
    void *user_data;
    canvas_t canvas;
    int screen_x;
    int screen_y;
    int width;
    int height;
};

struct surface_ops {
    /// Called when the surface need to render the contents into its canvas.
    void (*render)(struct surface *surface);
    /// Called when the cursor is moved. `screen_x` and `screen_y` are global
    /// cursor position.
    void (*global_mouse_move)(struct surface *surface, int screen_x,
                              int screen_y);
    /// Called when the left button is up.`screen_x` and `screen_y` are global
    /// cursor position. If the callback returns `true`, the event propagation
    /// stops.
    bool (*global_mouse_up)(struct surface *surface, int screen_x,
                            int screen_y);
    /// Called on a left button is down. `x` and `y` are surface-local cursor
    /// position. If the callback returns `true`, the event propagation stops.
    bool (*mouse_down)(struct surface *surface, int x, int y);
};

enum widget_type {
    WIDGET_TEXT,
    WIDGET_BUTTON,
};

struct widget_header {
    enum widget_type type;
    struct widget_header *next;
    int x;
    int y;
};

struct text_widget {
    struct widget_header header;
    const char *body;
    color_t color;
};

struct button_widget {
    struct widget_header header;
    struct text_widget *text;
};

#define WINDOW_TITLE_HEIGHT 23
struct window_data {
    bool being_moved;
    int prev_cursor_x;
    int prev_cursor_y;
    struct surface *surface;
};

struct cursor_data {
    enum cursor_shape shape;
};

struct wallpaper_data {};

struct widgets_data {
    list_t widgets;
};

canvas_t canvas_create(int width, int height);
canvas_t canvas_create_from_buffer(int screen_width, int screen_height,
                                   void *framebuffer,
                                   enum canvas_format format);
void canvas_draw_wallpaper(canvas_t canvas);
void canvas_draw_window(canvas_t canvas, struct window_data *window);
void canvas_draw_cursor(canvas_t canvas, enum cursor_shape shape);
void canvas_copy(canvas_t dst, canvas_t src, int x, int y);
void canvas_init(void *(*get_icon_png)(enum icon_type icon,
                                       unsigned *file_size),
                 void *(*open_asset_file)(const char *name,
                                          unsigned *file_size));

#endif
