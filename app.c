#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "app.h"

bool cntr_pressed = false;
pthread_t thread_id;
pid_t child_proc;
int fds[2];

CGEventRef handler(CGEventTapProxy proxy, CGEventType event_type, CGEventRef event, void* userInfo)
{
	CGKeyCode keycode = (CGKeyCode) CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
	if (event_type == kCGEventFlagsChanged)
	{
		if (keycode == kVK_Control)
			cntr_pressed = !cntr_pressed;
	}

	if (event_type == kCGEventKeyDown && keycode == kVK_ANSI_T && cntr_pressed)
	{
		char c = 'h';
		write(fds[1], &c, 1);
		return NULL;
	}

	return event;
}

int start_listener()
{
	CGEventMask event_mask =
	    (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) | (1 << kCGEventFlagsChanged);

	CFMachPortRef eventTap =
	    CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
			     event_mask, handler, NULL);

	if (!eventTap)
	{
		fprintf(stderr, "ERROR, insuficient permisions");
		return 1;
	}

	CFRunLoopSourceRef runLoopSource =
	    CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
	CFRunLoopRun();

	return 0;
}

void* sketch_thread_handler(void* arg)
{
        int ret = start_listener();
        return NULL;
}

int start_sketch_deamon()
{
	pipe(fds);

	child_proc = fork();
	if (child_proc == -1)
		return -1;
	else if (child_proc == 0)
	{
		dup2(fds[0], 0);
		close(fds[1]);

		char* args[] = {NULL};
		execv("./whiteboard/a.out", args);
	}

	close(fds[0]);

        int err = pthread_create(&thread_id, NULL, sketch_thread_handler, NULL);
        if (err != 0) {
		return err;
        }

	return 0;
}

void stop_sketch_deamon()
{
	char c = 'q';
	write(fds[1], &c, 1);
        pthread_cancel(thread_id);
}
