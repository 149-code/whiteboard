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

float calculateRSquared(vtVec(vec2s) points)
{
	float sum_x = 0;
	float sum_y = 0;
	float sum_xy = 0;
	float sum_x2 = 0;
	float sum_y2 = 0;

	for (int i = 0; i < vtLen(points); i++)
	{
		sum_x += points[i].x;
		sum_y += points[i].y;
		sum_xy += points[i].x * points[i].y;
		sum_x2 += points[i].x * points[i].x;
		sum_y2 += points[i].y * points[i].y;
	}

	float m = (sum_xy - sum_x * sum_y / vtLen(points)) /
		  (sum_x2 - sum_x * sum_x / vtLen(points));
	float b = (sum_y - m * sum_x) / vtLen(points);

	float r_squared = (vtLen(points) * sum_xy - sum_x * sum_y) /
			  sqrt((vtLen(points) * sum_x2 - sum_x * sum_x) *
				  (vtLen(points) * sum_y2 - sum_y * sum_y));

	return r_squared;
}

struct Operation fixOperation(struct Operation op)
{
	float line_r_square = calculateRSquared(op.points);

	if (fabs(line_r_square) > 0.8)
	{
		struct Operation ret = {.op = OT_LINE};
		ret.stroke_width = op.stroke_width;
		ret.color = op.color;
		ret.points = vtInit(vec2s, 2);
		ret.points[0] = op.points[0];
		ret.points[1] = op.points[vtLen(op.points) - 1];

		return ret;
	}

	return op;
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

/*
DrawHistory
Header(16): [MagicNumber(u32), Version(u32), Width(u32), Height(u32)]
Image(Width * Height * rgba)
NumOps(u32)
Op(NumOps)

Op
Header(4): [OpType(u32)]
Radius(u32)
Color(rgba)

DataSize(u32)
Data(u8 * DataSize)
*/

void opSerialise(struct Operation op, FILE* fp)
{
	fwrite(&op.op, sizeof(int), 1, fp);
	fwrite(&op.stroke_width, sizeof(int), 1, fp);
	fwrite(&op.color, sizeof(float), 4, fp);
	vtSerialise(op.points, fp);
}


void dhSerialise(struct DrawHistory dh, FILE* fp)
{
	unsigned int header[4] = {0x12345678, VERSION, SCREEN_WIDTH, SCREEN_HEIGHT};
	unsigned char* data = malloc(4 * SCREEN_WIDTH * SCREEN_HEIGHT);

	glBindTexture(GL_TEXTURE_2D, dh.tex);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	fwrite(header, sizeof(int), 4, fp);
	fwrite(data, sizeof(char), 4 * SCREEN_WIDTH * SCREEN_HEIGHT, fp);

	int numOps = vtLen(dh.operations);
	fwrite(&numOps, sizeof(int), 1, fp);

	for (int i = 0; i < numOps; i++)
		opSerialise(dh.operations[i], fp);
}

struct Operation opDeserialise(FILE* fp)
{
	struct Operation ret;

	fread(&ret.op, sizeof(int), 1, fp);
	fread(&ret.stroke_width, sizeof(int), 1, fp);
	fread(&ret.color, sizeof(float), 4, fp);

	if (ret.op == OT_TEXT)
		ret.text = vtDeserialise(char, fp);
	else
		ret.points = vtDeserialise(vec2s, fp);

	return ret;
}

int dhLoadDeserialise(FILE* fp, struct DrawHistory* dh)
{
	unsigned int header[4];
	fread(header, sizeof(int), 4, fp);

	if (header[0] != 0x12345678)
		return -1;
	if (header[2] != SCREEN_WIDTH || header[3] != SCREEN_HEIGHT)
		return -2;

	unsigned char* image = malloc(4 * header[2] * header[3]);
	fread(image, sizeof(char), 4 * SCREEN_WIDTH * SCREEN_HEIGHT, fp);
	glBindTexture(GL_TEXTURE_2D, dh->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, image);

	int numOps;
	fread(&numOps, sizeof(int), 1, fp);

	if (dh->operations)
		vtFree(dh->operations);

	dh->operations = vtInit(struct Operation, numOps);
	for (int i = 0; i < numOps; i++)
		dh->operations[i] = opDeserialise(fp);

	return 0;
}
