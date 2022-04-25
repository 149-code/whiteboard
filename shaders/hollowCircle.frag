#version 330 core

in vec2 pixelPos;
in vec2 centerPos;
out vec4 pixelColor;

uniform float radius;
uniform float aspectRatio;
uniform vec4 color;

void main()
{
	vec2 diff = (pixelPos - centerPos) / vec2(1, aspectRatio);

	if (abs(length(diff) - radius) < 0.001)
		pixelColor = color;
	else
		pixelColor = vec4(0, 0, 0, 0);
}
