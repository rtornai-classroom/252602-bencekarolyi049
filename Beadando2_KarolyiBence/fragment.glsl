#version 330 core

out vec4 FragColor;

uniform vec3 u_color;
uniform int u_isPoint;

void main() {
    // Ha kontrollpontot rajzolunk, kerekítjük
    if (u_isPoint == 1) {
        // A gl_PointCoord egy [0, 1] közötti belső koordináta a pöttyön belül.
        // Ezt átalakítjuk [-1, 1] tartományba, hogy a közepe legyen a (0,0)
        vec2 coord = gl_PointCoord * 2.0 - 1.0;
        
        // Ha a középponttól vett távolság négyzete nagyobb mint 1 (a sugár), eldobjuk a pixelt
        if (dot(coord, coord) > 1.0) {
            discard;
        }
    }
    
    // Színezés a megadott színnel (minden elemnek saját színe lesz)
    FragColor = vec4(u_color, 1.0);
}