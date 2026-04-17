#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

// --- FÁJLBEOLVASÓ FÜGGVÉNY ---
std::string readShaderFile(const char* filePath) {
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "HIBA: Nem talalhato vagy nem olvashato a shader fajl: " << filePath << std::endl;
        return "";
    }
}

// --- GLOBÁLIS VÁLTOZÓK ---
struct Point { float x, y; };
std::vector<Point> controlPoints;
int draggedPointIndex = -1;


const float pointRadius = 4.0f;

// --- MATEMATIKA (Bernstein-polinomhoz) ---
double nCr(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    double res = 1;
    for (int i = 1; i <= k; ++i) {
        res = res * (n - i + 1) / i;
    }
    return res;
}

// --- EGÉR KATTINTÁS (Bal: hozzáad/megfog, Jobb: töröl) ---
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            bool clickedOnPoint = false;
            for (size_t i = 0; i < controlPoints.size(); ++i) {
                float dx = controlPoints[i].x - (float)xpos;
                float dy = controlPoints[i].y - (float)ypos;
                if (sqrt(dx * dx + dy * dy) <= pointRadius + 2.0f) {
                    draggedPointIndex = i;
                    clickedOnPoint = true;
                    break;
                }
            }
            if (!clickedOnPoint) {
                controlPoints.push_back({ (float)xpos, (float)ypos });
            }
        }
        else if (action == GLFW_RELEASE) {
            draggedPointIndex = -1;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            float dx = controlPoints[i].x - (float)xpos;
            float dy = controlPoints[i].y - (float)ypos;
            if (sqrt(dx * dx + dy * dy) <= pointRadius + 2.0f) {
                controlPoints.erase(controlPoints.begin() + i);
                break;
            }
        }
    }
}

// --- EGÉR MOZGÁS (Drag-and-drop) ---
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (draggedPointIndex != -1) {
        controlPoints[draggedPointIndex].x = (float)xpos;
        controlPoints[draggedPointIndex].y = (float)ypos;
    }
}

int main() {
    // GLFW és ablak inicializálása
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(600, 600, "Bezier-gorbe Beadando", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glewInit();

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // Shaderek beolvasása és fordítása
    std::string vertexCode = readShaderFile("vertex.glsl");
    std::string fragmentCode = readShaderFile("fragment.glsl");
    const char* vertexSourcePtr = vertexCode.c_str();
    const char* fragmentSourcePtr = fragmentCode.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs, 1, &vertexSourcePtr, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs, 1, &fragmentSourcePtr, NULL); glCompileShader(fs);
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);

    GLuint vbo, vao; glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0); glEnableVertexAttribArray(0);

    // Projekciós mátrix beállítása a 600x600-as ablakhoz
    float m[16] = {
         2.0f / 600.0f,  0.0f,         0.0f, 0.0f,
         0.0f,        -2.0f / 600.0f,  0.0f, 0.0f,
         0.0f,         0.0f,        -1.0f, 0.0f,
        -1.0f,         1.0f,         0.0f, 1.0f
    };

    // Kezdeti 4 pont lerakása
    controlPoints = { {100, 500}, {200, 100}, {400, 100}, {500, 500} };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Fekete háttér (glClearColor)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_projection"), 1, 0, m);

        // =========================================================
        // 1. KONTROLLPOLIGON RAJZOLÁSA (Vékony Kék)
        // Bónusz: Nem záródik vissza!
        // =========================================================
        if (controlPoints.size() > 1) {
            std::vector<float> polyVertices;
            for (const auto& p : controlPoints) {
                polyVertices.push_back(p.x); polyVertices.push_back(p.y);
            }
            glBufferData(GL_ARRAY_BUFFER, polyVertices.size() * sizeof(float), polyVertices.data(), GL_DYNAMIC_DRAW);
            glUniform1i(glGetUniformLocation(prog, "u_isPoint"), 0);
            glUniform3f(glGetUniformLocation(prog, "u_color"), 0.0f, 0.5f, 1.0f); // Szín: Kék
            glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());
        }

        // =========================================================
        // 2. BÉZIER-GÖRBE RAJZOLÁSA (Vastag Piros)
        // Kötelező: Bármennyi pontot kezel, Bernstein-polinommal
        // =========================================================
        if (controlPoints.size() > 1) {
            std::vector<float> bezierVertices;
            int n = controlPoints.size() - 1;
            int segments = 100;

            for (int i = 0; i <= segments; ++i) {
                float t = (float)i / (float)segments;
                float bx = 0.0f; float by = 0.0f;
                for (int j = 0; j <= n; ++j) {
                    float bPoly = nCr(n, j) * pow(1.0f - t, n - j) * pow(t, j);
                    bx += bPoly * controlPoints[j].x;
                    by += bPoly * controlPoints[j].y;
                }
                bezierVertices.push_back(bx);
                bezierVertices.push_back(by);
            }
            glBufferData(GL_ARRAY_BUFFER, bezierVertices.size() * sizeof(float), bezierVertices.data(), GL_DYNAMIC_DRAW);

            glUniform1i(glGetUniformLocation(prog, "u_isPoint"), 0);
            glUniform3f(glGetUniformLocation(prog, "u_color"), 1.0f, 0.0f, 0.0f); // Szín: Tiszta piros

            glLineWidth(4.0f); // Vastagított vonal
            glDrawArrays(GL_LINE_STRIP, 0, bezierVertices.size() / 2);
            glLineWidth(1.0f); // Visszaállítás
        }

        // =========================================================
        // 3. KONTROLLPONTOK RAJZOLÁSA (Kerek formák)
        // Kötelező: 3 <= d <= 9 átmérő (nálunk d=8 pixel)
        // =========================================================
        if (!controlPoints.empty()) {
            glUniform1i(glGetUniformLocation(prog, "u_isPoint"), 0);
            glUniform3f(glGetUniformLocation(prog, "u_color"), 1.0f, 0.3f, 0.1f); // Szín: Narancsos-piros

            for (const auto& p : controlPoints) {
                std::vector<float> circleVertices;
                int sides = 30; // 30 db háromszögből álló tökéletes kör

                // Kör középpontja
                circleVertices.push_back(p.x);
                circleVertices.push_back(p.y);

                // Kör kerülete
                for (int i = 0; i <= sides; ++i) {
                    float angle = i * 2.0f * 3.1415926f / sides;
                    circleVertices.push_back(p.x + cos(angle) * pointRadius);
                    circleVertices.push_back(p.y + sin(angle) * pointRadius);
                }

                glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLE_FAN, 0, sides + 2); // Kerek bogyó kirajzolása
            }
        }

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
