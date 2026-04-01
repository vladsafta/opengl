#define STB_IMAGE_IMPLEMENTATION
#define _USE_MATH_DEFINES

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

// ===================== SHADERS =====================
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec2 TexCoord;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;

    uniform sampler2D texture1;

    void main() {
        FragColor = texture(texture1, TexCoord);
    }
)";

// ===================== CAMERA =====================
glm::vec3 cameraPos = glm::vec3(0.0f, -3.0f, 7.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = -10.0f;
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool firstMouse = true;

// ===================== HELPERS =====================
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

void processInput(GLFWwindow* window) {
    float cameraSpeed = 0.08f;
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
    glm::vec3 newPos = cameraPos;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        newPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        newPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        newPos -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        newPos += right * cameraSpeed;

    if (newPos.x > -9.5f && newPos.x < 9.5f &&
        newPos.y > -9.5f && newPos.y < 9.5f &&
        newPos.z > -9.5f && newPos.z < 9.5f) {
        cameraPos = newPos;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.08f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// ===================== GEOMETRY HELPERS =====================
void addVertex(std::vector<float>& vertices, float x, float y, float z, float u, float v) {
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
    vertices.push_back(u);
    vertices.push_back(v);
}

void addQuad(std::vector<float>& vertices,
             glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
             float u0, float v0, float u1, float v1) {
    addVertex(vertices, a.x, a.y, a.z, u0, v0);
    addVertex(vertices, b.x, b.y, b.z, u1, v0);
    addVertex(vertices, c.x, c.y, c.z, u1, v1);

    addVertex(vertices, c.x, c.y, c.z, u1, v1);
    addVertex(vertices, d.x, d.y, d.z, u0, v1);
    addVertex(vertices, a.x, a.y, a.z, u0, v0);
}

void addBox(std::vector<float>& vertices,
            float minX, float minY, float minZ,
            float maxX, float maxY, float maxZ) {
    glm::vec3 p000(minX, minY, minZ);
    glm::vec3 p001(minX, minY, maxZ);
    glm::vec3 p010(minX, maxY, minZ);
    glm::vec3 p011(minX, maxY, maxZ);
    glm::vec3 p100(maxX, minY, minZ);
    glm::vec3 p101(maxX, minY, maxZ);
    glm::vec3 p110(maxX, maxY, minZ);
    glm::vec3 p111(maxX, maxY, maxZ);

    // front
    addQuad(vertices, p001, p101, p111, p011, 0.0f, 0.0f, 1.0f, 1.0f);
    // back
    addQuad(vertices, p100, p000, p010, p110, 0.0f, 0.0f, 1.0f, 1.0f);
    // left
    addQuad(vertices, p000, p001, p011, p010, 0.0f, 0.0f, 1.0f, 1.0f);
    // right
    addQuad(vertices, p101, p100, p110, p111, 0.0f, 0.0f, 1.0f, 1.0f);
    // top
    addQuad(vertices, p010, p011, p111, p110, 0.0f, 0.0f, 1.0f, 1.0f);
    // bottom
    addQuad(vertices, p000, p100, p101, p001, 0.0f, 0.0f, 1.0f, 1.0f);
}

// ===================== TASK P1: RELIEF =====================
std::vector<float> generateReliefVertices(float size, int steps) {
    std::vector<float> vertices;
    float start = -size / 2.0f;
    float step = size / steps;

    for (int i = 0; i < steps; i++) {
        for (int j = 0; j < steps; j++) {
            float x0 = start + i * step;
            float z0 = start + j * step;
            float x1 = x0 + step;
            float z1 = z0 + step;

            // relief mai mic și mai domol
            float y00 = -10.0f + 0.25f * sin(x0 * 2.0f) * cos(z0 * 2.0f);
            float y10 = -10.0f + 0.25f * sin(x1 * 2.0f) * cos(z0 * 2.0f);
            float y11 = -10.0f + 0.25f * sin(x1 * 2.0f) * cos(z1 * 2.0f);
            float y01 = -10.0f + 0.25f * sin(x0 * 2.0f) * cos(z1 * 2.0f);

            // relief mic, exact în mijlocul ovalului
            if (x0 > -1.4f && x1 < 1.4f && z0 > -1.0f && z1 < 1.0f) {
                addVertex(vertices, x0, y00, z0, 0.0f, 0.0f);
                addVertex(vertices, x1, y10, z0, 1.0f, 0.0f);
                addVertex(vertices, x1, y11, z1, 1.0f, 1.0f);

                addVertex(vertices, x1, y11, z1, 1.0f, 1.0f);
                addVertex(vertices, x0, y01, z1, 0.0f, 1.0f);
                addVertex(vertices, x0, y00, z0, 0.0f, 0.0f);
            }
        }
    }

    return vertices;
}

// ===================== TASK P2: CLOSED OVAL ROAD =====================
std::vector<float> generateOvalRoadVertices(float centerX, float centerZ,
                                            float innerRadiusX, float innerRadiusZ,
                                            float outerRadiusX, float outerRadiusZ,
                                            int segments) {
    std::vector<float> vertices;
    float y = -9.95f;

    for (int i = 0; i < segments; i++) {
        float t0 = 2.0f * M_PI * i / segments;
        float t1 = 2.0f * M_PI * (i + 1) / segments;

        glm::vec3 inner0(centerX + innerRadiusX * cos(t0), y, centerZ + innerRadiusZ * sin(t0));
        glm::vec3 outer0(centerX + outerRadiusX * cos(t0), y, centerZ + outerRadiusZ * sin(t0));
        glm::vec3 inner1(centerX + innerRadiusX * cos(t1), y, centerZ + innerRadiusZ * sin(t1));
        glm::vec3 outer1(centerX + outerRadiusX * cos(t1), y, centerZ + outerRadiusZ * sin(t1));

        float u0 = (float)i / segments * 8.0f;
        float u1 = (float)(i + 1) / segments * 8.0f;

        addVertex(vertices, inner0.x, inner0.y, inner0.z, u0, 0.0f);
        addVertex(vertices, outer0.x, outer0.y, outer0.z, u0, 1.0f);
        addVertex(vertices, outer1.x, outer1.y, outer1.z, u1, 1.0f);

        addVertex(vertices, outer1.x, outer1.y, outer1.z, u1, 1.0f);
        addVertex(vertices, inner1.x, inner1.y, inner1.z, u1, 0.0f);
        addVertex(vertices, inner0.x, inner0.y, inner0.z, u0, 0.0f);
    }

    return vertices;
}

std::vector<float> generateBuildingVertices() {
    std::vector<float> vertices;

    // 12 clădiri 3D
    std::vector<glm::vec3> positions = {
        {-8.5f, -10.0f, -8.0f}, {-8.5f, -10.0f, -4.5f}, {-8.5f, -10.0f, -1.0f},
        {-8.5f, -10.0f,  2.5f}, {-8.5f, -10.0f,  6.0f}, {-8.5f, -10.0f,  8.5f},
        { 6.5f, -10.0f, -8.0f}, { 6.5f, -10.0f, -4.5f}, { 6.5f, -10.0f, -1.0f},
        { 6.5f, -10.0f,  2.5f}, { 6.5f, -10.0f,  6.0f}, { 6.5f, -10.0f,  8.5f}
    };

    std::vector<float> heights = {2.2f, 2.8f, 2.0f, 3.0f, 2.4f, 2.7f, 2.5f, 2.1f, 2.9f, 2.3f, 2.6f, 3.1f};

    for (int i = 0; i < (int)positions.size(); i++) {
        float minX = positions[i].x;
        float minY = positions[i].y;
        float minZ = positions[i].z;

        float maxX = minX + 1.4f;
        float maxY = minY + heights[i];
        float maxZ = minZ + 1.4f;

        addBox(vertices, minX, minY, minZ, maxX, maxY, maxZ);
    }

    return vertices;
}

std::vector<float> generateTreeVertices() {
    std::vector<float> vertices;

    std::vector<glm::vec3> positions = {
        {-6.0f, -10.0f, -7.0f}, {-6.0f, -10.0f, -3.0f}, {-6.0f, -10.0f, 1.0f},
        {-6.0f, -10.0f,  5.0f}, {-6.0f, -10.0f,  8.0f},
        { 5.0f, -10.0f, -7.0f}, { 5.0f, -10.0f, -3.0f}, { 5.0f, -10.0f, 1.0f},
        { 5.0f, -10.0f,  5.0f}, { 5.0f, -10.0f,  8.0f}
    };

    for (auto& p : positions) {
        float w = 0.5f;
        float h = 1.5f;

        addVertex(vertices, p.x - w, p.y,     p.z, 0.0f, 0.0f);
        addVertex(vertices, p.x + w, p.y,     p.z, 1.0f, 0.0f);
        addVertex(vertices, p.x,     p.y + h, p.z, 0.5f, 1.0f);
    }

    return vertices;
}

int main() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Scene in a Cube", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    GLuint grassTexture = loadTexture("obiecte/grass.jpg");
    GLuint skyTexture = loadTexture("obiecte/horizon.jpg");
    GLuint roadTexture = loadTexture("obiecte/road.jpg");
    GLuint buildingTexture = loadTexture("obiecte/building.jpg");
    GLuint treeTexture = loadTexture("obiecte/tree.jpg");

    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Cubul: bază + pereți + tavan
    float cubeVertices[] = {
        // podea
        -10.0f, -10.0f, -10.0f, 0.0f, 0.0f,
         10.0f, -10.0f, -10.0f, 4.0f, 0.0f,
         10.0f, -10.0f,  10.0f, 4.0f, 4.0f,
         10.0f, -10.0f,  10.0f, 4.0f, 4.0f,
        -10.0f, -10.0f,  10.0f, 0.0f, 4.0f,
        -10.0f, -10.0f, -10.0f, 0.0f, 0.0f,

        // față
        -10.0f, -10.0f,  10.0f, 0.0f, 0.0f,
         10.0f, -10.0f,  10.0f, 1.0f, 0.0f,
         10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
         10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
        -10.0f,  10.0f,  10.0f, 0.0f, 1.0f,
        -10.0f, -10.0f,  10.0f, 0.0f, 0.0f,

        // spate
        -10.0f, -10.0f, -10.0f, 0.0f, 0.0f,
         10.0f, -10.0f, -10.0f, 1.0f, 0.0f,
         10.0f,  10.0f, -10.0f, 1.0f, 1.0f,
         10.0f,  10.0f, -10.0f, 1.0f, 1.0f,
        -10.0f,  10.0f, -10.0f, 0.0f, 1.0f,
        -10.0f, -10.0f, -10.0f, 0.0f, 0.0f,

        // stânga
        -10.0f, -10.0f, -10.0f, 0.0f, 0.0f,
        -10.0f, -10.0f,  10.0f, 1.0f, 0.0f,
        -10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
        -10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
        -10.0f,  10.0f, -10.0f, 0.0f, 1.0f,
        -10.0f, -10.0f, -10.0f, 0.0f, 0.0f,

        // dreapta
         10.0f, -10.0f, -10.0f, 0.0f, 0.0f,
         10.0f, -10.0f,  10.0f, 1.0f, 0.0f,
         10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
         10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
         10.0f,  10.0f, -10.0f, 0.0f, 1.0f,
         10.0f, -10.0f, -10.0f, 0.0f, 0.0f,

        // tavan
        -10.0f,  10.0f, -10.0f, 0.0f, 0.0f,
         10.0f,  10.0f, -10.0f, 1.0f, 0.0f,
         10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
         10.0f,  10.0f,  10.0f, 1.0f, 1.0f,
        -10.0f,  10.0f,  10.0f, 0.0f, 1.0f,
        -10.0f,  10.0f, -10.0f, 0.0f, 0.0f
    };

    std::vector<float> reliefVertices = generateReliefVertices(3.2f, 30);
    std::vector<float> roadVertices = generateOvalRoadVertices(0.0f, 0.0f, 3.2f, 2.0f, 5.2f, 4.0f, 100);
    std::vector<float> buildingVertices = generateBuildingVertices();
    std::vector<float> treeVertices = generateTreeVertices();

    // Cube VAO/VBO
    GLuint cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Relief VAO/VBO
    GLuint reliefVAO, reliefVBO;
    glGenVertexArrays(1, &reliefVAO);
    glGenBuffers(1, &reliefVBO);
    glBindVertexArray(reliefVAO);
    glBindBuffer(GL_ARRAY_BUFFER, reliefVBO);
    glBufferData(GL_ARRAY_BUFFER, reliefVertices.size() * sizeof(float), reliefVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Road VAO/VBO
    GLuint roadVAO, roadVBO;
    glGenVertexArrays(1, &roadVAO);
    glGenBuffers(1, &roadVBO);
    glBindVertexArray(roadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
    glBufferData(GL_ARRAY_BUFFER, roadVertices.size() * sizeof(float), roadVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Buildings VAO/VBO
    GLuint buildingVAO, buildingVBO;
    glGenVertexArrays(1, &buildingVAO);
    glGenBuffers(1, &buildingVBO);
    glBindVertexArray(buildingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, buildingVBO);
    glBufferData(GL_ARRAY_BUFFER, buildingVertices.size() * sizeof(float), buildingVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Trees VAO/VBO
    GLuint treeVAO, treeVBO;
    glGenVertexArrays(1, &treeVAO);
    glGenBuffers(1, &treeVBO);
    glBindVertexArray(treeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, treeVBO);
    glBufferData(GL_ARRAY_BUFFER, treeVertices.size() * sizeof(float), treeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.5f, 0.75f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1280.0f / 720.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Podea
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Pereți + tavan
        glBindTexture(GL_TEXTURE_2D, skyTexture);
        glDrawArrays(GL_TRIANGLES, 6, 30);

        // Relief
        glBindVertexArray(reliefVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(reliefVertices.size() / 5));

        // Circuit oval închis
        glBindVertexArray(roadVAO);
        glBindTexture(GL_TEXTURE_2D, roadTexture);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(roadVertices.size() / 5));

        // Clădiri 3D
        glBindVertexArray(buildingVAO);
        glBindTexture(GL_TEXTURE_2D, buildingTexture);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(buildingVertices.size() / 5));

        // Copaci
        glBindVertexArray(treeVAO);
        glBindTexture(GL_TEXTURE_2D, treeTexture);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(treeVertices.size() / 5));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}