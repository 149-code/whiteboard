#version 330 core

uniform bool toTex;
in vec2 pos;

void main()
{
	if (toTex)
		gl_Position = vec4(pos - 1, 0.0, 1.0);
	else
		gl_Position = vec4(pos * 2 - 1, 0.0, 1.0);
}
