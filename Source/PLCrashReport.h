/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import <Foundation/Foundation.h>
#import "PLCrashReportSystemInfo.h"
#import "PLCrashReportApplicationInfo.h"
#import "PLCrashReportSignalInfo.h"
#import "PLCrashReportThreadInfo.h"
#import "PLCrashReportBinaryImageInfo.h"
#import "PLCrashReportExceptionInfo.h"

/** 
 * @ingroup constants
 * Crash file magic identifier */
#define PLCRASH_REPORT_FILE_MAGIC "plcrash"

/** 
 * @ingroup constants
 * Crash format version byte identifier. Will not change outside of the introduction of
 * an entirely new crash log format. */
#define PLCRASH_REPORT_FILE_VERSION 1

/**
 * @ingroup types
 * Crash log file header format.
 *
 * Crash log files start with 7 byte magic identifier (#PLCRASH_REPORT_FILE_MAGIC),
 * followed by a single unsigned byte version number (#PLCRASH_REPORT_FILE_VERSION).
 * The crash log message format itself is extensible, so this version number will only
 * be incremented in the event of an incompatible encoding or format change.
 */
struct PLCrashReportFileHeader {
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
typedef struct _PLCrashReportDecoder _PLCrashReportDecoder;

@interface PLCrashReport : NSObject {
@private
    /** Private implementation variables (used to hide the underlying protobuf parser) */
    _PLCrashReportDecoder *_decoder;

    /** System info */
    PLCrashReportSystemInfo *_systemInfo;

    /** Application info */
    PLCrashReportApplicationInfo *_applicationInfo;

    /** Signal info */
    PLCrashReportSignalInfo *_signalInfo;

    /** Thread info (PLCrashReportThreadInfo instances) */
    NSArray *_threads;

    /** Binary images (PLCrashReportBinaryImageInfo instances */
    NSArray *_images;

    /** Exception information (may be nil) */
    PLCrashReportExceptionInfo *_exceptionInfo;
}

- (id) initWithData: (NSData *) encodedData error: (NSError **) outError;

- (PLCrashReportBinaryImageInfo *) imageForAddress: (uint64_t) address;

/**
 * System information.
 */
@property(nonatomic, readonly) PLCrashReportSystemInfo *systemInfo;

/**
 * Application information.
 */
@property(nonatomic, readonly) PLCrashReportApplicationInfo *applicationInfo;

/**
 * Signal information. This provides the signal and signal code
 * of the fatal signal.
 */
@property(nonatomic, readonly) PLCrashReportSignalInfo *signalInfo;

/**
 * Thread information. Returns a list of PLCrashReportThreadInfo instances.
 */
@property(nonatomic, readonly) NSArray *threads;

/**
 * Binary image information. Returns a list of PLCrashReportBinaryImageInfo instances.
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
@property(nonatomic, readonly) PLCrashReportExceptionInfo *exceptionInfo;

@end
