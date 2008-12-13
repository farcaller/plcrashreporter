/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import "GTMSenTestCase.h"

#import "PLCrashLogWriter.h"
#import "PLCrashFrameWalker.h"

#import <sys/stat.h>
#import <sys/mman.h>
#import <fcntl.h>
#import <sys/utsname.h>

#import "crash_report.pb-c.h"

@interface PLCrashLogWriterTests : SenTestCase {
@private
    /* Path to crash log */
    NSString *_logPath;
    
    /* Test thread */
    plframe_test_thead_t _thr_args;
}

@end


@implementation PLCrashLogWriterTests

- (void) setUp {
    /* Create a temporary log path */
    _logPath = [[NSTemporaryDirectory() stringByAppendingString: [[NSProcessInfo processInfo] globallyUniqueString]] retain];
    
    /* Create the test thread */
    plframe_test_thread_spawn(&_thr_args);
}

- (void) tearDown {
    //NSError *error;
    
    /* Delete the file */
    //STAssertTrue([[NSFileManager defaultManager] removeItemAtPath: _logPath error: &error], @"Could not remove log file");
    NSLog(@"Not deleting file at path %@", _logPath);
    [_logPath release];

    /* Stop the test thread */
    plframe_test_thread_stop(&_thr_args);
}

// check a crash report's system info
- (void) checkSystemInfo: (Plcrash__CrashReport *) crashReport {
    Plcrash__CrashReport__SystemInfo *systemInfo = crashReport->system_info;
    struct utsname uts;
    uname(&uts);

    STAssertNotNULL(systemInfo, @"No system info available");
    // Nothing else to do?
    if (systemInfo == NULL)
        return;

    STAssertEquals(systemInfo->operating_system, PLCRASH_OS, @"Unexpected OS value");
    
    STAssertNotNULL(systemInfo->os_version, @"No OS version encoded");
    if (systemInfo->os_version != NULL)
        STAssertTrue(strcmp(systemInfo->os_version, uts.release) == 0, @"Unexpected system version");

    STAssertEquals(systemInfo->machine_type, PLCRASH_MACHINE, @"Unexpected machine type");

    STAssertTrue(systemInfo->timestamp != 0, @"Timestamp uninitialized");
}

- (void) checkBacktraces: (Plcrash__CrashReport *) crashReport {
    Plcrash__CrashReport__Backtrace **bts = crashReport->backtraces;

    STAssertNotNULL(bts, @"No thread messages were written");
    STAssertTrue(crashReport->n_backtraces > 0, @"0 thread messages were written");

    NSLog(@"Number of backtraces: %d", crashReport->n_backtraces);
    for (int i = 0; i < crashReport->n_backtraces; i++) {
        Plcrash__CrashReport__Backtrace *bt = bts[i];

        NSLog(@"\nBacktrace for thread ID: %d", bt->thread_number);
        
        for (int j = 0; j < bt->n_frames; j++) {
            Plcrash__CrashReport__Backtrace__StackFrame *f = bt->frames[j];
            NSLog(@"Frame at pc %lx", f->pc);
            if (f->nearest_symbol_name != NULL)
                NSLog(@"Frame symbol %lx %p", f->pc, f->nearest_symbol_name);
        }
    }
    // TODO
}

- (void) testWriteReport {
    siginfo_t info;
    plframe_cursor_t cursor;
    plcrash_writer_t writer;
    plasync_file_t file;

    /* Initialze faux crash data */
    {
        info.si_addr = 0x0;
        info.si_errno = 0;
        info.si_pid = getpid();
        info.si_uid = getuid();
        info.si_code = SEGV_MAPERR;
        info.si_signo = SIGSEGV;
        info.si_status = 0;
        
        /* Steal the test thread's stack for iteration */
        plframe_cursor_thread_init(&cursor, pthread_mach_thread_np(_thr_args.thread));
    }

    /* Open the output file */
    int fd = open([_logPath UTF8String], O_RDWR|O_CREAT|O_EXCL, 0644);
    plasync_file_init(&file, fd);

    /* Initialize a writer */
    STAssertEquals(PLCRASH_ESUCCESS, plcrash_writer_init(&writer), @"Initialization failed");

    /* Write the crash report */
    STAssertEquals(PLCRASH_ESUCCESS, plcrash_writer_report(&writer, &file, &info, cursor.uap), @"Crash log failed");

    /* Close it */
    plcrash_writer_close(&writer);

    /* Flush the output */
    plasync_file_flush(&file);

    /* Try reading it back in */
    void *buf;
    struct stat statbuf;
    {
        STAssertEquals(0, stat([_logPath UTF8String], &statbuf), @"fstat failed");
        
        buf = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        STAssertNotNULL(buf, @"Could not map pages");
    }

    /* Try to read the crash report */
    Plcrash__CrashReport *crashReport;
    crashReport = plcrash__crash_report__unpack(&protobuf_c_system_allocator, statbuf.st_size, buf);
    
    /* Output some debug decoding (TODO: Remove) */
    fprintf(stderr, "Binary dump: ");
    for (size_t i = 0; i < statbuf.st_size; i++) {
        fprintf(stderr, "%.2hhx", ((uint8_t *) buf)[i]);
    }
    fprintf(stderr, "\n");
    
    /* If reading the report didn't fail, test the contents */
    STAssertNotNULL(crashReport, @"Could not decode crash report");
    if (crashReport != NULL) {
        /* Test the report */
        [self checkSystemInfo: crashReport];
        [self checkBacktraces: crashReport];

        /* Free it */
        protobuf_c_message_free_unpacked((ProtobufCMessage *) crashReport, &protobuf_c_system_allocator);
    }


    STAssertEquals(0, munmap(buf, statbuf.st_size), @"Could not unmap pages: %s", strerror(errno));
    plasync_file_close(&file);
}

@end
