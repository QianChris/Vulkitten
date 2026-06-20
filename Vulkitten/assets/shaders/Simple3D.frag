#version 450 core

in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 o_Color;

uniform vec3 u_LightDir = vec3(0.5, 0.8, 0.6);
uniform vec4 u_BaseColor = vec4(0.7, 0.7, 0.7, 1.0);

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(u_LightDir);
    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.15;
    float lighting = ambient + diffuse * 0.85;
    o_Color = u_BaseColor * lighting;
}
