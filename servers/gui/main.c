#include "gui.h"
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
static backend_canvas_t *framebuffers = NULL;

backend_canvas_t get_back_buffer(void) {
    return framebuffers[front_buffer_index];
}

void swap_buffer(void) {
    struct message m;
    m.type = GPU_SWITCH_FRONT_BUFFER_MSG;
    m.gpu_switch_front_buffer.index = front_buffer_index;
    OOPS_OK(ipc_call(gpu_server, &m));

    front_buffer_index = (front_buffer_index + 1) % num_framebuffers;
}

static struct backend backend = {
    .get_back_buffer = get_back_buffer,
    .swap_buffer = swap_buffer,
};

static void init(void) {
    gpu_server = ipc_lookup("gpu");

    struct message m;
    m.type = GPU_SWITCH_FRONT_BUFFER_MSG;
    ASSERT_OK(ipc_call(gpu_server, &m));

    screen_height = m.gpu_set_default_mode_reply.height;
    screen_width = m.gpu_set_default_mode_reply.width;
    num_framebuffers = m.gpu_set_default_mode_reply.num_buffers;
    framebuffers =
        (backend_canvas_t *) malloc(sizeof(*framebuffers) * num_framebuffers);
    for (int i = 0; i < num_framebuffers; i++) {
        m.type = GPU_GET_BUFFER_MSG;
        ASSERT_OK(ipc_call(gpu_server, &m));

        framebuffers[i] = backend.create_framebuffer_canvas(
            screen_width, screen_height, m.gpu_get_buffer_reply.shm_handle);
    }

    gui_init(&backend);
    ASSERT_OK(ipc_serve("gui"));
}

void main(void) {
    init();
    TRACE("ready");

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case MOUSE_INPUT_MSG:
                break;
        }
    }
}
