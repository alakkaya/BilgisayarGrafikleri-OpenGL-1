#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

// Pencere boyutları
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Kamera ayarları
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

// Fare hareketi için değişkenler
float yaw = -90.0f; // Y eksenindeki açı
float pitch = 0.0f; // X eksenindeki açı
float lastX = SCR_WIDTH / 2.0f; // Son fare X pozisyonu
float lastY = SCR_HEIGHT / 2.0f; // Son fare Y pozisyonu
bool firstMouse = true; // İlk fare hareketi kontrolü

// Zaman değişkenleri
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Işık pozisyonu
glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window,int width, int height);
void processInput(GLFWwindow* window);
unsigned int loadShader(const char* vertexPath, const char* fragmentPath);

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