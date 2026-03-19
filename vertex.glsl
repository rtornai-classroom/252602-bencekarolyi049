#version 330 core

layout (location = 0) in vec2 aPos;
uniform mat4 u_projection;

out vec2 vPos;

void main() {
    // Pozíció beállítása a projekciós mátrixszal
    gl_Position = u_projection * vec4(aPos, 0.0, 1.0);
    
    // A DPI skálázás hibáinak elkerülése végett a pontos koordinátát küldjük át
    vPos = aPos; 
}