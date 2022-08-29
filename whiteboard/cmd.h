#pragma once

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "operation.h"

char* execCommand(char* cmd, struct Preferences* preferences, struct DrawHistory* dh);
