#include "window.hh"

#include <GLFW/glfw3.h>

namespace bzzt {

namespace {

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    window* w = static_cast<window*>(glfwGetWindowUserPointer(win));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        w->close();
    }
}

void window_resize_callback(GLFWwindow* win, int width, int height) {
    (void) win;
    (void) width;
    (void) height;
    //window* w = static_cast<window*>(glfwGetWindowUserPointer(win));
}

}

struct window::impl {
    GLFWwindow* win;
    bool running;
    unsigned int width;
    unsigned int height;
};

window::window(unsigned int width, unsigned int height) : internal{new window::impl} {
    internal->win = nullptr;
    internal->running = true;
    internal->width = width;
    internal->height = height;
    if (!glfwInit()) {
        throw window_exception {"glfwInit failed"};
    }
    internal->win = glfwCreateWindow(internal->width, internal->height, "Window title", NULL, NULL);
    if (!internal->win) {
        glfwTerminate();
        throw window_exception {"glfwCreateWindow failed"};
    }
    glfwMakeContextCurrent(internal->win);
    glfwSwapInterval(0);
    glfwSetWindowUserPointer(internal->win, static_cast<void*>(this));
    glfwSetKeyCallback(internal->win, key_callback);
    glfwSetWindowSizeCallback(internal->win, window_resize_callback);
}

window::~window() {
    if (internal->win) {
        glfwDestroyWindow(internal->win);
    }
    glfwTerminate();
    delete internal;
}

void window::update_screen() {
    glfwSwapBuffers(internal->win);
}

bool window::should_close() const {
    return glfwWindowShouldClose(internal->win) || !internal->running;
}

void window::handle_incoming_events() {
    //glfwPollEvents(); // If you do not want to wait for events.
    glfwWaitEvents(); // If you want to wait for events.
}

/*void window::set_window_properties(common::window_config const& window_config) {
    glfwSetWindowSize(win,
        static_cast<int>(window_config.width),
        static_cast<int>(window_config.height));
}*/

void window::close() {
    internal->running = false;
}

}