#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform Light
{
    vec4 u_LightDir;
};
layout (binding = 2) uniform BaseColor
{
    vec4 u_BaseColor;
};

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;


void main()
{
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(u_LightDir.xyz);
    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.15;
    float lighting = ambient + diffuse * 0.85;
    o_Color = u_BaseColor * lighting;
}
