#include "context.h"
#include "operation.h"

#include <math.h>

#define HISTORY_LEN 5

void opRender(struct Operation op, GLuint fbo, bool toTex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	if (op.op == OT_STROKE || op.op == OT_LINE)
	{
		float radius;
		if (toTex)
			radius = op.stroke_width / 2;
		else
			radius = op.stroke_width;

		GLTvertexStore vs = gltCreateVertexStore(1);
		gltVertexStoreSetData(
			vs, 0, sizeof(vec2s) * vtLen(op.points), op.points, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

		glUseProgram(shaderStore.circleShader);
		glUniform1i(glGetUniformLocation(shaderStore.circleShader, "toTex"), toTex);
		glUniform1f(glGetUniformLocation(shaderStore.circleShader, "radius"), radius);
		glUniform1f(glGetUniformLocation(shaderStore.circleShader, "aspectRatio"),
			(float) SCREEN_WIDTH / SCREEN_HEIGHT);
		glUniform4fv(
			glGetUniformLocation(shaderStore.circleShader, "color"), 1, &op.color.x);
		glDrawArrays(GL_POINTS, 0, vtLen(op.points));

		glUseProgram(shaderStore.lineShader);
		glUniform1i(glGetUniformLocation(shaderStore.lineShader, "toTex"), toTex);
		glUniform1f(glGetUniformLocation(shaderStore.lineShader, "radius"), radius);
		glUniform1f(glGetUniformLocation(shaderStore.lineShader, "aspectRatio"),
			(float) SCREEN_WIDTH / SCREEN_HEIGHT);
		glUniform4fv(glGetUniformLocation(shaderStore.lineShader, "color"), 1, &op.color.x);
		glDrawArrays(GL_LINE_STRIP, 0, vtLen(op.points));

		gltDeleteVertexStore(vs);
	}
	else if (op.op == OT_ERASER)
	{
		glDisable(GL_BLEND);

		float radius;
		if (toTex)
			radius = op.stroke_width / 2;
		else
			radius = op.stroke_width;

		vec4s clearColor = {0, 0, 0, 0};

		GLTvertexStore vs = gltCreateVertexStore(1);
		gltVertexStoreSetData(
			vs, 0, sizeof(vec2s) * vtLen(op.points), op.points, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

		glUseProgram(shaderStore.circleShader);
		glUniform1i(glGetUniformLocation(shaderStore.circleShader, "toTex"), toTex);
		glUniform1f(glGetUniformLocation(shaderStore.circleShader, "radius"), radius);
		glUniform1f(glGetUniformLocation(shaderStore.circleShader, "aspectRatio"),
			(float) SCREEN_WIDTH / SCREEN_HEIGHT);
		glUniform4fv(
			glGetUniformLocation(shaderStore.circleShader, "color"), 1, &clearColor.x);
		glDrawArrays(GL_POINTS, 0, vtLen(op.points));

		glUseProgram(shaderStore.lineShader);
		glUniform1i(glGetUniformLocation(shaderStore.lineShader, "toTex"), toTex);
		glUniform1f(glGetUniformLocation(shaderStore.lineShader, "radius"), radius);
		glUniform1f(glGetUniformLocation(shaderStore.lineShader, "aspectRatio"),
			(float) SCREEN_WIDTH / SCREEN_HEIGHT);
		glUniform4fv(
			glGetUniformLocation(shaderStore.lineShader, "color"), 1, &clearColor.x);
		glDrawArrays(GL_LINE_STRIP, 0, vtLen(op.points) - 1);

		glEnable(GL_BLEND);
		gltDeleteVertexStore(vs);
	}
	else if (op.op == OT_HOLLOW_DOT)
	{
		float radius;
		if (toTex)
			radius = op.stroke_width / 2;
		else
			radius = op.stroke_width;

		GLTvertexStore vs = gltCreateVertexStore(1);
		gltVertexStoreSetData(
			vs, 0, sizeof(vec2s) * vtLen(op.points), op.points, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

		glUseProgram(shaderStore.hollowCircleShader);
		glUniform1i(glGetUniformLocation(shaderStore.hollowCircleShader, "toTex"), toTex);
		glUniform1f(glGetUniformLocation(shaderStore.hollowCircleShader, "radius"), radius);
		glUniform1f(glGetUniformLocation(shaderStore.hollowCircleShader, "aspectRatio"),
			(float) SCREEN_WIDTH / SCREEN_HEIGHT);
		glUniform4fv(glGetUniformLocation(shaderStore.hollowCircleShader, "color"), 1,
			&op.color.x);
		glDrawArrays(GL_POINTS, 0, vtLen(op.points));

		gltDeleteVertexStore(vs);
	}
	else if (op.op == OT_CLEAR)
	{
		vec2s mesh[4] = {
			{0, 0},
			{1, 0},
			{0, 1},
			{1, 1},
		};

		glDisable(GL_BLEND);

		GLTvertexStore vs = gltCreateVertexStore(1);
		gltVertexStoreSetData(vs, 0, sizeof(mesh), mesh, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

		glUseProgram(shaderStore.rectShader);
		glUniform4fv(glGetUniformLocation(shaderStore.rectShader, "color"), 1, &op.color.x);
		glUniform1i(glGetUniformLocation(shaderStore.rectShader, "toTex"), toTex);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glEnable(GL_BLEND);

		gltDeleteVertexStore(vs);
	}
}

void opDelete(struct Operation op)
{
	if (op.op == OT_LINE || op.op == OT_TEXT || op.op == OT_STROKE || op.op == OT_ERASER)
		vtFree(op.points);
}

static inline float minf(float x, float y)
{
	if (x > y)
		return y;
	else
		return x;
}

static inline float maxf(float x, float y)
{
	if (x > y)
		return x;
	else
		return y;
}

struct Operation fixOperation(struct Operation op)
{
	struct Operation ret = {.op = OT_LINE};
	ret.stroke_width = op.stroke_width;
	ret.color = op.color;

	float x_min = op.points[0].x;
	float x_max = op.points[0].x;
	float y_min = op.points[0].y;
	float y_max = op.points[0].y;

	float sum_x = 0;
	float sum_y = 0;
	float sum_xy = 0;
	float sum_x2 = 0;
	float sum_y2 = 0;

	for (int i = 0; i < vtLen(op.points); i++)
	{
		x_min = minf(x_min, op.points[i].x);
		x_max = maxf(x_max, op.points[i].x);
		y_min = minf(y_min, op.points[i].y);
		y_max = maxf(y_max, op.points[i].y);

		sum_x += op.points[i].x;
		sum_y += op.points[i].y;
		sum_xy += op.points[i].x * op.points[i].y;
		sum_x2 += op.points[i].x * op.points[i].x;
		sum_y2 += op.points[i].y * op.points[i].y;
	}

	float m = (sum_xy - sum_x * sum_y / vtLen(op.points)) /
		  (sum_x2 - sum_x * sum_x / vtLen(op.points));
	float b = (sum_y - m * sum_x) / vtLen(op.points);

	float r_squared = (vtLen(op.points) * sum_xy - sum_x * sum_y) /
			  sqrt((vtLen(op.points) * sum_x2 - sum_x * sum_x) *
				  (vtLen(op.points) * sum_y2 - sum_y * sum_y));

	float ep1 = (y_min - b) / m;
	float ep2 = (y_max - b) / m;

	x_min = minf(x_min, ep1);
	x_min = minf(x_min, ep2);
	x_max = maxf(x_max, ep1);
	x_max = maxf(x_max, ep2);

	ret.points = vtInit(vec2s, 2);
	ret.points[0] = (vec2s){x_min, x_min * m + b};
	ret.points[1] = (vec2s){x_max, x_max * m + b};

	return ret;
}

void loadShaders()
{
	shaderStore.texShader = gltCreateShader("shaders/tex.vert", "shaders/tex.frag");
	shaderStore.rectShader = gltCreateShader("shaders/rect.vert", "shaders/rect.frag");
	shaderStore.lineShader =
		gltCreateAdvShader("shaders/line.vert", "shaders/line.frag", "shaders/line.geom");
	shaderStore.circleShader = gltCreateAdvShader(
		"shaders/circle.vert", "shaders/circle.frag", "shaders/circle.geom");
	shaderStore.hollowCircleShader = gltCreateAdvShader(
		"shaders/circle.vert", "shaders/hollowCircle.frag", "shaders/circle.geom");
}

struct DrawHistory dhInit()
{
	struct DrawHistory ret;

	ret.operations = vtInit(struct Operation, 0);

	glGenFramebuffers(1, &ret.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, ret.fbo);

	glGenTextures(1, &ret.tex);
	glBindTexture(GL_TEXTURE_2D, ret.tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ret.tex, 0);
	glGenerateMipmap(GL_TEXTURE_2D);

	glClear(GL_COLOR_BUFFER_BIT);

	return ret;
}

void dhAddOp(struct DrawHistory* dh, struct Operation op)
{
	if (vtLen(dh->operations) == HISTORY_LEN)
	{
		opRender(dh->operations[0], dh->fbo, true);
		opDelete(dh->operations[0]);

		for (int i = 0; i < HISTORY_LEN - 1; i++)
		{
			dh->operations[i] = dh->operations[i + 1];
		}

		dh->operations[HISTORY_LEN - 1] = op;
	}
	else
	{
		vtPush(&dh->operations, op);
	}
}

void dhRender(struct DrawHistory dh)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	static vec2s mesh[4] = {
		{0, 0},
		{0, 1},
		{1, 0},
		{1, 1},
	};

	GLTvertexStore vs = gltCreateVertexStore(1);
	gltVertexStoreSetData(vs, 0, sizeof(mesh), mesh, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	glUseProgram(shaderStore.texShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, dh.tex);
	glUniform1i(glGetUniformLocation(shaderStore.texShader, "textureSampler"), 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	gltDeleteVertexStore(vs);

	for (int i = 0; i < vtLen(dh.operations); i++)
	{
		opRender(dh.operations[i], 0, false);
	}
}

void dhUndo(struct DrawHistory* dh)
{
	if (vtLen(dh->operations) >= 1)
	{
		struct Operation op = vtPop(dh->operations);
		opDelete(op);
	}
}
