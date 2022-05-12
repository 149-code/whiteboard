#define GLT_IMPL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/struct.h>
#include <ct/glt.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmd.h"
#include "config.h"
#include "context.h"
#include "operation.h"

bool nkInput = false;

struct Cursor
{
	vec2s pos;
	bool down;
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

void cursorRender(struct Cursor cursor, float radius, vec4s color)
{
	vec2s mesh[1] = {{cursor.pos.x / SCREEN_WIDTH, 1 - (cursor.pos.y / SCREEN_HEIGHT)}};

	GLTvertexStore vs = gltCreateVertexStore(1);
	gltVertexStoreSetData(vs, 0, sizeof(mesh), mesh, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	glUseProgram(shaderStore.circleShader);
	glUniform1i(glGetUniformLocation(shaderStore.circleShader, "toTex"), false);
	glUniform1f(glGetUniformLocation(shaderStore.circleShader, "radius"), radius);
	glUniform4fv(glGetUniformLocation(shaderStore.circleShader, "color"), 1, &color.x);

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

double deltaWidth(double y, int dx)
{
	double ret;

	if (y >= 0)
		ret = y + dx;
	else
		ret = y + log(y) * dx;

	if (ret > 1)
		return ret;
	else
		return 1;
}

enum DRAW_MODE
{
	DM_BRUSH,
	DM_ERASE,
	DM_CMD,
};

void mod_nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint)
{
	if (nkInput)
		nk_glfw3_char_callback(win, codepoint);
}

int main()
{
	GLFWwindow* window = createWindowAndContext();
	struct nk_context* ctx = nk_glfw3_init(window, NK_GLFW3_DEFAULT);
	glfwSetCharCallback(window, mod_nk_glfw3_char_callback);

	struct nk_font_atlas* fontStash;
	nk_glfw3_font_stash_begin(&fontStash);
	struct nk_font* font = nk_font_atlas_add_default(fontStash, 20, NULL);
	nk_glfw3_font_stash_end();

	nk_init_default(ctx, &font->handle);
	ctx->style.window.padding.x = 0;
	ctx->style.window.padding.y = 0;

	struct DrawHistory dh = dhInit();
	struct Cursor oldCursor = getCursorInfo(window);
	struct Preferences preferences = {
		.color = (vec4s) {1.0, 0.8, 0.8, 1},
		.radius = 0.002,
		.mode = DM_BRUSH,
	};
	char cmdBuffer[256] = {0};
	struct Operation currOp = {.op = OT_NULL};

	bool u_hold = false;
	bool c_hold = false;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		struct Cursor newCursor = getCursorInfo(window);

		if (preferences.mode == DM_CMD)
		{
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				preferences.mode = DM_BRUSH;
				nkInput = false;
			}

			if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
			{
				char* res = execCommand(cmdBuffer, &preferences);

				if (res)
					strcpy(cmdBuffer, res);
				else
					preferences.mode = DM_BRUSH;

				nkInput = false;
			}
		}
		else
		{

			if (glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS &&
				glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
			{
				preferences.mode = DM_CMD;
				nkInput = true;
				cmdBuffer[0] = '\0';
			}

			if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
			{
				if (preferences.mode == DM_BRUSH)
					preferences.radius +=
						0.2 * pow(preferences.radius, 1.1);
				else
					preferences.radius +=
						0.2 * pow(preferences.radius, 1.1);
			}
			if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
			{
				if (preferences.mode == DM_BRUSH)
				{
					if (preferences.radius > 0.002)
						preferences.radius -=
							0.2 * pow(preferences.radius, 1.1);
				}
				else
				{
					if (preferences.radius >= 0.01)
						preferences.radius -=
							0.2 * pow(preferences.radius, 1.1);
				}
			}

			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
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

				struct Operation op = {
					.op = OT_CLEAR, .color = (vec4s){0, 0, 0, 0}};
				dhAddOp(&dh, op);
			}
			if (glfwGetKey(window, GLFW_KEY_C) != GLFW_PRESS)
				c_hold = false;

			if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
				preferences.mode = DM_BRUSH;
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
				preferences.mode = DM_ERASE;

			if (vtLen(dh.operations) > 0)
			{
				if (dh.operations[vtLen(dh.operations) - 1].op == OT_STROKE &&
					glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS)
				{
					dh.operations[vtLen(dh.operations) - 1] = fixOperation(
						dh.operations[vtLen(dh.operations) - 1]);
				}
			}

			if (newCursor.down == true && oldCursor.down == false)
			{
				if (preferences.mode == DM_BRUSH)
				{
					currOp.op = OT_STROKE;
					currOp.stroke_width = preferences.radius;
					currOp.color = preferences.color;
				}
				else if (preferences.mode == DM_ERASE)
				{
					currOp.op = OT_ERASER;
					currOp.stroke_width = preferences.radius;
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
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		dhRender(dh);
		opRender(currOp, 0, false);

		vec2s cursorPos = {
			newCursor.pos.x / SCREEN_WIDTH, 1 - (newCursor.pos.y / SCREEN_HEIGHT)};

		if (preferences.mode == DM_BRUSH)
		{
			struct Operation op = {
				.op = OT_STROKE,
				.stroke_width = preferences.radius,
				.color = preferences.color,
				.points = vtInit(vec2s, 0),
			};

			vtPush(&op.points, cursorPos);
			opRender(op, 0, false);
		}
		else if (preferences.mode == DM_ERASE)
		{
			struct Operation op = {
				.op = OT_HOLLOW_DOT,
				.stroke_width = preferences.radius,
				.color = (vec4s){0, 0, 0, 1},
				.points = vtInit(vec2s, 0),
			};

			vtPush(&op.points, cursorPos);
			opRender(op, 0, false);
		}
		else if (preferences.mode == DM_CMD)
		{
			nk_glfw3_new_frame();

			if (nk_begin(ctx, "",
				    nk_rect(0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, SCREEN_HEIGHT),
				    NK_WINDOW_NO_SCROLLBAR))
			{
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_edit_focus(ctx, NK_EDIT_ALWAYS_INSERT_MODE);
				nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, cmdBuffer,
					sizeof(cmdBuffer) - 1, nk_filter_default);
			}
			nk_end(ctx);

			nk_glfw3_render(NK_ANTI_ALIASING_OFF, 512 * 1024, 128 * 1024);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
		}

		glfwSwapBuffers(window);
	}
}
