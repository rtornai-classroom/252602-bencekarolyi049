#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>


const float PI = asin(1.0) * 2.0;

//  VERTEX SHADER

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 u_projection;

out vec2 vPos; // Átküldjük a fragment shadernek

void main() {
    gl_Position = u_projection * vec4(aPos, 0.0, 1.0);
    vPos = aPos; // Megjegyezzük az eredeti [0-600] közötti koordinátát
}
)";

//   FRAGMENT SHADER 

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 vPos; // A vertex shaderből érkező koordináta
out vec4 FragColor;

uniform int u_isCircle;
uniform vec2 u_circleCenter;
uniform float u_radius;
uniform vec3 u_colorCenter;
uniform vec3 u_colorEdge;
uniform vec3 u_lineColor;

void main() {
    if (u_isCircle == 1) {
        // Távolság a kapott vPos és a középpont között
        float dist = distance(vPos, u_circleCenter);
        
        // Sugarat meghaladó pixelek eldobása (így lesz kör a négyzetből)
        if (dist > u_radius) discard; 

        // Lineáris interpoláció (0.0 a közepén, 1.0 a szélén)
        float t = dist / u_radius;
        vec3 finalColor = mix(u_colorCenter, u_colorEdge, t);
        
        FragColor = vec4(finalColor, 1.0);
    } else {
        // Ha vonal
        FragColor = vec4(u_lineColor, 1.0);
    }
}
)";

int main() {
    if (!glfwInit()) return -1;

    // 1. FELADAT rész:A program létrehoz egy pontosan 600 pixel oldalhosszúságú négyzetes ablakot
    GLFWwindow* window = glfwCreateWindow(600, 600, "Beadando 1.", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync (villódzás ellen)
    glewInit();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs, 1, &vertexShaderSource, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs, 1, &fragmentShaderSource, NULL); glCompileShader(fs);
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);

    GLuint vbo, vao; glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0); glEnableVertexAttribArray(0);

    float m[16] = { 2 / 600.0f,0,0,0, 0,2 / 600.0f,0,0, 0,0,-1,0, -1,-1,0,1 };

    // 2. és 3. FELADAT: A kör elindul a közepéről, (cx=300, cy=300) és legyen r=50 pixel sugarú vx az a sebesseg
    float cx = 300, cy = 300, vx = 5, vy = 0, ly = 300, r = 50;
    bool sPr = false;

    while (!glfwWindowShouldClose(window)) {

        // BÓNUSZ 1: A fel és le nyíl billentyűkkel lehet mozgatni a vonalat fel, le
        if (glfwGetKey(window, GLFW_KEY_UP)) ly += 3;
        if (glfwGetKey(window, GLFW_KEY_DOWN)) ly -= 3;

        // BÓNUSZ 3: "alfa=25 érték alapján vegyünk fel egy 10 pixel hosszúságú irányvektort. Indítsuk el a kört ezen irányvektor mentén az S billentyű lenyomásával
        if (glfwGetKey(window, GLFW_KEY_S) && !sPr) {
            sPr = true;
            vx = 10 * cos(25 * PI / 180); // X komponens kiszámítása trigonometriával
            vy = 10 * sin(25 * PI / 180); // Y komponens kiszámítása trigonometriával
        }

        // Mozgás alkalmazása
        cx += vx;
        cy += vy;

        // 3. FELADAT & BÓNUSZ 3: képernyő szélét pontosan érintve a kör visszapattan
        if (cx + r > 600) { cx = 600 - r; vx = -vx; } // Jobb fal
        if (cx - r < 0) { cx = r;       vx = -vx; } // Bal fal
        if (cy + r > 600) { cy = 600 - r; vy = -vy; } // Felső fal
        if (cy - r < 0) { cy = r;       vy = -vy; } // Alsó fal

        // Metszésvizsgálat a szakasz és a kör között
        float clX = std::max(200.0f, std::min(cx, 400.0f));
        float clY = std::max(ly - 1.5f, std::min(cy, ly + 1.5f));
        bool hit = (pow(cx - clX, 2) + pow(cy - clY, 2)) <= r * r;

        // 1. FELADAT: négyzetes ablakot sárga háttérrel
        glClearColor(1, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "u_projection"), 1, 0, m);

        //4. FELADAT: SZAKASZ RAJZOLÁSA 
        // kék színnel rajzolunk harmad oldalhosszúságú (200px), 3 pixel vastagságú vízszintes szakaszt
        // X koordináták: 200-tól 400-ig tart || Y koordináták: ly-1.5-től ly+1.5-ig tart.
        float lv[] = { 200,ly - 1.5f, 400,ly - 1.5f, 400,ly + 1.5f, 200,ly + 1.5f };
        glBufferData(GL_ARRAY_BUFFER, sizeof(lv), lv, GL_STREAM_DRAW);
        glUniform1i(glGetUniformLocation(prog, "u_isCircle"), 0);
        glUniform3f(glGetUniformLocation(prog, "u_lineColor"), 0, 0, 1); // Kék szín
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 2. FELADAT: Megrajzoljuk a kört 
        // Rajzolunk egy négyzetet , amibe a shader belevágja a kört
        float cv[] = { cx - r,cy - r, cx + r,cy - r, cx + r,cy + r, cx - r,cy + r };
        glBufferData(GL_ARRAY_BUFFER, sizeof(cv), cv, GL_STREAM_DRAW);
        glUniform1i(glGetUniformLocation(prog, "u_isCircle"), 1);
        glUniform2f(glGetUniformLocation(prog, "u_circleCenter"), cx, cy);
        glUniform1f(glGetUniformLocation(prog, "u_radius"), r);

        // BÓNUSZ 2: "Amikor a körlap mozgás közben már nincs metszésben a szakasszal, a határvonal zöld és a centrum piros árnyalata cserelődnek fel
        if (hit) {
            // Metszésben van az
            glUniform3f(glGetUniformLocation(prog, "u_colorCenter"), 1, 0, 0); // Piros centrum
            glUniform3f(glGetUniformLocation(prog, "u_colorEdge"), 0, 1, 0);   // Zöld határvonal
        }
        else {
            // Nincs metszésben a színek felcserélődnek
            glUniform3f(glGetUniformLocation(prog, "u_colorCenter"), 0, 1, 0); // Zöld centrum
            glUniform3f(glGetUniformLocation(prog, "u_colorEdge"), 1, 0, 0);   // Piros határvonal
        }
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}