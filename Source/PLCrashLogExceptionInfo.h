/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import <Foundation/Foundation.h>


@interface PLCrashLogExceptionInfo : NSObject {
@private
    /** Name */
    NSString *_name;

    /** Reason */
    NSString *_reason;
}

- (id) initWithName: (NSString *) name reason: (NSString *) reason;

@property(nonatomic, readonly) NSString *name;
@property(nonatomic, readonly) NSString *reason;

@end
