#version 330 core
out vec4 colour;

in vec2 texCoords;

uniform vec4 spriteColour;
uniform sampler2D tex;

void main()
{
    colour = spriteColour * texture(tex, texCoords);
}