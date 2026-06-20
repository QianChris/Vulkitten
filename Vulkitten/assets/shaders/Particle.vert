#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in float inSize;

layout (binding = 2) uniform UBO 
{
	mat4 projection;
	mat4 view;
	vec2 screendim;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
	const float spriteSize = 0.05 * inSize;

	vec4 eyePos = ubo.view * vec4(inPos.x, inPos.y, inPos.z, 1.0); 
	vec4 projectedCorner = ubo.projection * vec4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w);

	gl_PointSize = clamp(ubo.screendim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);
	gl_Position = ubo.projection * eyePos;
}