#include "../core.h"
#include <stdlib.h>
#include <stdio.h>

static void LogInternal(int logLevel, FORMAT_STRING format, va_list args)
{
	double t = GetTime();
	int us = (int)(t * 1e6) % 1000;
	int ms = (int)(t * 1e3) % 1000;
	int sec = (int)(t) % 60;
	int min = (int)(t / 60) % 60;
	int hour = (int)(t / 3600);

	char buffer[4096];
	vsnprintf(buffer, sizeof buffer, format, args);
	TraceLog(logLevel, "[%02d:%02d:%02d.%03d'%03d] %s", hour, min, sec, ms, us, buffer);
}

void LogInfo(FORMAT_STRING message, ...)
{
	va_list args;
	va_start(args, message);
	LogInternal(LOG_INFO, message, args);
	va_end(args);
}

void LogWarning(FORMAT_STRING message, ...)
{
	va_list args;
	va_start(args, message);
	LogInternal(LOG_WARNING, message, args);
	va_end(args);
}

void LogError(FORMAT_STRING message, ...)
{
	va_list args;
	va_start(args, message);
	LogInternal(LOG_ERROR, message, args);
	va_end(args);
}

void Crash(FORMAT_STRING message, ...)
{
	__debugbreak(); // Drop into debugger if possible, so we can see what happened from the call stack.
	va_list args;
	va_start(args, message);
	LogInternal(LOG_FATAL, message, args);
	va_end(args); // Technically not necessary since LOG_FATAL will exit anyway.
}