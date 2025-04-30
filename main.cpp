#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main() {
    // GLFW başlat
    if (!glfwInit()) {
        std::cerr << "GLFW başlatılamadı!" << std::endl;
        return -1;
    }

    // OpenGL versiyonu ayarla
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Pencere oluştur
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Template", NULL, NULL);
    if (!window) {
        std::cerr << "Pencere oluşturulamadı!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLEW başlat
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW başlatılamadı!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Ana döngü
    while (!glfwWindowShouldClose(window)) {
        // Arkaplan temizleme
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Temizlik
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}