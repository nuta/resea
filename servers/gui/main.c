#include "gui.h"
#include <assetfs.h>
#include <ipc_client/shm.h>
#include <resea/async.h>
#include <resea/handle.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/timer.h>
#include <string.h>

static task_t gpu_server;
static int front_buffer_index = 0;
static unsigned screen_height;
static unsigned screen_width;
static size_t num_framebuffers;
static struct canvas **framebuffers = NULL;

static struct assetfs assets;
extern char _binary_build_servers_gui_assets_o_data_start[];
extern char _binary_build_servers_gui_assets_o_data_size[];

void *open_asset_file(const char *name, unsigned *file_size) {
    struct assetfs_file *file = assetfs_open_file(&assets, name);
    if (!file) {
        PANIC("asset not found \"%s\"", name);
    }

    *file_size = file->size;
    return assetfs_file_data(&assets, file);
}

void *get_icon_png(enum icon_type icon, unsigned *file_size) {
    char *name;
    switch (icon) {
        case ICON_CURSOR:
            name = "cursor.png";
            break;
        default:
            UNREACHABLE();
    }

    return open_asset_file(name, file_size);
}

canvas_t get_back_buffer(void) {
    return framebuffers[front_buffer_index];
}

void *shm_framebuffer;
void swap_buffer(void) {
    struct message m;
    m.type = GPU_SWITCH_FRONT_BUFFER_MSG;
    m.gpu_switch_front_buffer.index = front_buffer_index;
    OOPS_OK(ipc_call(gpu_server, &m));

    front_buffer_index = (front_buffer_index + 1) % num_framebuffers;
}

static struct os_ops os_ops = {
    .get_back_buffer = get_back_buffer,
    .swap_buffer = swap_buffer,
    .get_icon_png = get_icon_png,
    .open_asset_file = open_asset_file,
};

static void init(void) {
    gpu_server = ipc_lookup("gpu");

    struct message m;
    m.type = GPU_SET_DEFAULT_MODE_MSG;
    ASSERT_OK(ipc_call(gpu_server, &m));

    screen_height = m.gpu_set_default_mode_reply.height;
    screen_width = m.gpu_set_default_mode_reply.width;
    num_framebuffers = m.gpu_set_default_mode_reply.num_buffers;
    framebuffers =
        (struct canvas **) malloc(sizeof(*framebuffers) * num_framebuffers);
    for (size_t i = 0; i < num_framebuffers; i++) {
        m.type = GPU_GET_BUFFER_MSG;
        ASSERT_OK(ipc_call(gpu_server, &m));
        handle_t shm_handle = m.gpu_get_buffer_reply.shm_handle;

        void *framebuffer;
        ASSERT_OK(shm_map(shm_handle, true, &framebuffer));
        shm_framebuffer = framebuffer;
        framebuffers[i] = canvas_create_from_buffer(
            screen_width, screen_height, framebuffer, CANVAS_FORMAT_ARGB32);
    }

    assetfs_open(&assets, _binary_build_servers_gui_assets_o_data_start,
                 (size_t) _binary_build_servers_gui_assets_o_data_size);

    gui_init(screen_width, screen_height, &os_ops);
    ASSERT_OK(ipc_serve("gui"));
}

static struct surface *handle_to_surface(task_t client, handle_t handle,
                                         enum surface_type type) {
    struct gui_object *header = handle_get(client, handle);
    if (!header) {
        WARN_DBG("unallocated handle %d", handle);
        return NULL;
    }

    if (header->type != GUI_OBJECT_SURFACE) {
        WARN_DBG("handle %d is not a surface", handle);
        return NULL;
    }

    struct surface *surface = (struct surface *) header;
    if (surface->type != type) {
        WARN_DBG("handle %d is an unexpected surface type", handle);
        return NULL;
    }

    return surface;
}

static struct user_ctx *create_user_ctx(task_t client, handle_t handle) {
    struct user_ctx *ctx = malloc(sizeof(*ctx));
    ctx->client = client;
    ctx->handle = handle;
    return ctx;
}

static void handle_message(struct message m) {
    task_t client = m.src;
    switch (m.type) {
        case ASYNC_MSG:
            async_reply(m.src);
            break;
        case GUI_WINDOW_CREATE_MSG: {
            handle_t handle = handle_alloc(client);
            struct surface *window = window_create(
                m.gui_window_create.title, m.gui_window_create.width,
                m.gui_window_create.height, create_user_ctx(client, handle));
            free(m.gui_window_create.title);

            handle_set(client, handle, window);

            bzero(&m, sizeof(m));
            m.type = GUI_WINDOW_CREATE_REPLY_MSG;
            m.gui_window_create_reply.window = handle;
            ipc_reply(client, &m);
            break;
        }
        case GUI_BUTTON_CREATE_MSG: {
            struct surface *window = handle_to_surface(
                client, m.gui_button_create.window, SURFACE_WINDOW);
            if (!window) {
                ipc_reply_err(client, ERR_INVALID_ARG);
                break;
            }

            handle_t handle = handle_alloc(client);
            struct surface *button = button_create(
                window, m.gui_button_create.x, m.gui_button_create.y,
                m.gui_button_create.label, create_user_ctx(client, handle));
            free(m.gui_button_create.label);
            if (!button) {
                ipc_reply_err(client, ERR_INVALID_ARG);
                break;
            }

            handle_set(client, handle, button);

            bzero(&m, sizeof(m));
            m.type = GUI_BUTTON_CREATE_REPLY_MSG;
            m.gui_button_create_reply.button = handle;
            ipc_reply(client, &m);
            break;
        }
        case GUI_BUTTON_SET_LABEL_MSG: {
            struct surface *button = handle_to_surface(
                client, m.gui_button_set_label.button, SURFACE_BUTTON);
            if (!button) {
                ipc_reply_err(client, ERR_INVALID_ARG);
                break;
            }

            error_t err =
                button_set_label(button, m.gui_button_set_label.label);
            free(m.gui_button_set_label.label);
            if (err != OK) {
                ipc_reply_err(client, err);
                break;
            }

            bzero(&m, sizeof(m));
            m.type = GUI_BUTTON_SET_LABEL_REPLY_MSG;
            ipc_reply(client, &m);
            break;
        }
        case MOUSE_INPUT_MSG:
            gui_move_mouse(m.mouse_input.x_delta, m.mouse_input.y_delta,
                           m.mouse_input.clicked_left,
                           m.mouse_input.clicked_right);
            break;
        default:
            discard_unknown_message(&m);
    }
}

void main(void) {
    init();

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        gui_render();

        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);
        handle_message(m);
    }
}
