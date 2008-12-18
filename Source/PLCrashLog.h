/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import <Foundation/Foundation.h>
#import "PLCrashLogSystemInfo.h"
#import "PLCrashLogApplicationInfo.h"
#import "PLCrashLogSignalInfo.h"
#import "PLCrashLogThreadInfo.h"
#import "PLCrashLogBinaryImageInfo.h"
#import "PLCrashLogExceptionInfo.h"

/** 
 * @ingroup constants
 * Crash file magic identifier */
#define PLCRASH_LOG_FILE_MAGIC "plcrash"

/** 
 * @ingroup constants
 * Crash format version byte identifier. Will not change outside of the introduction of
 * an entirely new crash log format. */
#define PLCRASH_LOG_FILE_VERSION 1

/**
 * @ingroup types
 * Crash log file header format.
 *
 * Crash log files start with 7 byte magic identifier (#PLCRASH_LOG_FILE_MAGIC),
 * followed by a single unsigned byte version number (#PLCRASH_LOG_FILE_VERSION).
 * The crash log message format itself is extensible, so this version number will only
 * be incremented in the event of an incompatible encoding or format change.
 */
struct PLCrashLogFileHeader {
    /** Crash log magic identifier, not NULL terminated */
    const char magic[7];

    /** Crash log encoding/format version */
    const uint8_t version;

    /** File data */
    const uint8_t data[];
} __attribute__((packed));


/**
 * @internal
 * Private decoder instance variables (used to hide the underlying protobuf parser).
 */
typedef struct _PLCrashLogDecoder _PLCrashLogDecoder;

@interface PLCrashLog : NSObject {
@private
    /** Private implementation variables (used to hide the underlying protobuf parser) */
    _PLCrashLogDecoder *_decoder;

    /** System info */
    PLCrashLogSystemInfo *_systemInfo;

    /** Application info */
    PLCrashLogApplicationInfo *_applicationInfo;

    /** Signal info */
    PLCrashLogSignalInfo *_signalInfo;

    /** Thread info (PLCrashLogThreadInfo instances) */
    NSArray *_threads;

    /** Binary images (PLCrashLogBinaryImageInfo instances */
    NSArray *_images;

    /** Exception information (may be nil) */
    PLCrashLogExceptionInfo *_exceptionInfo;
}

- (id) initWithData: (NSData *) encodedData error: (NSError **) outError;

- (PLCrashLogBinaryImageInfo *) imageForAddress: (uint64_t) address;

/**
 * System information.
 */
@property(nonatomic, readonly) PLCrashLogSystemInfo *systemInfo;

/**
 * Application information.
 */
@property(nonatomic, readonly) PLCrashLogApplicationInfo *applicationInfo;

/**
 * Signal information. This provides the signal and signal code
 * of the fatal signal.
 */
@property(nonatomic, readonly) PLCrashLogSignalInfo *signalInfo;

/**
 * Thread information. Returns a list of PLCrashLogThreadInfo instances.
 */
@property(nonatomic, readonly) NSArray *threads;

/**
 * Binary image information. Returns a list of PLCrashLogBinaryImageInfo instances.
 */
@property(nonatomic, readonly) NSArray *images;

/**
 * YES if exception information is available.
 */
@property(nonatomic, readonly) BOOL hasExceptionInfo;

/**
 * Exception information. Only available if a crash was caused by an uncaught exception,
 * otherwise nil.
 */
@property(nonatomic, readonly) PLCrashLogExceptionInfo *exceptionInfo;

@end
