#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <flecs.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#if !defined(ECS_TARGET_WINDOWS) && !defined(ECS_TARGET_EM) && !defined(ECS_TARGET_ANDROID)
#include <execinfo.h>
#define ECS_BT_BUF_SIZE 100
static void dump_backtrace(FILE *stream) {
    int    nptrs;
    void  *buffer[ECS_BT_BUF_SIZE];
    char **strings;

    nptrs = backtrace(buffer, ECS_BT_BUF_SIZE);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        return;
    }

    for (int j = 3; j < nptrs; j++) {
        fprintf(stream, "%s\n", strings[j]);
    }

    free(strings);
}
#else
static void dump_backtrace(FILE *stream) { (void)stream; }
#endif

static void custom_log(int32_t level, const char *file, int32_t line, const char *msg) {
    FILE *stream;
    if (level >= 0) {
        stream = stdout;
    } else {
        stream = stderr;
    }

    bool use_colors = ecs_os_api.flags_ & EcsOsApiLogWithColors;
    bool timestamp  = ecs_os_api.flags_ & EcsOsApiLogWithTimeStamp;
    bool deltatime  = ecs_os_api.flags_ & EcsOsApiLogWithTimeDelta;

    time_t now = 0;

    if (deltatime) {
        now          = time(NULL);
        time_t delta = 0;
        if (ecs_os_api.log_last_timestamp_) {
            delta = now - ecs_os_api.log_last_timestamp_;
        }
        ecs_os_api.log_last_timestamp_ = (int64_t)now;

        if (delta) {
            if (delta < 10) {
                fputs(" ", stream);
            }
            if (delta < 100) {
                fputs(" ", stream);
            }
            char time_buf[20];
            ecs_os_sprintf(time_buf, "%u", (uint32_t)delta);
            fputs("+", stream);
            fputs(time_buf, stream);
            fputs(" ", stream);
        } else {
            fputs("     ", stream);
        }
    }

    if (timestamp) {
        if (!now) {
            now = time(NULL);
        }
        char time_buf[20];
        ecs_os_sprintf(time_buf, "%u", (uint32_t)now);
        fputs(time_buf, stream);
        fputs(" ", stream);
    }

    if (level >= 0) {
        if (level == 0) {
            if (use_colors)
                fputs(ECS_MAGENTA, stream);
        } else {
            if (use_colors)
                fputs(ECS_GREY, stream);
        }
        fputs("info", stream);
    } else if (level == -2) {
        if (use_colors)
            fputs(ECS_YELLOW, stream);
        fputs("warning", stream);
    } else if (level == -3) {
        if (use_colors)
            fputs(ECS_RED, stream);
        fputs("error", stream);
    } else if (level == -4) {
        if (use_colors)
            fputs(ECS_RED, stream);
        fputs("fatal", stream);
    }

    if (use_colors)
        fputs(ECS_NORMAL, stream);
    fputs(": ", stream);

    if (level >= 0) {
        if (ecs_os_api.log_indent_) {
            char indent[32];
            int  i, indent_count = ecs_os_api.log_indent_;
            if (indent_count > 15)
                indent_count = 15;

            for (i = 0; i < indent_count; i++) {
                indent[i * 2]     = '|';
                indent[i * 2 + 1] = ' ';
            }

            if (ecs_os_api.log_indent_ != indent_count) {
                indent[i * 2 - 2] = '+';
            }

            indent[i * 2] = '\0';

            fputs(indent, stream);
        }
    }

    if (level < 0) {
        if (file) {
            const char *file_ptr = strrchr(file, '/');
            if (!file_ptr) {
                file_ptr = strrchr(file, '\\');
            }

            if (file_ptr) {
                file = file_ptr + 1;
            }

            fputs(file, stream);
            fputs(": ", stream);
        }

        if (line) {
            fprintf(stream, "%d: ", line);
        }
    }

    fputs(msg, stream);

    fputs("\n", stream);

    if (level == -4) {
        dump_backtrace(stream);
    }
}

#endif