/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */


#import <stdio.h> // for snprintf
#import <unistd.h>
#import <stdbool.h>

// Debug output support. Lines are capped at 1024
#define PLCF_DEBUG(msg, args...) {\
    char output[1024];\
    snprintf(output, sizeof(output), "[PLCrashReport] " msg "\n", ## args); \
    write(STDERR_FILENO, output, strlen(output));\
}


/**
 * @internal
 * @ingroup plcrash_async_bufio
 *
 * Async-safe buffered file output. This implementation is only intended for use
 * within signal handler execution of crash log output.
 */
typedef struct plasync_file {
    int fd;
    size_t buflen;
    char buffer[256];
} plasync_file_t;


void plasync_file_init (plasync_file_t *file, int fd);
bool plasync_file_write (plasync_file_t *file, const void *data, size_t len);
bool plasync_file_flush (plasync_file_t *file);
bool plasync_file_close (plasync_file_t *file);
