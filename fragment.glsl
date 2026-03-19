#version 330 core

in vec2 vPos; 
out vec4 FragColor;

// A C++ kódból érkező változók (Uniforms)
uniform int u_isCircle;
uniform vec2 u_circleCenter;
uniform float u_radius;
uniform vec3 u_colorCenter;
uniform vec3 u_colorEdge;
uniform vec3 u_lineColor;

void main() {
    // Ha kört rajzolunk (u_isCircle == 1)
    if (u_isCircle == 1) {
        // Távolság a kapott vPos és a középpont között
        float dist = distance(vPos, u_circleCenter);
                
        // Sugarat meghaladó pixelek eldobása (így lesz kör a négyzetből)
        if (dist > u_radius) {
            discard; 
        }

        // Lineáris interpoláció (0.0 a közepén, 1.0 a szélén)
        float t = dist / u_radius;
        vec3 finalColor = mix(u_colorCenter, u_colorEdge, t);
                
        FragColor = vec4(finalColor, 1.0);
    } 
    // Ha a vízszintes vonalat rajzoljuk
    else {
        FragColor = vec4(u_lineColor, 1.0);
    }
}