#version 330 core

layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform float radius;
uniform float aspectRatio;

void main()
{
	vec4 tangent = normalize(gl_in[1].gl_Position - gl_in[0].gl_Position);
	vec4 normal = vec4(-tangent.y, tangent.x * aspectRatio, 0, 0);

	gl_Position = gl_in[0].gl_Position + normal * radius;
	EmitVertex();

	gl_Position = gl_in[0].gl_Position - normal * radius;
	EmitVertex();

	gl_Position = gl_in[1].gl_Position + normal * radius;
	EmitVertex();

	gl_Position = gl_in[1].gl_Position - normal * radius;
	EmitVertex();

	EndPrimitive();
}
