#define _USE_MATH_DEFINES
#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Camera {
    glm::vec3 position{ 0.0f, 0.0f, 3.0f };
    glm::vec3 front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 2.5f;
    float sensitivity = 0.1f;
    float zoom = 45.0f;

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(sf::Keyboard::Key key, float dt) {
        float v = speed * dt;
        glm::vec3 right = glm::normalize(glm::cross(front, up));

        if (key == sf::Keyboard::Key::W)
            position += front * v;
        if (key == sf::Keyboard::Key::S)
            position -= front * v;
        if (key == sf::Keyboard::Key::A)
            position -= right * v;
        if (key == sf::Keyboard::Key::D)
            position += right * v;
    }

    void processMouse(float xo, float yo) {
        xo *= sensitivity;
        yo *= sensitivity;

        yaw += xo;
        pitch += yo;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
    }
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

GLuint Program = 0;
GLuint VAO = 0;
GLuint VBO = 0;
GLuint EBO = 0;
GLuint normalTexture = 0;
bool useNormal = true;
bool isMouseGrabbed = true;
Camera camera;
glm::vec3 sphereColor(1.0f, 1.0f, 1.0f);

std::vector<Vertex> vertices;
std::vector<unsigned int> indices;

std::string OpenFileDialog() {
    wchar_t filename[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"Image Files\0*.png;*.jpg;*.bmp\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring ws(filename);
        return std::string(ws.begin(), ws.end());
    }
    return "";
}

void createDefaultFlatNormalTexture() {
    if (normalTexture) {
        glDeleteTextures(1, &normalTexture);
        normalTexture = 0;
    }

    const unsigned char flatPixel[4] = { 128, 128, 255, 255 };
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, flatPixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void loadNormal(const std::string& path) {
    sf::Image img;
    if (!img.loadFromFile(path)) {
        std::cerr << "Failed to load normal texture: " << path << std::endl;
        return;
    }

    if (normalTexture) {
        glDeleteTextures(1, &normalTexture);
        normalTexture = 0;
    }

    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        static_cast<GLsizei>(img.getSize().x), static_cast<GLsizei>(img.getSize().y),
        0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

}

void generateSphere() {
    vertices.clear();
    indices.clear();

    int sectors = 36;
    int stacks = 36;
    float r = 1.0f;

    for (int i = 0; i <= stacks; i++) {
        float v = static_cast<float>(i) / static_cast<float>(stacks);
        float phi = static_cast<float>(M_PI) * v;

        for (int j = 0; j <= sectors; j++) {
            float u = static_cast<float>(j) / static_cast<float>(sectors);
            float theta = 2.0f * static_cast<float>(M_PI) * u;

            float x = r * sin(phi) * cos(theta);
            float y = r * cos(phi);
            float z = r * sin(phi) * sin(theta);

            vertices.push_back({
                {x, y, z},
                glm::normalize(glm::vec3(x, y, z)),
                {u, v},
                glm::vec3(0.0f),
                glm::vec3(0.0f)
                });
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < sectors; j++) {
            unsigned int cur = i * (sectors + 1) + j;
            unsigned int next = cur + sectors + 1;
            indices.insert(indices.end(), { cur, next, cur + 1, cur + 1, next, next + 1 });
        }
    }
}

void computeTangents() {
    for (auto& v : vertices) {
        v.tangent = glm::vec3(0.0f);
        v.bitangent = glm::vec3(0.0f);
    }

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i + 0]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.pos - v0.pos;
        glm::vec3 edge2 = v2.pos - v0.pos;

        glm::vec2 dUV1 = v1.uv - v0.uv;
        glm::vec2 dUV2 = v2.uv - v0.uv;

        float det = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
        if (std::fabs(det) < 1e-8f)
            continue;

        float f = 1.0f / det;

        glm::vec3 tangent = f * (edge1 * dUV2.y - edge2 * dUV1.y);
        glm::vec3 bitangent = f * (-edge1 * dUV2.x + edge2 * dUV1.x);

        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;

        v0.bitangent += bitangent;
        v1.bitangent += bitangent;
        v2.bitangent += bitangent;
    }

    for (auto& v : vertices) {
        if (glm::length(v.tangent) < 1e-6f) {
            glm::vec3 helper = (std::fabs(v.normal.z) < 0.999f)
                ? glm::vec3(0.0f, 0.0f, 1.0f)
                : glm::vec3(0.0f, 1.0f, 0.0f);
            v.tangent = glm::normalize(glm::cross(helper, v.normal));
        }
        else {
            v.tangent = glm::normalize(v.tangent - v.normal * glm::dot(v.normal, v.tangent));
        }

        glm::vec3 derivedBitangent = glm::cross(v.normal, v.tangent);
        if (glm::length(derivedBitangent) < 1e-6f) {
            glm::vec3 helper = (std::fabs(v.normal.x) < 0.999f)
                ? glm::vec3(1.0f, 0.0f, 0.0f)
                : glm::vec3(0.0f, 1.0f, 0.0f);
            derivedBitangent = glm::cross(v.normal, helper);
        }
        v.bitangent = glm::normalize(derivedBitangent);
    }
}

const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);

    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));

    TBN = mat3(T, B, N);

    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = N;
    TexCoord = aUV;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fs = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

uniform sampler2D normalMap;
uniform bool useNormal;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;

out vec4 color;

void main() {
    vec3 norm = normalize(Normal);

    if (useNormal) {
        vec3 localNormal = texture(normalMap, TexCoord).rgb;
        localNormal = normalize(localNormal * 2.0 - 1.0);
        norm = normalize(TBN * localNormal);
    }

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 col = (0.1 + diff) * objectColor + spec * vec3(1.0);
    color = vec4(col, 1.0);
}
)";

GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(std::max(1, logLength)));
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Shader compilation failed:\n" << log.data() << std::endl;
    }

    return shader;
}

void initGL() {
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);

    Program = glCreateProgram();
    glAttachShader(Program, v);
    glAttachShader(Program, f);
    glLinkProgram(Program);

    GLint success = 0;
    glGetProgramiv(Program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(std::max(1, logLength)));
        glGetProgramInfoLog(Program, logLength, nullptr, log.data());
        std::cerr << "Program link failed:\n" << log.data() << std::endl;
    }

    glDeleteShader(v);
    glDeleteShader(f);
}

void initBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(glm::vec3) + sizeof(glm::vec2)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

void draw(const sf::Window& window) {
    glUseProgram(Program);

    glm::mat4 model(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    sf::Vector2u size = window.getSize();
    float aspect = size.y == 0 ? 1.0f : static_cast<float>(size.x) / static_cast<float>(size.y);
    glm::mat4 proj = glm::perspective(glm::radians(camera.zoom), aspect, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(Program, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

    glUniform3f(glGetUniformLocation(Program, "lightPos"), 2.0f, 3.0f, 4.0f);
    glUniform3f(glGetUniformLocation(Program, "viewPos"), camera.position.x, camera.position.y, camera.position.z);
    glUniform1i(glGetUniformLocation(Program, "useNormal"), useNormal ? 1 : 0);
    glUniform3f(glGetUniformLocation(Program, "objectColor"), sphereColor.x, sphereColor.y, sphereColor.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(Program, "normalMap"), 0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool ChooseColorDialog(glm::vec3& color) {
    static COLORREF acrCustClr[16] = { 0 };
    CHOOSECOLORW cc = { 0 };
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = GetActiveWindow();
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = RGB(
        static_cast<BYTE>(color.x * 255.0f),
        static_cast<BYTE>(color.y * 255.0f),
        static_cast<BYTE>(color.z * 255.0f)
    );
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        color.x = static_cast<float>(GetRValue(cc.rgbResult)) / 255.0f;
        color.y = static_cast<float>(GetGValue(cc.rgbResult)) / 255.0f;
        color.z = static_cast<float>(GetBValue(cc.rgbResult)) / 255.0f;
        return true;
    }
    return false;
}

int main() {
    sf::Window window(sf::VideoMode({ 1024, 768 }), "Normal Mapping", sf::Style::Default);
    window.setActive(true);

    window.setMouseCursorGrabbed(true);
    window.setMouseCursorVisible(false);

    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "glewInit failed: " << glewGetErrorString(glewStatus) << std::endl;
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, static_cast<GLsizei>(window.getSize().x), static_cast<GLsizei>(window.getSize().y));
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    generateSphere();
    computeTangents();
    initGL();
    initBuffers();
    createDefaultFlatNormalTexture();

    sf::Clock clk;
    sf::Mouse::setPosition(sf::Vector2i(static_cast<int>(window.getSize().x / 2), static_cast<int>(window.getSize().y / 2)), window);
    sf::Vector2i lastMousePos = sf::Mouse::getPosition(window);

    while (window.isOpen()) {
        float dt = clk.restart().asSeconds();

        while (auto e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>())
                window.close();

            if (auto resized = e->getIf<sf::Event::Resized>())
                glViewport(0, 0, static_cast<GLsizei>(resized->size.x), static_cast<GLsizei>(resized->size.y));

            if (auto k = e->getIf<sf::Event::KeyPressed>()) {
                if (k->code == sf::Keyboard::Key::B)
                    useNormal = !useNormal;

                if (k->code == sf::Keyboard::Key::L) {
                    std::string path = OpenFileDialog();
                    if (!path.empty())
                        loadNormal(path);
                }

                if (k->code == sf::Keyboard::Key::C) {
                    ChooseColorDialog(sphereColor);
                }

                if (k->code == sf::Keyboard::Key::Escape) {
                    isMouseGrabbed = !isMouseGrabbed;

                    if (isMouseGrabbed) {
                        window.setMouseCursorGrabbed(true);
                        window.setMouseCursorVisible(false);
                        sf::Mouse::setPosition(sf::Vector2i(static_cast<int>(window.getSize().x / 2), static_cast<int>(window.getSize().y / 2)), window);
                        lastMousePos = sf::Mouse::getPosition(window);
                    }
                    else {
                        window.setMouseCursorGrabbed(false);
                        window.setMouseCursorVisible(true);
                        lastMousePos = sf::Mouse::getPosition(window);
                    }
                }
            }
        }

        sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);
        sf::Vector2i delta = currentMousePos - lastMousePos;

        if (isMouseGrabbed && (delta.x != 0 || delta.y != 0)) {
            camera.processMouse(static_cast<float>(delta.x), static_cast<float>(delta.y));
            sf::Mouse::setPosition(sf::Vector2i(static_cast<int>(window.getSize().x / 2), static_cast<int>(window.getSize().y / 2)), window);
            lastMousePos = sf::Mouse::getPosition(window);
        }
        else if (!isMouseGrabbed) {
            lastMousePos = currentMousePos;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            camera.processKeyboard(sf::Keyboard::Key::W, dt);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            camera.processKeyboard(sf::Keyboard::Key::S, dt);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            camera.processKeyboard(sf::Keyboard::Key::A, dt);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            camera.processKeyboard(sf::Keyboard::Key::D, dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw(window);
        window.display();
    }

    if (normalTexture)
        glDeleteTextures(1, &normalTexture);
    if (EBO)
        glDeleteBuffers(1, &EBO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (Program)
        glDeleteProgram(Program);

    return 0;
}
