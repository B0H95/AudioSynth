#include "graphics_area.hh"

#include <GL/glu.h>
#include <GL/gl.h>
#include <sstream>

namespace bzzt {

namespace {

auto vertex_shader_source {
    "#version 330 core\n"
    "layout (location=0) in vec2 position;\n"
    "layout (location=1) in vec4 color;\n"
    "layout (location=2) in vec2 texcoord;\n"
    "out vec4 out_color;\n"
    "out vec2 out_texcoord;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 0.0f, 1.0f);\n"
    "    out_color = color;\n"
    "    out_texcoord = texcoord;\n"
    "}\n"
};

auto fragment_shader_source {
    "#version 330 core\n"
    "in vec4 out_color;\n"
    "in vec2 out_texcoord;\n"
    "out vec4 output_color;\n"
    "uniform sampler2D font_texture;\n"
    "void main() {\n"
    "    output_color = texture(font_texture, out_texcoord) * out_color;\n"
    "}\n"
};

struct text_render_data {
    GLfloat x;
    GLfloat y;
    GLfloat u;
    GLfloat v;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
};

struct text_handle {
    unsigned int index;
    unsigned int length;
};

const GLfloat FONT_SIZE {8.0f / 1024.0f};

const unsigned int MAX_TEXT_RENDER_DATA_PER_LAYER {sizeof(text_render_data) * 6 * 2048};

}

// See font.cc
extern const unsigned int font_width;
extern const unsigned int font_height;
extern const unsigned char* const font_pixel_data;

struct graphics_area::impl {
    impl() : vertex_shader{}, fragment_shader{}, shader_program{}, rendered_chars{}, text_handles{}, text_handles_occupied{}, text_vbo{}, text_vao{}, font_texture{} {
        text_handles.reserve(8);
        text_handles_occupied.reserve(8);
    }

    GLint vertex_shader;
    GLint fragment_shader;
    GLint shader_program;

    unsigned int rendered_chars;
    std::vector<text_handle> text_handles;
    std::vector<bool> text_handles_occupied;
    GLuint text_vbo;
    GLuint text_vao;
    GLuint font_texture;
};

bool graphics_area::init(message_box& msg_box) {
    internal = new impl{};
    if (!internal) {
        msg_box.push_error("Could not allocate memory for the graphics area");
        return false;
    }

    auto init_success {true};
    const auto ERROR_LENGTH {512};
    GLchar error_buffer[ERROR_LENGTH];
    GLint success;
    std::stringstream ss;

    glewInit();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    internal->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(internal->vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(internal->vertex_shader);
    glGetShaderiv(internal->vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(internal->vertex_shader, ERROR_LENGTH, nullptr, error_buffer);
        ss << "Vertex shader compilation error: " << error_buffer;
        msg_box.push_error(ss.str());
        init_success = false;
        ss.str("");
    }

    internal->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(internal->fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(internal->fragment_shader);
    glGetShaderiv(internal->fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(internal->fragment_shader, ERROR_LENGTH, nullptr, error_buffer);
        ss << "Fragment shader compilation error: " << error_buffer;
        msg_box.push_error(ss.str());
        init_success = false;
        ss.str("");
    }

    internal->shader_program = glCreateProgram();
    glAttachShader(internal->shader_program, internal->vertex_shader);
    glAttachShader(internal->shader_program, internal->fragment_shader);
    glLinkProgram(internal->shader_program);
    glGetProgramiv(internal->shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(internal->shader_program, ERROR_LENGTH, nullptr, error_buffer);
        ss << "Program linking error: " << error_buffer;
        msg_box.push_error(ss.str());
        init_success = false;
        ss.str("");
    }

    glGenVertexArrays(1, &internal->text_vao);
    glBindVertexArray(internal->text_vao);

    glGenBuffers(1, &internal->text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, internal->text_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_TEXT_RENDER_DATA_PER_LAYER, nullptr, GL_STATIC_DRAW);

    GLsizei stride {4 * sizeof(GLfloat) + 4 * sizeof(GLubyte)};

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (GLvoid*)(4 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(2 * sizeof(GLfloat)));

    glGenTextures(1, &internal->font_texture);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, internal->font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font_width, font_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_pixel_data);

    glUniform1i(glGetUniformLocation(internal->shader_program, "font_texture"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return init_success;
}

void graphics_area::deinit() {
    if (internal) {
        glDeleteTextures(1, &internal->font_texture);

        glDetachShader(internal->shader_program, internal->fragment_shader);
        glDeleteShader(internal->fragment_shader);
        glDetachShader(internal->shader_program, internal->vertex_shader);
        glDeleteShader(internal->vertex_shader);
        glDeleteProgram(internal->shader_program);

        glDeleteBuffers(1, &internal->text_vbo);
        glDeleteVertexArrays(1, &internal->text_vao);

        delete internal;
    }
}

void graphics_area::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(internal->shader_program);
    glBindVertexArray(internal->text_vao);
    glDrawArrays(GL_TRIANGLES, 0, internal->rendered_chars * 6);
    glBindVertexArray(0);
}

graphics_area::graphics_handle graphics_area::add_text(const char* text, unsigned int length, float x, float y, float char_width, float char_height) {
    auto initial_size {internal->rendered_chars};

    glBindVertexArray(internal->text_vao);
    auto render_data {static_cast<text_render_data*>(glMapNamedBuffer(internal->text_vbo, GL_WRITE_ONLY))};

    unsigned int vertex_count {initial_size * 6};
    for (unsigned int i {0}; i < length; ++i) {
        render_data[vertex_count++] = {x, y, 0.0f, FONT_SIZE * static_cast<float>(text[i]), 255, 255, 255, 255};
        render_data[vertex_count++] = {x + char_width, y, 1.0f, FONT_SIZE * static_cast<float>(text[i]), 255, 255, 255, 255};
        render_data[vertex_count++] = {x + char_width, y - char_height, 1.0f, FONT_SIZE * static_cast<float>(text[i] + 1), 255, 255, 255, 255};
        render_data[vertex_count++] = {x + char_width, y - char_height, 1.0f, FONT_SIZE * static_cast<float>(text[i] + 1), 255, 255, 255, 255};
        render_data[vertex_count++] = {x, y - char_height, 0.0f, FONT_SIZE * static_cast<float>(text[i] + 1), 255, 255, 255, 255};
        render_data[vertex_count++] = {x, y, 0.0f, FONT_SIZE * static_cast<float>(text[i]), 255, 255, 255, 255};
        x += char_width;
    }

    glUnmapNamedBuffer(internal->text_vbo);

    internal->rendered_chars += length;

    for (unsigned int i {0}; i < internal->text_handles_occupied.size(); ++i) {
        auto occupied {internal->text_handles_occupied[i]};
        auto& text_handle {internal->text_handles[i]};
        if (!occupied) {
            internal->text_handles_occupied[i] = true;
            text_handle.index = initial_size;
            text_handle.length = length;
            return i;
        }
    }

    internal->text_handles.push_back({initial_size, length});
    internal->text_handles_occupied.push_back(true);

    return internal->text_handles.size() - 1;
}

void graphics_area::delete_text(graphics_area::graphics_handle handle) {
    glBindVertexArray(internal->text_vao);

    auto render_data {static_cast<text_render_data*>(glMapNamedBuffer(internal->text_vbo, GL_READ_WRITE))};
    auto const& text_handle {internal->text_handles[handle]};

    unsigned int chars_to_move {internal->rendered_chars - text_handle.index - text_handle.length};
    for (unsigned int i {text_handle.index * 6}; i < (text_handle.index + chars_to_move) * 6; ++i) {
        render_data[i] = render_data[i + text_handle.length * 6];
    }

    for (auto& h : internal->text_handles) {
        if (h.index > text_handle.index) {
            h.index -= text_handle.length;
        }
    }

    glUnmapNamedBuffer(internal->text_vbo);

    internal->text_handles_occupied[handle] = false;
    internal->rendered_chars -= text_handle.length;
}

}