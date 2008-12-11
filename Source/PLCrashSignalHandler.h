/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import <Foundation/Foundation.h>

#import <mach/mach.h>
#import <mach/exception.h>


@interface PLCrashSignalHandler : NSObject {
@private
    /** Signal stack */
    stack_t _sigstk;
}

+ (PLCrashSignalHandler *) sharedHandler;
- (BOOL) registerHandlerAndReturnError: (NSError **) outError;

- (void) testHandlerWithSignal: (int) signal code: (int) code faultAddress: (void *) address;

@end
