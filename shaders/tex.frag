#version 330 core

uniform sampler2D textureSampler;
in vec2 pixelPos;
out vec4 pixelColor;

void main()
{
	pixelColor = texture(textureSampler, pixelPos);

	if (pixelPos.x > 1)
		pixelColor = vec4(1, 0, 0, 0);
}
