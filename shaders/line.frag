#version 330 core

in vec2 pixelPos;
out vec4 pixelColor;

uniform vec4 color;

void main()
{
	pixelColor = color;
}
