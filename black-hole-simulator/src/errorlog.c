#include "errorlog.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

static FILE *g_log = NULL;
static int g_log_failed = 0;

static void vlog(const char *fmt, va_list ap) {
    if (!g_log && !g_log_failed) {
        g_log = fopen("error.log", "w");
        if (!g_log) g_log_failed = 1;   /* e.g. read-only folder: keep going without a file */
        else {
            time_t t = time(NULL);
            char stamp[64];
            strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", localtime(&t));
            fprintf(g_log, "Black Hole Simulator - error log (%s)\n", stamp);
        }
    }
    char line[2048];
    vsnprintf(line, sizeof line, fmt, ap);
    if (g_log) { fprintf(g_log, "%s\n", line); fflush(g_log); }
    fprintf(stderr, "%s\n", line);
}

void bhs_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
}

void bhs_fatal(const char *title, const char *fmt, ...) {
    char msg[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    bhs_log("FATAL: %s - %s", title, msg);
#ifdef _WIN32
    char full[2300];
    snprintf(full, sizeof full, "%s\n\nA technical log was saved to the file error.log in the simulator folder.", msg);
    MessageBoxA(NULL, full, title, MB_OK | MB_ICONERROR);
#endif
}

#ifdef _WIN32
static LONG WINAPI crash_handler(EXCEPTION_POINTERS *ep) {
    bhs_log("CRASH: unexpected error code 0x%08lX at address %p",
            (unsigned long)ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
    MessageBoxA(NULL,
        "The simulator crashed unexpectedly.\n\nTry updating your graphics driver. "
        "A technical log was saved to error.log in the simulator folder.",
        "Black Hole Simulator", MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void bhs_platform_init(void) {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        char *slash = strrchr(path, '\\');
        if (slash) { *slash = '\0'; SetCurrentDirectoryA(path); }
    }
    SetProcessDPIAware();
    SetUnhandledExceptionFilter(crash_handler);
#elif defined(__linux__)
    char path[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", path, sizeof path - 1);
    if (n > 0) {
        path[n] = '\0';
        char *slash = strrchr(path, '/');
        if (slash) { *slash = '\0'; if (chdir(path) != 0) { /* keep current folder */ } }
    }
#endif
}
