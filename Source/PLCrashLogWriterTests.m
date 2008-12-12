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
    NSError *error;
    
    /* Delete the file */
    STAssertTrue([[NSFileManager defaultManager] removeItemAtPath: _logPath error: &error], @"Could not remove log file");
    [_logPath release];

    /* Stop the test thread */
    plframe_test_thread_stop(&_thr_args);
}

// check a crash report's system info
- (void) checkSystemInfo: (Plcrash__CrashReport__SystemInfo *) systemInfo {
    struct utsname uts;
    uname(&uts);

    STAssertNotNULL(systemInfo, @"No system info available");
    
    STAssertEquals(systemInfo->operating_system, PLCRASH_OS, @"Unexpected OS value");
    STAssertTrue(strcmp(systemInfo->os_version, uts.release) == 0, @"Unexpected system version");

    STAssertEquals(systemInfo->machine_type, PLCRASH_MACHINE, @"Unexpected machine type");

    STAssertTrue(systemInfo->timestamp != 0, @"Timestamp uninitialized");
}

- (void) testWriteReport {
    siginfo_t info;
    plframe_cursor_t cursor;
    plcrash_writer_t writer;

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

    /* Initialize a writer */
    STAssertEquals(PLCRASH_ESUCCESS, plcrash_writer_init(&writer, [_logPath UTF8String]), @"Initialization failed");

    /* Write the crash report */
    STAssertEquals(PLCRASH_ESUCCESS, plcrash_writer_report(&writer, &info, cursor.uap), @"Crash log failed");

    /* Close it */
    plcrash_writer_close(&writer);

    /* Try reading it back in */
    int infd;
    void *buf;
    struct stat statbuf;
    {
        STAssertEquals(0, stat([_logPath UTF8String], &statbuf), @"fstat failed");
    
        infd = open([_logPath UTF8String], O_RDONLY);
        STAssertGreaterThan(infd, 0, @"Could not open log file");
        
        buf = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, infd, 0);
        STAssertNotNULL(buf, @"Could not map pages");
    }

    /* Try to read the crash report */
    Plcrash__CrashReport *crashReport;
    crashReport = plcrash__crash_report__unpack(&protobuf_c_system_allocator, statbuf.st_size, buf);
    STAssertNotNULL(crashReport, @"Could not decode crash report");

    if (crashReport != NULL) {
        /* Test the report */
        [self checkSystemInfo: crashReport->system_info];

        /* Free it */
        protobuf_c_message_free_unpacked((ProtobufCMessage *) crashReport, &protobuf_c_system_allocator);
    }

    close(infd);
    STAssertEquals(0, munmap(buf, statbuf.st_size), @"Could not unmap pages: %s", strerror(errno));
}

@end
