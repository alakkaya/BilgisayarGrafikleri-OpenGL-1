#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>

// Pencere boyutları
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Kamera ayarları
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Fare kontrol değişkenleri
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Işık pozisyonu
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// Zaman değişkenleri
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Fonksiyon prototipleri
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadShader(const char* vertexPath, const char* fragmentPath);
unsigned int createCube();
std::vector<float> createSphereVertices(float radius, int sectorCount, int stackCount, glm::vec3 color);
std::vector<unsigned int> createSphereIndices(int sectorCount, int stackCount);
unsigned int createSphere(float radius, int sectorCount, int stackCount, glm::vec3 color);

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
      
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
        
    vec3 result = (ambient + diffuse + specular) * Color;
    FragColor = vec4(result, 1.0);
}
)";

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
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Pencere oluştur
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Çalışma Masası Modeli", NULL, NULL);
    if (!window) {
        std::cerr << "Pencere oluşturulamadı!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Fare yakalama modu
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW başlat
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW başlatılamadı!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Derinlik testi etkinleştir
    glEnable(GL_DEPTH_TEST);

    // Shader programı oluştur
    unsigned int shaderProgram = glCreateProgram();
    
    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Vertex shader derleme kontrolü
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Fragment shader derleme kontrolü
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Shader programını link et
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Shader programı link kontrolü
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Shader objeleri artık gerekli değil
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Çalışma masası objelerini oluştur
    // Masaüstü (tahta) - kahverengi
    float deskVertices[] = {
        // pozisyonlar          // normallar         // renkler (kahverengi)
        -1.5f, -0.1f, -0.8f,    0.0f, 1.0f, 0.0f,    0.55f, 0.27f, 0.07f,
         1.5f, -0.1f, -0.8f,    0.0f, 1.0f, 0.0f,    0.55f, 0.27f, 0.07f,
         1.5f, -0.1f,  0.8f,    0.0f, 1.0f, 0.0f,    0.55f, 0.27f, 0.07f,
         1.5f, -0.1f,  0.8f,    0.0f, 1.0f, 0.0f,    0.55f, 0.27f, 0.07f,
        -1.5f, -0.1f,  0.8f,    0.0f, 1.0f, 0.0f,    0.55f, 0.27f, 0.07f,
        -1.5f, -0.1f, -0.8f,    0.0f, 1.0f, 0.0f,    0.55f, 0.27f, 0.07f,

        // Masa alt yüzü
        -1.5f, -0.15f, -0.8f,   0.0f, -1.0f, 0.0f,   0.45f, 0.20f, 0.05f,
         1.5f, -0.15f, -0.8f,   0.0f, -1.0f, 0.0f,   0.45f, 0.20f, 0.05f,
         1.5f, -0.15f,  0.8f,   0.0f, -1.0f, 0.0f,   0.45f, 0.20f, 0.05f,
         1.5f, -0.15f,  0.8f,   0.0f, -1.0f, 0.0f,   0.45f, 0.20f, 0.05f,
        -1.5f, -0.15f,  0.8f,   0.0f, -1.0f, 0.0f,   0.45f, 0.20f, 0.05f,
        -1.5f, -0.15f, -0.8f,   0.0f, -1.0f, 0.0f,   0.45f, 0.20f, 0.05f,

        // Masa kenarları
        -1.5f, -0.15f, -0.8f,   0.0f, 0.0f, -1.0f,   0.50f, 0.25f, 0.06f,
         1.5f, -0.15f, -0.8f,   0.0f, 0.0f, -1.0f,   0.50f, 0.25f, 0.06f,
         1.5f, -0.1f, -0.8f,    0.0f, 0.0f, -1.0f,   0.50f, 0.25f, 0.06f,
         1.5f, -0.1f, -0.8f,    0.0f, 0.0f, -1.0f,   0.50f, 0.25f, 0.06f,
        -1.5f, -0.1f, -0.8f,    0.0f, 0.0f, -1.0f,   0.50f, 0.25f, 0.06f,
        -1.5f, -0.15f, -0.8f,   0.0f, 0.0f, -1.0f,   0.50f, 0.25f, 0.06f,

         1.5f, -0.15f, -0.8f,   1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.15f,  0.8f,   1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.1f,  0.8f,    1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.1f,  0.8f,    1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.1f, -0.8f,    1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.15f, -0.8f,   1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,

        -1.5f, -0.15f,  0.8f,   0.0f, 0.0f, 1.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.15f,  0.8f,   0.0f, 0.0f, 1.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.1f,  0.8f,    0.0f, 0.0f, 1.0f,    0.50f, 0.25f, 0.06f,
         1.5f, -0.1f,  0.8f,    0.0f, 0.0f, 1.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.1f,  0.8f,    0.0f, 0.0f, 1.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.15f,  0.8f,   0.0f, 0.0f, 1.0f,    0.50f, 0.25f, 0.06f,

        -1.5f, -0.15f, -0.8f,  -1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.15f,  0.8f,  -1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.1f,  0.8f,   -1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.1f,  0.8f,   -1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.1f, -0.8f,   -1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
        -1.5f, -0.15f, -0.8f,  -1.0f, 0.0f, 0.0f,    0.50f, 0.25f, 0.06f,
    };

    // Masa bacakları (4 tane)
    float legVertices[] = {
        // positions          // normals           // colors (dark brown)
        -0.05f, -0.8f, -0.05f,  0.0f, 0.0f, -1.0f, 0.35f, 0.18f, 0.04f,
         0.05f, -0.8f, -0.05f,  0.0f, 0.0f, -1.0f, 0.35f, 0.18f, 0.04f,
         0.05f,  0.0f, -0.05f,  0.0f, 0.0f, -1.0f, 0.35f, 0.18f, 0.04f,
         0.05f,  0.0f, -0.05f,  0.0f, 0.0f, -1.0f, 0.35f, 0.18f, 0.04f,
        -0.05f,  0.0f, -0.05f,  0.0f, 0.0f, -1.0f, 0.35f, 0.18f, 0.04f,
        -0.05f, -0.8f, -0.05f,  0.0f, 0.0f, -1.0f, 0.35f, 0.18f, 0.04f,

        -0.05f, -0.8f,  0.05f,  0.0f, 0.0f, 1.0f,  0.35f, 0.18f, 0.04f,
         0.05f, -0.8f,  0.05f,  0.0f, 0.0f, 1.0f,  0.35f, 0.18f, 0.04f,
         0.05f,  0.0f,  0.05f,  0.0f, 0.0f, 1.0f,  0.35f, 0.18f, 0.04f,
         0.05f,  0.0f,  0.05f,  0.0f, 0.0f, 1.0f,  0.35f, 0.18f, 0.04f,
        -0.05f,  0.0f,  0.05f,  0.0f, 0.0f, 1.0f,  0.35f, 0.18f, 0.04f,
        -0.05f, -0.8f,  0.05f,  0.0f, 0.0f, 1.0f,  0.35f, 0.18f, 0.04f,

        -0.05f,  0.0f,  0.05f, -1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
        -0.05f,  0.0f, -0.05f, -1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
        -0.05f, -0.8f, -0.05f, -1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
        -0.05f, -0.8f, -0.05f, -1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
        -0.05f, -0.8f,  0.05f, -1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
        -0.05f,  0.0f,  0.05f, -1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,

         0.05f,  0.0f,  0.05f,  1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
         0.05f,  0.0f, -0.05f,  1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
         0.05f, -0.8f, -0.05f,  1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
         0.05f, -0.8f, -0.05f,  1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
         0.05f, -0.8f,  0.05f,  1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
         0.05f,  0.0f,  0.05f,  1.0f, 0.0f, 0.0f,  0.35f, 0.18f, 0.04f,
    };

    // Monitör için vertex verileri
    float monitorVertices[] = {
        // pozisyonlar          // normallar         // renkler (siyah)
        // Ana ekran yüzeyi (ekran kısmı - koyu siyah)
        -0.3f, 0.0f, -0.02f,    0.0f, 0.0f, 1.0f,    0.05f, 0.05f, 0.05f,
         0.3f, 0.0f, -0.02f,    0.0f, 0.0f, 1.0f,    0.05f, 0.05f, 0.05f,
         0.3f, 0.4f, -0.02f,    0.0f, 0.0f, 1.0f,    0.05f, 0.05f, 0.05f,
         0.3f, 0.4f, -0.02f,    0.0f, 0.0f, 1.0f,    0.05f, 0.05f, 0.05f,
        -0.3f, 0.4f, -0.02f,    0.0f, 0.0f, 1.0f,    0.05f, 0.05f, 0.05f,
        -0.3f, 0.0f, -0.02f,    0.0f, 0.0f, 1.0f,    0.05f, 0.05f, 0.05f,

        // Monitör çerçevesi (gri)
        -0.32f, -0.02f, -0.03f, 0.0f, 0.0f, 1.0f,    0.3f, 0.3f, 0.3f,
         0.32f, -0.02f, -0.03f, 0.0f, 0.0f, 1.0f,    0.3f, 0.3f, 0.3f,
         0.32f, 0.42f, -0.03f,  0.0f, 0.0f, 1.0f,    0.3f, 0.3f, 0.3f,
         0.32f, 0.42f, -0.03f,  0.0f, 0.0f, 1.0f,    0.3f, 0.3f, 0.3f,
        -0.32f, 0.42f, -0.03f,  0.0f, 0.0f, 1.0f,    0.3f, 0.3f, 0.3f,
        -0.32f, -0.02f, -0.03f, 0.0f, 0.0f, 1.0f,    0.3f, 0.3f, 0.3f,

        // Monitör standı (koyu gri)
        -0.05f, -0.02f, -0.03f, 0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
         0.05f, -0.02f, -0.03f, 0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
         0.05f, -0.1f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
         0.05f, -0.1f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
        -0.05f, -0.1f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
        -0.05f, -0.02f, -0.03f, 0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,

        // Monitör taban (koyu gri) - Y koordinatlarını artırarak tabanı yukarı kaldır
-0.15f, -0.09f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,  // Eski: -0.15f, -0.1f, -0.03f
0.15f, -0.09f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,  // Eski: 0.15f, -0.1f, -0.03f
0.15f, -0.09f, 0.1f,    0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,  // Eski: 0.15f, -0.1f, 0.1f
0.15f, -0.09f, 0.1f,    0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,  // Eski: 0.15f, -0.1f, 0.1f
-0.15f, -0.09f, 0.1f,    0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,  // Eski: -0.15f, -0.1f, 0.1f
-0.15f, -0.09f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,  // Eski: -0.15f, -0.1f, -0.03f
    };

    // Mouse için dikdörtgen küp (3D blok)
    float mouseVertices[] = {
        // Top face
        -0.03f, -0.07f, -0.03f,  0.0f, 1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.03f,  0.0f, 1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.08f,  0.0f, 1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.08f,  0.0f, 1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.08f,  0.0f, 1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.03f,  0.0f, 1.0f, 0.0f,   0.15f, 0.15f, 0.15f,

        // Bottom face
        -0.03f, -0.11f, -0.03f,  0.0f,-1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.03f,  0.0f,-1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.08f,  0.0f,-1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.08f,  0.0f,-1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.11f, -0.08f,  0.0f,-1.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.11f, -0.03f,  0.0f,-1.0f, 0.0f,   0.15f, 0.15f, 0.15f,

        // Front face
        -0.03f, -0.11f, -0.03f,  0.0f, 0.0f, 1.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.03f,  0.0f, 0.0f, 1.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.03f,  0.0f, 0.0f, 1.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.03f,  0.0f, 0.0f, 1.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.03f,  0.0f, 0.0f, 1.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.11f, -0.03f,  0.0f, 0.0f, 1.0f,   0.15f, 0.15f, 0.15f,

        // Back face
        -0.03f, -0.11f, -0.08f,  0.0f, 0.0f,-1.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.08f,  0.0f, 0.0f,-1.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.08f,  0.0f, 0.0f,-1.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.08f,  0.0f, 0.0f,-1.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.08f,  0.0f, 0.0f,-1.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.11f, -0.08f,  0.0f, 0.0f,-1.0f,   0.15f, 0.15f, 0.15f,

        // Left face
        -0.03f, -0.11f, -0.03f, -1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.11f, -0.08f, -1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.08f, -1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.08f, -1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.07f, -0.03f, -1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
        -0.03f, -0.11f, -0.03f, -1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,

        // Right face
         0.03f, -0.11f, -0.03f,  1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.08f,  1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.08f,  1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.08f,  1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.07f, -0.03f,  1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f,
         0.03f, -0.11f, -0.03f,  1.0f, 0.0f, 0.0f,   0.15f, 0.15f, 0.15f
    };

    // Klavye için vertex verileri
    float keyboardVertices[] = {
        // Ana klavye gövdesi (siyah)
        // pozisyonlar          // normallar         // renkler (siyah)
        // Üst yüzey
        -0.25f, -0.09f, -0.15f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
         0.25f, -0.09f, -0.15f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
         0.25f, -0.09f, -0.35f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
         0.25f, -0.09f, -0.35f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
        -0.25f, -0.09f, -0.35f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
        -0.25f, -0.09f, -0.15f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,

        // Alt yüzey
        -0.25f, -0.11f, -0.15f, 0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
         0.25f, -0.11f, -0.15f, 0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
         0.25f, -0.11f, -0.35f, 0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
         0.25f, -0.11f, -0.35f, 0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
        -0.25f, -0.11f, -0.35f, 0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
        -0.25f, -0.11f, -0.15f, 0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,

        // Ön yüzey
        -0.25f, -0.11f, -0.15f, 0.0f, 0.0f, 1.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.11f, -0.15f, 0.0f, 0.0f, 1.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.15f, 0.0f, 0.0f, 1.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.15f, 0.0f, 0.0f, 1.0f,    0.12f, 0.12f, 0.12f,
        -0.25f, -0.09f, -0.15f, 0.0f, 0.0f, 1.0f,    0.12f, 0.12f, 0.12f,
        -0.25f, -0.11f, -0.15f, 0.0f, 0.0f, 1.0f,    0.12f, 0.12f, 0.12f,

        // Arka yüzey
        -0.25f, -0.11f, -0.35f, 0.0f, 0.0f, -1.0f,   0.12f, 0.12f, 0.12f,
         0.25f, -0.11f, -0.35f, 0.0f, 0.0f, -1.0f,   0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.35f, 0.0f, 0.0f, -1.0f,   0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.35f, 0.0f, 0.0f, -1.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.09f, -0.35f, 0.0f, 0.0f, -1.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.11f, -0.35f, 0.0f, 0.0f, -1.0f,   0.12f, 0.12f, 0.12f,

        // Sol yüzey
        -0.25f, -0.11f, -0.35f, -1.0f, 0.0f, 0.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.11f, -0.15f, -1.0f, 0.0f, 0.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.09f, -0.15f, -1.0f, 0.0f, 0.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.09f, -0.15f, -1.0f, 0.0f, 0.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.09f, -0.35f, -1.0f, 0.0f, 0.0f,   0.12f, 0.12f, 0.12f,
        -0.25f, -0.11f, -0.35f, -1.0f, 0.0f, 0.0f,   0.12f, 0.12f, 0.12f,

        // Sağ yüzey
         0.25f, -0.11f, -0.35f, 1.0f, 0.0f, 0.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.11f, -0.15f, 1.0f, 0.0f, 0.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.15f, 1.0f, 0.0f, 0.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.15f, 1.0f, 0.0f, 0.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.09f, -0.35f, 1.0f, 0.0f, 0.0f,    0.12f, 0.12f, 0.12f,
         0.25f, -0.11f, -0.35f, 1.0f, 0.0f, 0.0f,    0.12f, 0.12f, 0.12f,
    };

    // Kitaplar (üst üste 3 kitap)
    float bookVertices[] = {
        // Birinci kitap (kırmızı kitap)
        // pozisyonlar          // normallar         // renkler (kırmızı)
        // Üst yüzey
        -1.3f, -0.09f, 0.6f,    0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.6f,    0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.3f,    0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.3f,    0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.3f,    0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.6f,    0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,

        // Alt yüzey
        -1.3f, -0.12f, 0.6f,    0.0f, -1.0f, 0.0f,   0.7f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.6f,    0.0f, -1.0f, 0.0f,   0.7f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.3f,    0.0f, -1.0f, 0.0f,   0.7f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.3f,    0.0f, -1.0f, 0.0f,   0.7f, 0.1f, 0.1f,
        -1.3f, -0.12f, 0.3f,    0.0f, -1.0f, 0.0f,   0.7f, 0.1f, 0.1f,
        -1.3f, -0.12f, 0.6f,    0.0f, -1.0f, 0.0f,   0.7f, 0.1f, 0.1f,

        // Ön yüzey
        -1.3f, -0.12f, 0.6f,    0.0f, 0.0f, 1.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.6f,    0.0f, 0.0f, 1.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.6f,    0.0f, 0.0f, 1.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.6f,    0.0f, 0.0f, 1.0f,    0.75f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.6f,    0.0f, 0.0f, 1.0f,    0.75f, 0.1f, 0.1f,
        -1.3f, -0.12f, 0.6f,    0.0f, 0.0f, 1.0f,    0.75f, 0.1f, 0.1f,

        // Arka yüzey
        -1.3f, -0.12f, 0.3f,    0.0f, 0.0f, -1.0f,   0.75f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.3f,    0.0f, 0.0f, -1.0f,   0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.3f,    0.0f, 0.0f, -1.0f,   0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.3f,    0.0f, 0.0f, -1.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.3f,    0.0f, 0.0f, -1.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.12f, 0.3f,    0.0f, 0.0f, -1.0f,   0.75f, 0.1f, 0.1f,

        // Sol yüzey
        -1.3f, -0.12f, 0.3f,    -1.0f, 0.0f, 0.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.12f, 0.6f,    -1.0f, 0.0f, 0.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.6f,    -1.0f, 0.0f, 0.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.6f,    -1.0f, 0.0f, 0.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.09f, 0.3f,    -1.0f, 0.0f, 0.0f,   0.75f, 0.1f, 0.1f,
        -1.3f, -0.12f, 0.3f,    -1.0f, 0.0f, 0.0f,   0.75f, 0.1f, 0.1f,

        // Sağ yüzey
        -1.0f, -0.12f, 0.3f,    1.0f, 0.0f, 0.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.6f,    1.0f, 0.0f, 0.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.6f,    1.0f, 0.0f, 0.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.6f,    1.0f, 0.0f, 0.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.09f, 0.3f,    1.0f, 0.0f, 0.0f,    0.75f, 0.1f, 0.1f,
        -1.0f, -0.12f, 0.3f,    1.0f, 0.0f, 0.0f,    0.75f, 0.1f, 0.1f,

        // İkinci kitap (mavi kitap, birincinin üstünde)
        // Üst yüzey
        -1.25f, -0.05f, 0.55f,  0.0f, 1.0f, 0.0f,    0.1f, 0.1f, 0.8f,
        -0.95f, -0.05f, 0.55f,  0.0f, 1.0f, 0.0f,    0.1f, 0.1f, 0.8f,
        -0.95f, -0.05f, 0.35f,  0.0f, 1.0f, 0.0f,    0.1f, 0.1f, 0.8f,
        -0.95f, -0.05f, 0.35f,  0.0f, 1.0f, 0.0f,    0.1f, 0.1f, 0.8f,
        -1.25f, -0.05f, 0.35f,  0.0f, 1.0f, 0.0f,    0.1f, 0.1f, 0.8f,
        -1.25f, -0.05f, 0.55f,  0.0f, 1.0f, 0.0f,    0.1f, 0.1f, 0.8f,

        // Alt yüzey (bu, birinci kitabın üzerine oturuyor)
        -1.25f, -0.09f, 0.55f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.7f,
        -0.95f, -0.09f, 0.55f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.7f,
        -0.95f, -0.09f, 0.35f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.7f,
        -0.95f, -0.09f, 0.35f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.7f,
        -1.25f, -0.09f, 0.35f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.7f,
        -1.25f, -0.09f, 0.55f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.7f,

        // Ön yüzey
        -1.25f, -0.09f, 0.55f,  0.0f, 0.0f, 1.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.09f, 0.55f,  0.0f, 0.0f, 1.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.55f,  0.0f, 0.0f, 1.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.55f,  0.0f, 0.0f, 1.0f,    0.1f, 0.1f, 0.75f,
        -1.25f, -0.05f, 0.55f,  0.0f, 0.0f, 1.0f,    0.1f, 0.1f, 0.75f,
        -1.25f, -0.09f, 0.55f,  0.0f, 0.0f, 1.0f,    0.1f, 0.1f, 0.75f,

        // Arka yüzey
        -1.25f, -0.09f, 0.35f,  0.0f, 0.0f, -1.0f,   0.1f, 0.1f, 0.75f,
        -0.95f, -0.09f, 0.35f,  0.0f, 0.0f, -1.0f,   0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.35f,  0.0f, 0.0f, -1.0f,   0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.35f,  0.0f, 0.0f, -1.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.05f, 0.35f,  0.0f, 0.0f, -1.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.09f, 0.35f,  0.0f, 0.0f, -1.0f,   0.1f, 0.1f, 0.75f,

        // Sol yüzey
        -1.25f, -0.09f, 0.35f,  -1.0f, 0.0f, 0.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.09f, 0.55f,  -1.0f, 0.0f, 0.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.05f, 0.55f,  -1.0f, 0.0f, 0.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.05f, 0.55f,  -1.0f, 0.0f, 0.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.05f, 0.35f,  -1.0f, 0.0f, 0.0f,   0.1f, 0.1f, 0.75f,
        -1.25f, -0.09f, 0.35f,  -1.0f, 0.0f, 0.0f,   0.1f, 0.1f, 0.75f,

        // Sağ yüzey
        -0.95f, -0.09f, 0.35f,  1.0f, 0.0f, 0.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.09f, 0.55f,  1.0f, 0.0f, 0.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.55f,  1.0f, 0.0f, 0.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.55f,  1.0f, 0.0f, 0.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.05f, 0.35f,  1.0f, 0.0f, 0.0f,    0.1f, 0.1f, 0.75f,
        -0.95f, -0.09f, 0.35f,  1.0f, 0.0f, 0.0f,    0.1f, 0.1f, 0.75f,

        // Üçüncü kitap (yeşil kitap, ikincinin üstünde)
        // Üst yüzey
        -1.2f, -0.01f, 0.5f,    0.0f, 1.0f, 0.0f,    0.1f, 0.7f, 0.3f,
        -1.0f, -0.01f, 0.5f,    0.0f, 1.0f, 0.0f,    0.1f, 0.7f, 0.3f,
        -1.0f, -0.01f, 0.4f,    0.0f, 1.0f, 0.0f,    0.1f, 0.7f, 0.3f,
        -1.0f, -0.01f, 0.4f,    0.0f, 1.0f, 0.0f,    0.1f, 0.7f, 0.3f,
        -1.2f, -0.01f, 0.4f,    0.0f, 1.0f, 0.0f,    0.1f, 0.7f, 0.3f,
        -1.2f, -0.01f, 0.5f,    0.0f, 1.0f, 0.0f,    0.1f, 0.7f, 0.3f,

        // Alt yüzey (bu, ikinci kitabın üzerine oturuyor)
        -1.2f, -0.05f, 0.5f,    0.0f, -1.0f, 0.0f,   0.1f, 0.6f, 0.3f,
        -1.0f, -0.05f, 0.5f,    0.0f, -1.0f, 0.0f,   0.1f, 0.6f, 0.3f,
        -1.0f, -0.05f, 0.4f,    0.0f, -1.0f, 0.0f,   0.1f, 0.6f, 0.3f,
        -1.0f, -0.05f, 0.4f,    0.0f, -1.0f, 0.0f,   0.1f, 0.6f, 0.3f,
        -1.2f, -0.05f, 0.4f,    0.0f, -1.0f, 0.0f,   0.1f, 0.6f, 0.3f,
        -1.2f, -0.05f, 0.5f,    0.0f, -1.0f, 0.0f,   0.1f, 0.6f, 0.3f,

        // Ön yüzey
        -1.2f, -0.05f, 0.5f,    0.0f, 0.0f, 1.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.05f, 0.5f,    0.0f, 0.0f, 1.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.5f,    0.0f, 0.0f, 1.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.5f,    0.0f, 0.0f, 1.0f,    0.1f, 0.65f, 0.3f,
        -1.2f, -0.01f, 0.5f,    0.0f, 0.0f, 1.0f,    0.1f, 0.65f, 0.3f,
        -1.2f, -0.05f, 0.5f,    0.0f, 0.0f, 1.0f,    0.1f, 0.65f, 0.3f,

        // Arka yüzey
        -1.2f, -0.05f, 0.4f,    0.0f, 0.0f, -1.0f,   0.1f, 0.65f, 0.3f,
        -1.0f, -0.05f, 0.4f,    0.0f, 0.0f, -1.0f,   0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.4f,    0.0f, 0.0f, -1.0f,   0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.4f,    0.0f, 0.0f, -1.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.01f, 0.4f,    0.0f, 0.0f, -1.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.05f, 0.4f,    0.0f, 0.0f, -1.0f,   0.1f, 0.65f, 0.3f,

        // Sol yüzey
        -1.2f, -0.05f, 0.4f,    -1.0f, 0.0f, 0.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.05f, 0.5f,    -1.0f, 0.0f, 0.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.01f, 0.5f,    -1.0f, 0.0f, 0.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.01f, 0.5f,    -1.0f, 0.0f, 0.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.01f, 0.4f,    -1.0f, 0.0f, 0.0f,   0.1f, 0.65f, 0.3f,
        -1.2f, -0.05f, 0.4f,    -1.0f, 0.0f, 0.0f,   0.1f, 0.65f, 0.3f,

        // Sağ yüzey
        -1.0f, -0.05f, 0.4f,    1.0f, 0.0f, 0.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.05f, 0.5f,    1.0f, 0.0f, 0.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.5f,    1.0f, 0.0f, 0.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.5f,    1.0f, 0.0f, 0.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.01f, 0.4f,    1.0f, 0.0f, 0.0f,    0.1f, 0.65f, 0.3f,
        -1.0f, -0.05f, 0.4f,    1.0f, 0.0f, 0.0f,    0.1f, 0.65f, 0.3f
    };

    // Masa VAO/VBO
    unsigned int deskVAO, deskVBO;
    glGenVertexArrays(1, &deskVAO);
    glGenBuffers(1, &deskVBO);
    
    glBindVertexArray(deskVAO);
    glBindBuffer(GL_ARRAY_BUFFER, deskVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(deskVertices), deskVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Masa bacağı VAO/VBO
    unsigned int legVAO, legVBO;
    glGenVertexArrays(1, &legVAO);
    glGenBuffers(1, &legVBO);
    
    glBindVertexArray(legVAO);
    glBindBuffer(GL_ARRAY_BUFFER, legVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(legVertices), legVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Monitör VAO/VBO
    unsigned int monitorVAO, monitorVBO;
    glGenVertexArrays(1, &monitorVAO);
    glGenBuffers(1, &monitorVBO);
    
    glBindVertexArray(monitorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, monitorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(monitorVertices), monitorVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Mouse VAO/VBO
    unsigned int mouseVAO, mouseVBO;
    glGenVertexArrays(1, &mouseVAO);
    glGenBuffers(1, &mouseVBO);
    
    glBindVertexArray(mouseVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mouseVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mouseVertices), mouseVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Klavye VAO/VBO
    unsigned int keyboardVAO, keyboardVBO;
    glGenVertexArrays(1, &keyboardVAO);
    glGenBuffers(1, &keyboardVBO);
    
    glBindVertexArray(keyboardVAO);
    glBindBuffer(GL_ARRAY_BUFFER, keyboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(keyboardVertices), keyboardVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Kitap VAO/VBO
    unsigned int bookVAO, bookVBO;
    glGenVertexArrays(1, &bookVAO);
    glGenBuffers(1, &bookVBO);
    
    glBindVertexArray(bookVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bookVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bookVertices), bookVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Ana döngü
    while (!glfwWindowShouldClose(window)) {
        // Zaman hesaplaması
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Temizleme
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Aktive shader
        glUseProgram(shaderProgram);
        
        // Işık ayarları
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);

        // View/Projection dönüşümleri
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Masa üst kısmını çiz
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(deskVAO);
        glDrawArrays(GL_TRIANGLES, 0, 42); // 42 vertices for desk

        // Masa bacakları
        // Sol ön bacak
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.3f, 0.0f, 0.6f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(legVAO);
        glDrawArrays(GL_TRIANGLES, 0, 24);

        // Sağ ön bacak
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.3f, 0.0f, 0.6f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 24);

        // Sol arka bacak
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.3f, 0.0f, -0.6f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 24);

        // Sağ arka bacak
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.3f, 0.0f, -0.6f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 24);

        // Monitör
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(monitorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 30);

        // Mouse
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.4f, 0.0f, 0.4f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(mouseVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Klavye
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.6f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(keyboardVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Kitaplar - 3 tane kitap için tüm vertex verisini çiz (6 üçgen x 6 yüz x 3 kitap = 108 üçgen)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(bookVAO);
        glDrawArrays(GL_TRIANGLES, 0, 108); // 108 vertex (3 kitap, her kitap 6 yüz * 6 vertex)

        // Buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Temizlik
    glDeleteVertexArrays(1, &deskVAO);
    glDeleteVertexArrays(1, &legVAO);
    glDeleteVertexArrays(1, &monitorVAO);
    glDeleteVertexArrays(1, &mouseVAO);
    glDeleteVertexArrays(1, &keyboardVAO);
    glDeleteVertexArrays(1, &bookVAO);
    glDeleteBuffers(1, &deskVBO);
    glDeleteBuffers(1, &legVBO);
    glDeleteBuffers(1, &monitorVBO);
    glDeleteBuffers(1, &mouseVBO);
    glDeleteBuffers(1, &keyboardVBO);
    glDeleteBuffers(1, &bookVBO);
    
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// Görüntü ekranı boyut değişimi
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Fare hareketi
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Y koordinatları tersine çevrilmiş
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Bakış kilitlemesi (90 dereceden fazla yukarı veya aşağı bakmayı engeller)
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Yeni ön vektör hesapla
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

// Fare tekerleği
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // İsteğe bağlı yakınlaştırma/uzaklaştırma olabilir
}

// Klavye girişi
void processInput(GLFWwindow* window) {
    // ESC tuşu ile pencereyi kapatma işlemi
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return; // ESC tuşuna basıldığında diğer tuşları kontrol etmeyi bırak
    }

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= cameraUp * cameraSpeed;
}

// Küre oluşturma fonksiyonu
std::vector<float> createSphereVertices(float radius, int sectorCount, int stackCount, glm::vec3 color) {
    std::vector<float> vertices;
    float x, y, z, xy; // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius; // normal

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle); // r * cos(u)
        z = radius * sinf(stackAngle); // r * sin(u)

        for (int j = 0; j < sectorCount; ++j) {
            sectorAngle = j * sectorStep; // starting from 0 to 2pi

            x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);

            // Renk bilgisi ekliyoruz
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
    }
    return vertices;
}

std::vector<unsigned int> createSphereIndices(int sectorCount, int stackCount) {
    std::vector<unsigned int> indices;
    int k1, k2;

    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1); // beginning of current stack
        k2 = k1 + sectorCount + 1; // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
    return indices;
}

unsigned int createSphere(float radius, int sectorCount, int stackCount, glm::vec3 color) {
    std::vector<float> vertices = createSphereVertices(radius, sectorCount, stackCount, color);
    std::vector<unsigned int> indices = createSphereIndices(sectorCount, stackCount);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return VAO;
}