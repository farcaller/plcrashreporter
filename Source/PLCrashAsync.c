/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import "PLCrashAsync.h"

#import <stdint.h>
#import <errno.h>
#import <string.h>

/**
 * @internal
 * @defgroup plcrash_async Async Safe Utilities
 * @ingroup plcrash_internal
 *
 * Implements async-safe utility functions
 *
 * @{
 */

/**
 * Return an error description for the given plcrash_error_t.
 */
const char *plcrash_strerror (plcrash_error_t error) {
    switch (error) {
        case PLCRASH_ESUCCESS:
            return "No error";
        case PLCRASH_EUNKNOWN:
            return "Unknown error";
        case PLCRASH_OUTPUT_ERR:
            return "Output file can not be opened (or written to)";
        case PLCRASH_ENOMEM:
            return "No memory available";
        case PLCRASH_ENOTSUP:
            return "Operation not supported";
        case PLCRASH_EINVAL:
            return "Invalid argument";
        case PLCRASH_EINTERNAL:
            return "Internal error";
    }
    
    /* Should be unreachable */
    return "Unhandled error code";
}

/**
 * @internal
 * @ingroup plcrash_async
 * @defgroup plcrash_async_bufio Async-safe Buffered IO
 * @{
 */

/**
 * 
 * Write len bytes to fd, looping until all bytes are written
 * or an error occurs. For the local file system, only one call to write()
 * should be necessary
 */
static ssize_t writen (int fd, const void *data, size_t len) {
    const void *p;
    size_t left;
    ssize_t written;
    
    /* Loop until all bytes are written */
    p = data;
    left = len;
    while (left > 0) {
        if ((written = write(fd, p, left)) <= 0) {
            if (errno == EINTR) {
                // Try again
                written = 0;
            } else {
                PLCF_DEBUG("Error occured writing to crash log: %s", strerror(errno));
                return -1;
            }
        }
        
        left -= written;
        p += written;
    }
    
    return written;
}


/**
 * Initialize the plasync_file_t instance.
 *
 * @param file File structure to initialize.
 * @param fd Open file descriptor.
 */
void plasync_file_init (plasync_file_t *file, int fd) {
    file->fd = fd;
    file->buflen = 0;
}


/**
 * Write all bytes from @a data to the file buffer. Returns true on success,
 * or false if an error occurs.
 */
bool plasync_file_write (plasync_file_t *file, const void *data, size_t len) {
    /* Check if the buffer will fill */
    if (file->buflen + len > sizeof(file->buffer)) {
        /* Flush the buffer */
        if (writen(file->fd, file->buffer, file->buflen) < 0) {
            return false;
        }
        
        file->buflen = 0;
    }
    
    /* Check if the new data fits within the buffer, if so, buffer it */
    if (len + file->buflen < sizeof(file->buffer)) {
        memcpy(file->buffer + file->buflen, data, len);
        file->buflen += len;
        
        return true;
        
    } else {
        /* Won't fit in the buffer, just write it */
        if (writen(file->fd, data, len) < 0) {
            return false;
        }
        
        return true;
    } 
}


/**
 * Flush all buffered bytes from the file buffer.
 */
bool plasync_file_flush (plasync_file_t *file) {
    /* Anything to do? */
    if (file->buflen == 0)
        return true;
    
    /* Write remaining */
    if (writen(file->fd, file->buffer, file->buflen) < 0)
        return false;
    
    return true;
}


/**
 * Close the backing file descriptor.
 */
bool plasync_file_close (plasync_file_t *file) {
    if (close(file->fd) != 0) {
        PLCF_DEBUG("Error closing file: %s", strerror(errno));
        return false;
    }

    return true;
}

/**
 * @} plcrash_async_bufio
 */

/**
 * @} plcrash_async
 */