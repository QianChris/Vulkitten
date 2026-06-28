#version 460 core
// Required by ARB_separate_shader_objects (enabled by GL_PROGRAM_SEPARABLE)
out gl_PerVertex {
    vec4 gl_Position;
};
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 0) out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
