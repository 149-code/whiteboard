#pragma once

#include <cglm/struct.h>

struct BrushContext
{
	float radius;
	vec4s color;
};

struct Preferences
{
	struct BrushContext brush;
	float eraserRadius;
	int mode;
};

