#version 450

layout(location = 0) out vec4 color;
layout(location = 1) out int entityID;

uniform sampler2D u_Textures[32];

layout (location = 0) in vec4 v_Color;
layout (location = 1) in vec2 v_TexCoord;
layout (location = 2) in float v_TexIndex;
layout (location = 3) in float v_TilingFactor;
layout (location = 4) in flat int v_EntityID;

void main()
{
    int index = int(v_TexIndex);
    vec4 texColor = texture(u_Textures[index], v_TexCoord * v_TilingFactor);
    color = texColor * v_Color;

    entityID = v_EntityID;
}