#define GLT_IMPL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/struct.h>
#include <ct/glt.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "context.h"
#include "operation.h"
#include "tablet.h"

struct Cursor
{
	vec2s pos;
	bool down;
};

struct BrushContext
{
	float radius;
	vec4s color;
};

struct Preferences
{
	struct BrushContext brush;
	float eraserRadius;
	bool isEraser;
};

struct Cursor getCursorInfo(GLFWwindow* window)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	bool down = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

	return (struct Cursor){
		.pos = {x, y},
		.down = down,
	};
}

void cursorRender(struct Cursor cursor, struct BrushContext brushCtx)
{
	vec2s mesh[1] = {{cursor.pos.x / SCREEN_WIDTH, 1 - (cursor.pos.y / SCREEN_HEIGHT)}};

	GLTvertexStore vs = gltCreateVertexStore(1);
	gltVertexStoreSetData(vs, 0, sizeof(mesh), mesh, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	glUseProgram(shaderStore.circleShader);
	glUniform1i(glGetUniformLocation(shaderStore.circleShader, "toTex"), false);
	glUniform1f(glGetUniformLocation(shaderStore.circleShader, "radius"), brushCtx.radius);
	glUniform4fv(glGetUniformLocation(shaderStore.circleShader, "color"), 1, &brushCtx.color.x);

	glDrawArrays(GL_POINTS, 0, 1);
}

GLFWwindow* createWindowAndContext()
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize glfw\n");
		exit(1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	SCREEN_WIDTH = mode->width;
	SCREEN_HEIGHT = mode->height;

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "App", NULL, NULL);
	glfwMakeContextCurrent(window);

	if (glewInit())
	{
		fprintf(stderr, "Failed to initialize glew\n");
		exit(1);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	loadShaders();
	return window;
}

int main()
{
	init();
	GLFWwindow* window = createWindowAndContext();

	struct DrawHistory dh = dhInit();
	struct Cursor oldCursor = getCursorInfo(window);
	struct Preferences preferences = {
		.brush = (struct BrushContext){.color = {0.2, 0.8, 0.8, 1}, .radius = 0.002},
		.eraserRadius = 0.02,
		.isEraser = false,
	};
	struct Operation currOp = {.op = OT_NULL};

	bool u_hold = false;
	bool c_hold = false;
	bool e_hold = false;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && !u_hold)
		{
			u_hold = true;
			dhUndo(&dh);
		}
		if (glfwGetKey(window, GLFW_KEY_U) != GLFW_PRESS)
			u_hold = false;

		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !c_hold)
		{
			c_hold = true;

			struct Operation op = {.op = OT_CLEAR, .color = (vec4s){0, 0, 0, 0}};
			dhAddOp(&dh, op);
		}
		if (glfwGetKey(window, GLFW_KEY_C) != GLFW_PRESS)
			c_hold = false;

		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !e_hold)
		{
			e_hold = true;
			preferences.isEraser = !preferences.isEraser;
		}
		if (glfwGetKey(window, GLFW_KEY_E) != GLFW_PRESS)
			e_hold = false;

		if (vtLen(dh.operations) > 0)
		{
			if (dh.operations[vtLen(dh.operations) - 1].op == OT_STROKE &&
				glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
			{
				dh.operations[vtLen(dh.operations) - 1] =
					fixOperation(dh.operations[vtLen(dh.operations) - 1]);
			}
		}

		struct Cursor newCursor = getCursorInfo(window);

		if (newCursor.down == true && oldCursor.down == false)
		{
			if (!preferences.isEraser)
			{
				currOp.op = OT_STROKE;
				currOp.stroke_width = preferences.brush.radius;
				currOp.color = preferences.brush.color;
			}
			else
			{
				currOp.op = OT_ERASER;
				currOp.stroke_width = preferences.eraserRadius;
			}

			currOp.points = vtInit(vec2s, 0);
			vec2s newPoint = {newCursor.pos.x / SCREEN_WIDTH,
				1 - (newCursor.pos.y / SCREEN_HEIGHT)};
			vtPush(&currOp.points, newPoint);
		}
		if (newCursor.down == true && oldCursor.down == true)
		{
			vec2s newPoint = {newCursor.pos.x / SCREEN_WIDTH,
				1 - (newCursor.pos.y / SCREEN_HEIGHT)};
			vtPush(&currOp.points, newPoint);
		}
		if (newCursor.down == false && oldCursor.down == true)
		{
			dhAddOp(&dh, currOp);
			currOp = (struct Operation){.op = OT_NULL};
		}

		oldCursor = newCursor;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		dhRender(dh);
		opRender(currOp, 0, false);

		vec2s cursorPos = {
			newCursor.pos.x / SCREEN_WIDTH, 1 - (newCursor.pos.y / SCREEN_HEIGHT)};

		if (!preferences.isEraser)
		{
			struct Operation op = {
				.op = OT_STROKE,
				.stroke_width = preferences.brush.radius,
				.color = preferences.brush.color,
				.points = vtInit(vec2s, 0),
			};

			vtPush(&op.points, cursorPos);
			opRender(op, 0, false);
		}
		else
		{
			struct Operation op = {
				.op = OT_HOLLOW_DOT,
				.stroke_width = preferences.eraserRadius,
				.color = (vec4s){0, 0, 0, 1},
				.points = vtInit(vec2s, 0),
			};

			vtPush(&op.points, cursorPos);
			opRender(op, 0, false);
		}

		glfwSwapBuffers(window);
	}
}
