/*
 * Author: Landon Fuller <landonf@plausiblelabs.com>
 *
 * Copyright (c) 2008 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 */

#import "PLCrashLog.h"
#import "CrashReporter.h"

#import "crash_report.pb-c.h"

struct _PLCrashLogDecoder {
    Plcrash__CrashReport *crashReport;
};

#define IMAGE_UUID_DIGEST_LEN 16

@interface PLCrashLog (PrivateMethods)

- (Plcrash__CrashReport *) decodeCrashData: (NSData *) data error: (NSError **) outError;
- (PLCrashLogSystemInfo *) extractSystemInfo: (Plcrash__CrashReport__SystemInfo *) systemInfo error: (NSError **) outError;
- (PLCrashLogApplicationInfo *) extractApplicationInfo: (Plcrash__CrashReport__ApplicationInfo *) applicationInfo error: (NSError **) outError;
- (NSArray *) extractThreadInfo: (Plcrash__CrashReport *) crashReport error: (NSError **) outError;
- (NSArray *) extractImageInfo: (Plcrash__CrashReport *) crashReport error: (NSError **) outError;
- (PLCrashLogApplicationInfo *) extractExceptionInfo: (Plcrash__CrashReport__Exception *) exceptionInfo error: (NSError **) outError;
- (PLCrashLogSignalInfo *) extractSignalInfo: (Plcrash__CrashReport__Signal *) signalInfo error: (NSError **) outError;

@end


static void populate_nserror (NSError **error, PLCrashReporterError code, NSString *description);

/**
 * Provides decoding of crash logs generated by the PLCrashReporter framework.
 *
 * @warning This API should be considered in-development and subject to change.
 */
@implementation PLCrashLog

/**
 * Initialize with the provided crash log data. On error, nil will be returned, and
 * an NSError instance will be provided via @a error, if non-NULL.
 *
 * @param encodedData Encoded plcrash crash log.
 * @param outError If an error occurs, this pointer will contain an NSError object
 * indicating why the crash log could not be parsed. If no error occurs, this parameter
 * will be left unmodified. You may specify NULL for this parameter, and no error information
 * will be provided.
 *
 * @par Designated Initializer
 * This method is the designated initializer for the PLCrashLog class.
 */
- (id) initWithData: (NSData *) encodedData error: (NSError **) outError {
    if ((self = [super init]) == nil) {
        // This shouldn't happen, but we have to fufill our API contract
        populate_nserror(outError, PLCrashReporterErrorUnknown, @"Could not initialize superclass");
        return nil;
    }


    /* Allocate the struct and attempt to parse */
    _decoder = malloc(sizeof(_PLCrashLogDecoder));
    _decoder->crashReport = [self decodeCrashData: encodedData error: outError];

    /* Check if decoding failed. If so, outError has already been populated. */
    if (_decoder->crashReport == NULL) {
        goto error;
    }


    /* System info */
    _systemInfo = [[self extractSystemInfo: _decoder->crashReport->system_info error: outError] retain];
    if (!_systemInfo)
        goto error;

    /* Application info */
    _applicationInfo = [[self extractApplicationInfo: _decoder->crashReport->application_info error: outError] retain];
    if (!_applicationInfo)
        goto error;

    /* Signal info */
    _signalInfo = [[self extractSignalInfo: _decoder->crashReport->signal error: outError] retain];
    if (!_signalInfo)
        goto error;

    /* Thread info */
    _threads = [[self extractThreadInfo: _decoder->crashReport error: outError] retain];
    if (!_threads)
        goto error;

    /* Image info */
    _images = [[self extractImageInfo: _decoder->crashReport error: outError] retain];
    if (!_images)
        goto error;

    /* Exception info, if it is available */
    if (_decoder->crashReport->exception != NULL) {
        _exceptionInfo = [[self extractExceptionInfo: _decoder->crashReport->exception error: outError] retain];
        if (!_exceptionInfo)
            goto error;
    }

    return self;

error:
    [self release];
    return nil;
}

- (void) dealloc {
    /* Free the data objects */
    [_systemInfo release];
    [_applicationInfo release];
    [_threads release];
    [_images release];
    [_exceptionInfo release];

    /* Free the decoder state */
    if (_decoder != NULL) {
        if (_decoder->crashReport != NULL) {
            protobuf_c_message_free_unpacked((ProtobufCMessage *) _decoder->crashReport, &protobuf_c_system_allocator);
        }

        free(_decoder);
        _decoder = NULL;
    }

    [super dealloc];
}

/**
 * Return the binary image containing the given address, or nil if no binary image
 * is found.
 */
- (PLCrashLogBinaryImageInfo *) imageForAddress: (uint64_t) address {
    for (PLCrashLogBinaryImageInfo *imageInfo in self.images) {
        if (imageInfo.imageBaseAddress <= address && address < (imageInfo.imageBaseAddress + imageInfo.imageSize))
            return imageInfo;
    }

    /* Not found */
    return nil;
}

// property getter. property is documented.
// Returns YES if exception information is available.
- (BOOL) hasExceptionInfo {
    if (_exceptionInfo != nil)
        return YES;
    return NO;
}

@synthesize systemInfo = _systemInfo;
@synthesize applicationInfo = _applicationInfo;
@synthesize signalInfo = _signalInfo;
@synthesize threads = _threads;
@synthesize images = _images;
@synthesize exceptionInfo = _exceptionInfo;

@end


/**
 * @internal
 * Private Methods
 */
@implementation PLCrashLog (PrivateMethods)

/**
 * Decode the crash log message.
 *
 * @warning MEMORY WARNING. The caller is responsible for deallocating th ePlcrash__CrashReport instance
 * returned by this method via protobuf_c_message_free_unpacked().
 */
- (Plcrash__CrashReport *) decodeCrashData: (NSData *) data error: (NSError **) outError {
    const struct PLCrashLogFileHeader *header;
    const void *bytes;

    bytes = [data bytes];
    header = bytes;

    /* Verify that the crash log is sufficently large */
    if (sizeof(struct PLCrashLogFileHeader) >= [data length]) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, NSLocalizedString(@"Could not decode truncated crash log",
                                                                                             @"Crash log decoding error message"));
        return NULL;
    }

    /* Check the file magic */
    if (memcmp(header->magic, PLCRASH_LOG_FILE_MAGIC, strlen(PLCRASH_LOG_FILE_MAGIC)) != 0) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid,NSLocalizedString(@"Could not decode invalid crash log header",
                                                                                            @"Crash log decoding error message"));
        return NULL;
    }

    /* Check the version */
    if(header->version != PLCRASH_LOG_FILE_VERSION) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, [NSString stringWithFormat: NSLocalizedString(@"Could not decode unsupported crash report version: %d", 
                                                                                                                         @"Crash log decoding message"), header->version]);
        return NULL;
    }

    Plcrash__CrashReport *crashReport = plcrash__crash_report__unpack(&protobuf_c_system_allocator, [data length] - sizeof(struct PLCrashLogFileHeader), header->data);
    if (crashReport == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, NSLocalizedString(@"An unknown error occured decoding the crash report", 
                                                                                             @"Crash log decoding error message"));
        return NULL;
    }

    return crashReport;
}


/**
 * Extract system information from the crash log. Returns nil on error.
 */
- (PLCrashLogSystemInfo *) extractSystemInfo: (Plcrash__CrashReport__SystemInfo *) systemInfo error: (NSError **) outError {
    NSDate *timestamp = nil;
    
    /* Validate */
    if (systemInfo == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing System Information section", 
                                           @"Missing sysinfo in crash report"));
        return nil;
    }
    
    if (systemInfo->os_version == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing System Information OS version field", 
                                           @"Missing sysinfo operating system in crash report"));
        return nil;
    }
    
    /* Set up the timestamp, if available */
    if (systemInfo->timestamp != 0)
        timestamp = [NSDate dateWithTimeIntervalSince1970: systemInfo->timestamp];
    
    /* Done */
    return [[[PLCrashLogSystemInfo alloc] initWithOperatingSystem: systemInfo->operating_system
                                           operatingSystemVersion: [NSString stringWithUTF8String: systemInfo->os_version]
                                                     architecture: systemInfo->architecture
                                                        timestamp: timestamp] autorelease];
}


/**
 * Extract application information from the crash log. Returns nil on error.
 */
- (PLCrashLogApplicationInfo *) extractApplicationInfo: (Plcrash__CrashReport__ApplicationInfo *) applicationInfo 
                                                 error: (NSError **) outError
{    
    /* Validate */
    if (applicationInfo == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing Application Information section", 
                                           @"Missing appinfo in crash report"));
        return nil;
    }

    /* Identifier available? */
    if (applicationInfo->identifier == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing Application Information app identifier field", 
                                           @"Missing appinfo operating system in crash report"));
        return nil;
    }

    /* Version available? */
    if (applicationInfo->version == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing Application Information app version field", 
                                           @"Missing appinfo operating system in crash report"));
        return nil;
    }
    
    /* Done */
    NSString *identifier = [NSString stringWithUTF8String: applicationInfo->identifier];
    NSString *version = [NSString stringWithUTF8String: applicationInfo->version];

    return [[[PLCrashLogApplicationInfo alloc] initWithApplicationIdentifier: identifier
                                                          applicationVersion: version] autorelease];
}

/**
 * Extract thread information from the crash log. Returns nil on error, or an array of PLCrashLogThreadInfo
 * instances on success.
 */
- (NSArray *) extractThreadInfo: (Plcrash__CrashReport *) crashReport error: (NSError **) outError {
    /* There should be at least one thread */
    if (crashReport->n_threads == 0) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid,
                         NSLocalizedString(@"Crash report is missing thread state information",
                                           @"Missing thread info in crash report"));
        return nil;
    }

    /* Handle all threads */
    NSMutableArray *threadResult = [NSMutableArray arrayWithCapacity: crashReport->n_threads];
    for (size_t thr_idx = 0; thr_idx < crashReport->n_threads; thr_idx++) {
        Plcrash__CrashReport__Thread *thread = crashReport->threads[thr_idx];
        
        /* Fetch stack frames for this thread */
        NSMutableArray *frames = [NSMutableArray arrayWithCapacity: thread->n_frames];
        for (size_t frame_idx = 0; frame_idx < thread->n_frames; frame_idx++) {
            Plcrash__CrashReport__Thread__StackFrame *frame = thread->frames[frame_idx];
            PLCrashLogStackFrameInfo *frameInfo;

            frameInfo = [[[PLCrashLogStackFrameInfo alloc] initWithInstructionPointer: frame->pc] autorelease];
            [frames addObject: frameInfo];
        }

        /* Fetch registers for this thread */
        NSMutableArray *registers = [NSMutableArray arrayWithCapacity: thread->n_registers];
        for (size_t reg_idx = 0; reg_idx < thread->n_registers; reg_idx++) {
            Plcrash__CrashReport__Thread__RegisterValue *reg = thread->registers[reg_idx];
            PLCrashLogRegisterInfo *regInfo;

            /* Handle missing register name (should not occur!) */
            if (reg->name == NULL) {
                populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, @"Missing register name in register value");
                return nil;
            }

            regInfo = [[[PLCrashLogRegisterInfo alloc] initWithRegisterName: [NSString stringWithUTF8String: reg->name]
                                                              registerValue: reg->value] autorelease];
            [registers addObject: regInfo];
        }

        /* Create the thread info instance */
        PLCrashLogThreadInfo *threadInfo = [[[PLCrashLogThreadInfo alloc] initWithThreadNumber: thread->thread_number
                                                                                   stackFrames: frames 
                                                                                       crashed: thread->crashed 
                                                                                     registers: registers] autorelease];
        [threadResult addObject: threadInfo];
    }
    
    return threadResult;
}


/**
 * Extract binary image information from the crash log. Returns nil on error.
 */
- (NSArray *) extractImageInfo: (Plcrash__CrashReport *) crashReport error: (NSError **) outError {
    /* There should be at least one image */
    if (crashReport->n_binary_images == 0) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid,
                         NSLocalizedString(@"Crash report is missing binary image information",
                                           @"Missing image info in crash report"));
        return nil;
    }

    /* Handle all records */
    NSMutableArray *images = [NSMutableArray arrayWithCapacity: crashReport->n_binary_images];
    for (size_t i = 0; i < crashReport->n_binary_images; i++) {
        Plcrash__CrashReport__BinaryImage *image = crashReport->binary_images[i];
        PLCrashLogBinaryImageInfo *imageInfo;

        /* Validate */
        if (image->name == NULL) {
            populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, @"Missing image name in image record");
            return nil;
        }

        /* Convert UUID to hex string */
        NSString *uuid = nil;
        if (image->uuid.len == 0) {
            /* No UUID */
            uuid = nil;
        } else if (image->uuid.len != IMAGE_UUID_DIGEST_LEN) {
            /* Invalid UUID */
            populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, @"Invalid image binary UUID length");
            return nil;
        } else if (image->uuid.len != 0) {
            /* Valid UUID */
    
            /* Convert to ascii */
            char output[(IMAGE_UUID_DIGEST_LEN * 2) + 1];
            const char hex[] = "0123456789abcdef";

            for (int i = 0; i < IMAGE_UUID_DIGEST_LEN; i++) {
                unsigned char c = ((unsigned char *) image->uuid.data)[i];
                output[i * 2 + 0] = hex[c >> 4];
                output[i * 2 + 1] = hex[c & 0x0F];
            }
            output[sizeof(output)] = '\0';
    
            uuid = [NSString stringWithCString: output length: sizeof(output) - 1];
        }

        assert(image->uuid.len == 0 || uuid != nil);
        imageInfo = [[[PLCrashLogBinaryImageInfo alloc] initWithImageBaseAddress: image->base_address 
                                                                       imageSize: image->size 
                                                                       imageName: [NSString stringWithUTF8String: image->name]
                                                                       imageUUID: uuid] autorelease];
        [images addObject: imageInfo];
    }

    return images;
}

/**
 * Extract  exception information from the crash log. Returns nil on error.
 */
- (PLCrashLogApplicationInfo *) extractExceptionInfo: (Plcrash__CrashReport__Exception *) exceptionInfo
                                               error: (NSError **) outError
{
    /* Validate */
    if (exceptionInfo == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing Exception Information section", 
                                           @"Missing appinfo in crash report"));
        return nil;
    }
    
    /* Name available? */
    if (exceptionInfo->name == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing exception name field", 
                                           @"Missing appinfo operating system in crash report"));
        return nil;
    }
    
    /* Reason available? */
    if (exceptionInfo->reason == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing exception reason field", 
                                           @"Missing appinfo operating system in crash report"));
        return nil;
    }
    
    /* Done */
    NSString *name = [NSString stringWithUTF8String: exceptionInfo->name];
    NSString *reason = [NSString stringWithUTF8String: exceptionInfo->reason];
    
    return [[[PLCrashLogExceptionInfo alloc] initWithExceptionName: name reason: reason] autorelease];
}

/**
 * Extract signal information from the crash log. Returns nil on error.
 */
- (PLCrashLogSignalInfo *) extractSignalInfo: (Plcrash__CrashReport__Signal *) signalInfo
                                       error: (NSError **) outError
{
    /* Validate */
    if (signalInfo == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing Signal Information section", 
                                           @"Missing appinfo in crash report"));
        return nil;
    }
    
    /* Name available? */
    if (signalInfo->name == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing signal name field", 
                                           @"Missing appinfo operating system in crash report"));
        return nil;
    }
    
    /* Code available? */
    if (signalInfo->code == NULL) {
        populate_nserror(outError, PLCrashReporterErrorCrashReportInvalid, 
                         NSLocalizedString(@"Crash report is missing signal code field", 
                                           @"Missing appinfo operating system in crash report"));
        return nil;
    }
    
    /* Done */
    NSString *name = [NSString stringWithUTF8String: signalInfo->name];
    NSString *code = [NSString stringWithUTF8String: signalInfo->code];
    
    return [[[PLCrashLogSignalInfo alloc] initWithSignalName: name code: code address: signalInfo->address] autorelease];
}

@end

/**
 * @internal
 
 * Populate an NSError instance with the provided information.
 *
 * @param error Error instance to populate. If NULL, this method returns
 * and nothing is modified.
 * @param code The error code corresponding to this error.
 * @param description A localized error description.
 * @param cause The underlying cause, if any. May be nil.
 */
static void populate_nserror (NSError **error, PLCrashReporterError code, NSString *description) {
    NSMutableDictionary *userInfo;
    
    if (error == NULL)
        return;
    
    /* Create the userInfo dictionary */
    userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
                description, NSLocalizedDescriptionKey,
                nil
                ];
    
    *error = [NSError errorWithDomain: PLCrashReporterErrorDomain code: code userInfo: userInfo];
}