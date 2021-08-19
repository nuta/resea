#include "gui.h"
#include <assetfs.h>
#include <ipc_client/shm.h>
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
        case ICON_POINTER:
            name = "pointer.png";
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

        switch (m.type) {
            case MOUSE_INPUT_MSG:
                gui_move_mouse(m.mouse_input.x_delta, m.mouse_input.y_delta,
                               m.mouse_input.clicked_left,
                               m.mouse_input.clicked_right);
                break;
        }
    }
}
