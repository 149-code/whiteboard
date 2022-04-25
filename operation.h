#pragma once

#include <ct/glt.h>
#include <ct/vt.h>

#include <cglm/struct.h>

struct Shaders
{
	GLuint texShader;
	GLuint lineShader;
	GLuint hollowCircleShader;
	GLuint circleShader;
	GLuint rectShader;
};

static struct Shaders shaderStore;

struct Operation
{
	union {
		vtVec(vec2s) points;
		vtVec(char) text;
	};
	int op;
	float stroke_width;
	vec4s color;
};

enum OperationTypes
{
	OT_NULL,
	OT_CLEAR,
	OT_ERASER,
	OT_STROKE,
	OT_HOLLOW_DOT,
	OT_LINE,
	OT_CIRCLE,
	OT_TEXT,
};

struct DrawHistory
{
	vtVec(struct Operation) operations;

	GLuint fbo;
	GLuint tex;

	GLTvertexStore vs;
};

void loadShaders();
void opRender(struct Operation op, GLuint fbo, bool toTex);
void opDelete(struct Operation op);
struct Operation fixOperation(struct Operation op);

struct DrawHistory dhInit();
void dhAddOp(struct DrawHistory* dh, struct Operation op);
void dhUndo(struct DrawHistory* dh);
void dhRender(struct DrawHistory dh);
