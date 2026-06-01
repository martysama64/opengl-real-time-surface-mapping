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
#include <algorithm>
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
GLuint displacementTexture = 0;
GLuint colorTexture = 0;
bool useDisplacement = true;
bool useColorTexture = true;
bool isMouseGrabbed = true;
Camera camera;
glm::vec3 sphereColor(1.0f, 1.0f, 1.0f);
glm::vec2 displacementTexelSize(1.0f / 512.0f, 1.0f / 512.0f);
float displacementStrength = 0.08f;
float normalStrength = 3.0f;
float tessLevel = 16.0f;

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

void createDefaultFlatDisplacementTexture() {
    if (displacementTexture) {
        glDeleteTextures(1, &displacementTexture);
        displacementTexture = 0;
    }

    const unsigned char flatPixel[4] = { 128, 128, 128, 255 };
    glGenTextures(1, &displacementTexture);
    glBindTexture(GL_TEXTURE_2D, displacementTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, flatPixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    displacementTexelSize = glm::vec2(1.0f, 1.0f);
}

void createDefaultFlatColorTexture() {
    if (colorTexture) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }

    const unsigned char whitePixel[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void loadDisplacement(const std::string& path) {
    sf::Image img;
    if (!img.loadFromFile(path)) {
        std::cerr << "Failed to load displacement texture: " << path << std::endl;
        return;
    }

    if (displacementTexture) {
        glDeleteTextures(1, &displacementTexture);
        displacementTexture = 0;
    }

    glGenTextures(1, &displacementTexture);
    glBindTexture(GL_TEXTURE_2D, displacementTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        static_cast<GLsizei>(img.getSize().x), static_cast<GLsizei>(img.getSize().y),
        0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    displacementTexelSize = glm::vec2(
        1.0f / static_cast<float>(img.getSize().x),
        1.0f / static_cast<float>(img.getSize().y)
    );
}

void loadColorTexture(const std::string& path) {
    sf::Image img;
    if (!img.loadFromFile(path)) {
        std::cerr << "Failed to load color texture: " << path << std::endl;
        return;
    }

    if (colorTexture) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

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
#version 410 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

out VertexData {
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
} vsOut;

void main() {
    vsOut.Position = aPos;
    vsOut.Normal = aNormal;
    vsOut.TexCoord = aUV;
    vsOut.Tangent = aTangent;
    vsOut.Bitangent = aBitangent;
    gl_Position = vec4(aPos, 1.0);
}
)";

const char* tcs = R"(
#version 410 core
layout(vertices = 3) out;

in VertexData {
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
} tcsIn[];

out VertexData {
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
} tcsOut[];

uniform float tessLevel;

void main() {
    tcsOut[gl_InvocationID].Position = tcsIn[gl_InvocationID].Position;
    tcsOut[gl_InvocationID].Normal = tcsIn[gl_InvocationID].Normal;
    tcsOut[gl_InvocationID].TexCoord = tcsIn[gl_InvocationID].TexCoord;
    tcsOut[gl_InvocationID].Tangent = tcsIn[gl_InvocationID].Tangent;
    tcsOut[gl_InvocationID].Bitangent = tcsIn[gl_InvocationID].Bitangent;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = tessLevel;
        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;
    }
}
)";

const char* tes = R"(
#version 410 core
layout(triangles, equal_spacing, ccw) in;

in VertexData {
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
    vec3 Tangent;
    vec3 Bitangent;
} tesIn[];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D displacementMap;
uniform bool useDisplacement;
uniform float displacementStrength;

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

vec2 transformUV(vec2 uv) {
    return uv;
}

void main() {
    vec3 position = interpolate3D(tesIn[0].Position, tesIn[1].Position, tesIn[2].Position);
    vec3 normal = normalize(interpolate3D(tesIn[0].Normal, tesIn[1].Normal, tesIn[2].Normal));
    vec2 texCoord = interpolate2D(tesIn[0].TexCoord, tesIn[1].TexCoord, tesIn[2].TexCoord);
    vec2 displacementUV = transformUV(texCoord);
    vec3 tangent = normalize(interpolate3D(tesIn[0].Tangent, tesIn[1].Tangent, tesIn[2].Tangent));
    vec3 bitangent = normalize(interpolate3D(tesIn[0].Bitangent, tesIn[1].Bitangent, tesIn[2].Bitangent));

    if (useDisplacement) {
        float h = texture(displacementMap, displacementUV).r;
        position += normal * ((h - 0.5) * displacementStrength);
    }

    vec3 N = normalize(mat3(transpose(inverse(model))) * normal);
    vec3 T = normalize(mat3(model) * tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(mat3(model) * bitangent);
    B = normalize(B - dot(B, N) * N - dot(B, T) * T);

    if (length(B) < 1e-6)
        B = normalize(cross(N, T));

    TBN = mat3(T, B, N);
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = N;
    TexCoord = texCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fs = R"(
#version 410 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

uniform sampler2D displacementMap;
uniform sampler2D colorMap;
uniform bool useDisplacement;
uniform bool useColorTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec2 displacementTexelSize;
uniform float normalStrength;

out vec4 color;

vec2 transformUV(vec2 uv) {
    return uv;
}

void main() {
    vec3 norm = normalize(Normal);
    vec2 uv = transformUV(TexCoord);

    if (useDisplacement) {
        float hL = texture(displacementMap, uv + vec2(-displacementTexelSize.x, 0.0)).r;
        float hR = texture(displacementMap, uv + vec2( displacementTexelSize.x, 0.0)).r;
        float hD = texture(displacementMap, uv + vec2(0.0, -displacementTexelSize.y)).r;
        float hU = texture(displacementMap, uv + vec2(0.0,  displacementTexelSize.y)).r;

        float dx = hL - hR;
        float dy = hD - hU;

        vec3 localNormal = normalize(vec3(dx * normalStrength, dy * normalStrength, 1.0));
        norm = normalize(TBN * localNormal);
    }

    vec3 baseColor = objectColor;

    if (useColorTexture)
        baseColor *= texture(colorMap, uv).rgb;

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 col = (0.1 + diff) * baseColor + spec * vec3(1.0);
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
    GLuint c = compile(GL_TESS_CONTROL_SHADER, tcs);
    GLuint e = compile(GL_TESS_EVALUATION_SHADER, tes);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);

    Program = glCreateProgram();
    glAttachShader(Program, v);
    glAttachShader(Program, c);
    glAttachShader(Program, e);
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
    glDeleteShader(c);
    glDeleteShader(e);
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
    glUniform1i(glGetUniformLocation(Program, "useDisplacement"), useDisplacement ? 1 : 0);
    glUniform1i(glGetUniformLocation(Program, "useColorTexture"), useColorTexture ? 1 : 0);
    glUniform3f(glGetUniformLocation(Program, "objectColor"), sphereColor.x, sphereColor.y, sphereColor.z);
    glUniform2f(glGetUniformLocation(Program, "displacementTexelSize"), displacementTexelSize.x, displacementTexelSize.y);
    glUniform1f(glGetUniformLocation(Program, "displacementStrength"), displacementStrength);
    glUniform1f(glGetUniformLocation(Program, "normalStrength"), normalStrength);
    glUniform1f(glGetUniformLocation(Program, "tessLevel"), tessLevel);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, displacementTexture);
    glUniform1i(glGetUniformLocation(Program, "displacementMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(Program, "colorMap"), 1);

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glBindVertexArray(VAO);
    glDrawElements(GL_PATCHES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
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
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 4;
    settings.minorVersion = 1;

    sf::Window window(sf::VideoMode({ 1024, 768 }), "Displacement Mapping", sf::Style::Default, sf::State::Windowed, settings);
    window.setActive(true);

    window.setMouseCursorGrabbed(true);
    window.setMouseCursorVisible(false);

    glewExperimental = GL_TRUE;

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
    createDefaultFlatDisplacementTexture();
    createDefaultFlatColorTexture();

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
                    useDisplacement = !useDisplacement;

                if (k->code == sf::Keyboard::Key::L) {
                    std::string path = OpenFileDialog();
                    if (!path.empty())
                        loadDisplacement(path);
                }

                if (k->code == sf::Keyboard::Key::K) {
                    std::string path = OpenFileDialog();
                    if (!path.empty())
                        loadColorTexture(path);
                }

                if (k->code == sf::Keyboard::Key::T)
                    useColorTexture = !useColorTexture;

                if (k->code == sf::Keyboard::Key::C) {
                    ChooseColorDialog(sphereColor);
                }

                if (k->code == sf::Keyboard::Key::Up)
                    displacementStrength = std::min(1.0f, displacementStrength + 0.02f);

                if (k->code == sf::Keyboard::Key::Down)
                    displacementStrength = std::max(0.0f, displacementStrength - 0.02f);

                if (k->code == sf::Keyboard::Key::Add || k->code == sf::Keyboard::Key::Equal)
                    displacementStrength = std::min(1.0f, displacementStrength + 0.01f);

                if (k->code == sf::Keyboard::Key::Subtract || k->code == sf::Keyboard::Key::Hyphen)
                    displacementStrength = std::max(0.0f, displacementStrength - 0.01f);

                if (k->code == sf::Keyboard::Key::PageUp)
                    tessLevel = std::min(32.0f, tessLevel + 1.0f);

                if (k->code == sf::Keyboard::Key::PageDown)
                    tessLevel = std::max(1.0f, tessLevel - 1.0f);

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

    if (displacementTexture)
        glDeleteTextures(1, &displacementTexture);
    if (colorTexture)
        glDeleteTextures(1, &colorTexture);
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
