#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform CameraUBO
{
    mat4 u_Projection;
    mat4 u_View;
    mat4 u_Model;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_TexCoord;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_Projection * u_View * worldPos;
    v_Normal = mat3(u_Model) * a_Normal;
    v_TexCoord = a_TexCoord;
}
