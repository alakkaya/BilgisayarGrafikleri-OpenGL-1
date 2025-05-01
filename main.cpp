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

// Vertex Shader
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    
    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main()
    {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        TexCoord = aTexCoord;
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
    )";
    
    // Fragment Shader
    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;
    
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    uniform int useTexture;
    uniform sampler2D texture1;
    
    void main()
    {
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
        
        vec3 result;
        if (useTexture == 1) {
            result = (ambient + diffuse + specular) * vec3(texture(texture1, TexCoord));
        } else {
            result = (ambient + diffuse + specular) * objectColor;
        }
        
        FragColor = vec4(result, 1.0);
    }
    )";
    
    // Light Cube Fragment Shader
    const char* lightCubeFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    void main()
    {
        FragColor = vec4(1.0); // Beyaz
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
    
            // Monitör taban (koyu gri)
            -0.15f, -0.1f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
             0.15f, -0.1f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
             0.15f, -0.1f, 0.1f,    0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
             0.15f, -0.1f, 0.1f,    0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
            -0.15f, -0.1f, 0.1f,    0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f,
            -0.15f, -0.1f, -0.03f,  0.0f, 0.0f, 1.0f,    0.2f, 0.2f, 0.2f
        };
    
        // Laptop için vertex verileri
        float laptopVertices[] = {
            // pozisyonlar          // normallar         // renkler (gümüş)
            // Alt kısım (klavye bölümü)
            -0.25f, -0.1f, -0.35f,  0.0f, 1.0f, 0.0f,    0.8f, 0.8f, 0.8f,
             0.25f, -0.1f, -0.35f,  0.0f, 1.0f, 0.0f,    0.8f, 0.8f, 0.8f,
             0.25f, -0.1f, -0.55f,  0.0f, 1.0f, 0.0f,    0.8f, 0.8f, 0.8f,
             0.25f, -0.1f, -0.55f,  0.0f, 1.0f, 0.0f,    0.8f, 0.8f, 0.8f,
            -0.25f, -0.1f, -0.55f,  0.0f, 1.0f, 0.0f,    0.8f, 0.8f, 0.8f,
            -0.25f, -0.1f, -0.35f,  0.0f, 1.0f, 0.0f,    0.8f, 0.8f, 0.8f,
    
            // Alt yüzey (taban)
            -0.25f, -0.12f, -0.35f, 0.0f, -1.0f, 0.0f,   0.7f, 0.7f, 0.7f,
             0.25f, -0.12f, -0.35f, 0.0f, -1.0f, 0.0f,   0.7f, 0.7f, 0.7f,
             0.25f, -0.12f, -0.55f, 0.0f, -1.0f, 0.0f,   0.7f, 0.7f, 0.7f,
             0.25f, -0.12f, -0.55f, 0.0f, -1.0f, 0.0f,   0.7f, 0.7f, 0.7f,
            -0.25f, -0.12f, -0.55f, 0.0f, -1.0f, 0.0f,   0.7f, 0.7f, 0.7f,
            -0.25f, -0.12f, -0.35f, 0.0f, -1.0f, 0.0f,   0.7f, 0.7f, 0.7f,
    
            // Ekran kısmı (hafif açık)
            -0.25f, -0.1f, -0.35f,  0.0f, 0.7f, 0.7f,    0.8f, 0.8f, 0.8f,
             0.25f, -0.1f, -0.35f,  0.0f, 0.7f, 0.7f,    0.8f, 0.8f, 0.8f,
             0.25f, 0.25f, -0.2f,   0.0f, 0.7f, 0.7f,    0.8f, 0.8f, 0.8f,
             0.25f, 0.25f, -0.2f,   0.0f, 0.7f, 0.7f,    0.8f, 0.8f, 0.8f,
            -0.25f, 0.25f, -0.2f,   0.0f, 0.7f, 0.7f,    0.8f, 0.8f, 0.8f,
            -0.25f, -0.1f, -0.35f,  0.0f, 0.7f, 0.7f,    0.8f, 0.8f, 0.8f,
    
            // Ekran iç kısmı (siyah)
            -0.23f, -0.07f, -0.34f, 0.0f, 0.7f, 0.7f,    0.05f, 0.05f, 0.05f,
             0.23f, -0.07f, -0.34f, 0.0f, 0.7f, 0.7f,    0.05f, 0.05f, 0.05f,
             0.23f, 0.23f, -0.21f,  0.0f, 0.7f, 0.7f,    0.05f, 0.05f, 0.05f,
             0.23f, 0.23f, -0.21f,  0.0f, 0.7f, 0.7f,    0.05f, 0.05f, 0.05f,
            -0.23f, 0.23f, -0.21f,  0.0f, 0.7f, 0.7f,    0.05f, 0.05f, 0.05f,
            -0.23f, -0.07f, -0.34f, 0.0f, 0.7f, 0.7f,    0.05f, 0.05f, 0.05f
        };
    
        // Klavye
        float keyboardVertices[] = {
            // pozisyonlar          // normallar         // renkler (siyah)
            -0.15f, -0.09f, -0.15f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
             0.15f, -0.09f, -0.15f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
             0.15f, -0.09f, -0.35f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
             0.15f, -0.09f, -0.35f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
            -0.15f, -0.09f, -0.35f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
            -0.15f, -0.09f, -0.15f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
    
            // Klavye alt kısmı
            -0.15f, -0.1f, -0.15f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
             0.15f, -0.1f, -0.15f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
             0.15f, -0.1f, -0.35f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
             0.15f, -0.1f, -0.35f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
            -0.15f, -0.1f, -0.35f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
            -0.15f, -0.1f, -0.15f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f
        };
    
        // Mouse
        float mouseVertices[] = {
            // pozisyonlar          // normallar         // renkler (siyah)
            -0.03f, -0.09f, -0.03f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
             0.03f, -0.09f, -0.03f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
             0.03f, -0.09f, -0.08f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
             0.03f, -0.09f, -0.08f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
            -0.03f, -0.09f, -0.08f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
            -0.03f, -0.09f, -0.03f, 0.0f, 1.0f, 0.0f,    0.15f, 0.15f, 0.15f,
    
            // Mouse alt kısmı
            -0.03f, -0.1f, -0.03f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
             0.03f, -0.1f, -0.03f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
             0.03f, -0.1f, -0.08f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
             0.03f, -0.1f, -0.08f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
            -0.03f, -0.1f, -0.08f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f,
            -0.03f, -0.1f, -0.03f,  0.0f, -1.0f, 0.0f,   0.1f, 0.1f, 0.1f
        };
    
        // Kitap
        float bookVertices[] = {
            // pozisyonlar          // normallar         // renkler (kırmızı)
            -0.15f, -0.09f, 0.2f,   0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
             0.15f, -0.09f, 0.2f,   0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
             0.15f, -0.09f, 0.4f,   0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
             0.15f, -0.09f, 0.4f,   0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
            -0.15f, -0.09f, 0.4f,   0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
            -0.15f, -0.09f, 0.2f,   0.0f, 1.0f, 0.0f,    0.8f, 0.1f, 0.1f,
    
            // Kitap yan kısmı
            -0.15f, -0.09f, 0.2f,   0.0f, 0.0f, -1.0f,   0.7f, 0.1f, 0.1f,
             0.15f, -0.09f, 0.2f,   0.0f, 0.0f, -1.0f,   0.7f, 0.1f, 0.1f,
             0.15f, -0.05f, 0.2f,   0.0f, 0.0f, -1.0f,   0.7f, 0.1f, 0.1f,
             0.15f, -0.05f, 0.2f,   0.0f, 0.0f, -1.0f,   0.7f, 0.1f, 0.1f,
            -0.15f, -0.05f, 0.2f,   0.0f, 0.0f, -1.0f,   0.7f, 0.1f, 0.1f,
            -0.15f, -0.09f, 0.2f,   0.0f, 0.0f, -1.0f,   0.7f, 0.1f, 0.1f,
    
            // Kitap üst kısmı
            -0.15f, -0.05f, 0.2f,   0.0f, 1.0f, 0.0f,    0.9f, 0.9f, 0.9f, // beyaz sayfa
             0.15f, -0.05f, 0.2f,   0.0f, 1.0f, 0.0f,    0.9f, 0.9f, 0.9f,
             0.15f, -0.05f, 0.4f,   0.0f, 1.0f, 0.0f,    0.9f, 0.9f, 0.9f,
             0.15f, -0.05f, 0.4f,   0.0f, 1.0f, 0.0f,    0.9f, 0.9f, 0.9f,
            -0.15f, -0.05f, 0.4f,   0.0f, 1.0f, 0.0f,    0.9f, 0.9f, 0.9f,
            -0.15f, -0.05f, 0.2f,   0.0f, 1.0f, 0.0f,    0.9f, 0.9f, 0.9f
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
    
        // Laptop VAO/VBO
        unsigned int laptopVAO, laptopVBO;
        glGenVertexArrays(1, &laptopVAO);
        glGenBuffers(1, &laptopVBO);
        
        glBindVertexArray(laptopVAO);
        glBindBuffer(GL_ARRAY_BUFFER, laptopVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(laptopVertices), laptopVertices, GL_STATIC_DRAW);
        
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
    
            // Laptop
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.6f, 0.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(laptopVAO);
            glDrawArrays(GL_TRIANGLES, 0, 30);
    
            // Klavye
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(keyboardVAO);
            glDrawArrays(GL_TRIANGLES, 0, 12);
    
            // Mouse
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.25f, 0.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(mouseVAO);
            glDrawArrays(GL_TRIANGLES, 0, 12);
    
            // Kitap
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(bookVAO);
            glDrawArrays(GL_TRIANGLES, 0, 18);
    
            // Buffers
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    
        // Temizlik
        glDeleteVertexArrays(1, &deskVAO);
        glDeleteVertexArrays(1, &legVAO);
        glDeleteVertexArrays(1, &monitorVAO);
        glDeleteVertexArrays(1, &laptopVAO);
        glDeleteVertexArrays(1, &keyboardVAO);
        glDeleteVertexArrays(1, &mouseVAO);
        glDeleteVertexArrays(1, &bookVAO);
        glDeleteBuffers(1, &deskVBO);
        glDeleteBuffers(1, &legVBO);
        glDeleteBuffers(1, &monitorVBO);
        glDeleteBuffers(1, &laptopVBO);
        glDeleteBuffers(1, &keyboardVBO);
        glDeleteBuffers(1, &mouseVBO);
        glDeleteBuffers(1, &bookVBO);
        
        glDeleteProgram(shaderProgram);
    
        glfwTerminate();
        return 0;
    }
    
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

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