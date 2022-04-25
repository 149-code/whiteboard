#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out vec2 pixelPos;
out vec2 centerPos;
uniform float aspectRatio;
uniform float radius;

void main()
{
	centerPos = gl_in[0].gl_Position.xy;
	float adjRadius = radius * aspectRatio;

	gl_Position = gl_in[0].gl_Position + vec4(radius, adjRadius, 0, 0);
	pixelPos = gl_Position.xy;
	EmitVertex();

	gl_Position = gl_in[0].gl_Position + vec4(-radius, adjRadius, 0, 0);
	pixelPos = gl_Position.xy;
	EmitVertex();

	gl_Position = gl_in[0].gl_Position + vec4(radius, -adjRadius, 0, 0);
	pixelPos = gl_Position.xy;
	EmitVertex();

	gl_Position = gl_in[0].gl_Position + vec4(-radius, -adjRadius, 0, 0);
	pixelPos = gl_Position.xy;
	EmitVertex();

	EndPrimitive();
}
