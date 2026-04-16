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

// --- GLOBÁLIS VÁLTOZÓK AZ EGÉRKEZELÉSHEZ ---
struct Point { float x, y; };
std::vector<Point> controlPoints; // A kontrollpontok listája
int draggedPointIndex = -1;       // Épp melyik pontot húzzuk (-1 = semelyiket)
const float pointRadius = 6.0f;   // A kerek pontok sugara (d=12 pixel)

// --- MATEMATIKA: Binomiális együttható (n alatt a k) ---
double nCr(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    double res = 1;
    for (int i = 1; i <= k; ++i) {
        res = res * (n - i + 1) / i;
    }
    return res;
}

// --- EGÉR KATTINTÁS KEZELŐ ---
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Megnézzük, rákattintottunk-e egy meglévő pontra (Drag kezdete)
            bool clickedOnPoint = false;
            for (size_t i = 0; i < controlPoints.size(); ++i) {
                float dx = controlPoints[i].x - (float)xpos;
                float dy = controlPoints[i].y - (float)ypos;
                if (sqrt(dx * dx + dy * dy) <= pointRadius + 2.0f) { // Kis ráhagyás a könnyebb kattintásért
                    draggedPointIndex = i;
                    clickedOnPoint = true;
                    break;
                }
            }
            // BÓNUSZ 2: Ha üres helyre kattintottunk, új pont létrehozása
            if (!clickedOnPoint) {
                controlPoints.push_back({ (float)xpos, (float)ypos });
            }
        }
        else if (action == GLFW_RELEASE) {
            // Drag befejezése
            draggedPointIndex = -1;
        }
    }

    // BÓNUSZ 2: Jobb gombbal meglévő pont eltávolítása
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

// --- EGÉR MOZGÁS KEZELŐ (DRAG AND DROP) ---
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // 3. FELADAT: A kontrollpontok a drag-and-drop technikával mozgathatóak
    if (draggedPointIndex != -1) {
        controlPoints[draggedPointIndex].x = (float)xpos;
        controlPoints[draggedPointIndex].y = (float)ypos;
    }
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(600, 600, "Bezier-gorbe Szerkeszto", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glewInit();

    // Egér funkciók bekötése a GLFW-be
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // Fontos: Engedélyezzük, hogy a shader átméretezze a pontokat
    glEnable(GL_PROGRAM_POINT_SIZE);

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

    // Projekciós mátrix: A bal felső sarok a (0,0), jobb alsó a (600,600). Így egyezik az egérrel!
    float m[16] = {
         2.0f / 600.0f,  0.0f,         0.0f, 0.0f,
         0.0f,        -2.0f / 600.0f,  0.0f, 0.0f,
         0.0f,         0.0f,        -1.0f, 0.0f,
        -1.0f,         1.0f,         0.0f, 1.0f
    };

    // 2. FELADAT: Kezdetben 4 darab kontrollpont
    controlPoints = { {100, 500}, {200, 100}, {400, 100}, {500, 500} };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f); // Világosszürke háttér
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_projection"), 1, 0, m);

        // --- 1. KONTROLLPOLIGON RAJZOLÁSA (BÓNUSZ 1) ---
        if (controlPoints.size() > 1) {
            std::vector<float> polyVertices;
            for (const auto& p : controlPoints) {
                polyVertices.push_back(p.x); polyVertices.push_back(p.y);
            }
            glBufferData(GL_ARRAY_BUFFER, polyVertices.size() * sizeof(float), polyVertices.data(), GL_DYNAMIC_DRAW);
            glUniform1i(glGetUniformLocation(prog, "u_isPoint"), 0);
            glUniform3f(glGetUniformLocation(prog, "u_color"), 0.7f, 0.7f, 0.7f); // Szürke szín
            // GL_LINE_STRIP: Összeköti a pontokat, de nem záródik vissza!
            glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());
        }

        // --- 2. BÉZIER-GÖRBE RAJZOLÁSA (1. FELADAT) ---
        if (controlPoints.size() > 1) {
            std::vector<float> bezierVertices;
            int n = controlPoints.size() - 1; // A görbe fokszáma
            int segments = 100; // 100 darab vonalkából rakjuk össze a görbét (részletes törött vonal)

            for (int i = 0; i <= segments; ++i) {
                float t = (float)i / (float)segments;
                float bx = 0.0f;
                float by = 0.0f;

                // Bézier matematikai képlet alkalmazása (Bernstein-polinom)
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
            glUniform3f(glGetUniformLocation(prog, "u_color"), 1.0f, 0.0f, 0.0f); // Piros szín a görbének
            glDrawArrays(GL_LINE_STRIP, 0, bezierVertices.size() / 2);
        }

        // --- 3. KONTROLLPONTOK RAJZOLÁSA (2. FELADAT) ---
        if (!controlPoints.empty()) {
            std::vector<float> ptVertices;
            for (const auto& p : controlPoints) {
                ptVertices.push_back(p.x); ptVertices.push_back(p.y);
            }
            glBufferData(GL_ARRAY_BUFFER, ptVertices.size() * sizeof(float), ptVertices.data(), GL_DYNAMIC_DRAW);
            glUniform1i(glGetUniformLocation(prog, "u_isPoint"), 1); // Bekapcsoljuk a kerekítést a shaderben!
            glUniform1f(glGetUniformLocation(prog, "u_pointSize"), pointRadius * 2.0f); // d=12 pixel
            glUniform3f(glGetUniformLocation(prog, "u_color"), 0.0f, 0.0f, 1.0f); // Kék pontok
            glDrawArrays(GL_POINTS, 0, controlPoints.size());
        }

        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}