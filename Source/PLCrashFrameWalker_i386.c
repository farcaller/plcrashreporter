/*
 *  PLCrashFrameWalker_i386.c
 *  CrashReporter
 *
 *  Created by Landon Fuller on 12/8/08.
 *  Copyright 2008 Plausible Labs Cooperative, Inc.. All rights reserved.
 *
 */

#include "PLCrashFrameWalker.h"

#ifdef __i386__

// PLFrameWalker API
plframe_error_t plframe_cursor_init (plframe_cursor_t *cursor, ucontext_t *uap) {
    return PLFRAME_EUNKNOWN;
}


// PLFrameWalker API
plframe_error_t plframe_cursor_next (plframe_cursor_t *cursor) {
    return PLFRAME_EUNKNOWN;
}


// PLFrameWalker API
plframe_error_t plframe_get_reg (plframe_cursor_t *cursor, plframe_regnum_t regnum, plframe_word_t *reg) {
    return PLFRAME_EUNKNOWN;
}


// PLFrameWalker API
plframe_error_t plframe_get_freg (plframe_cursor_t *cursor, plframe_regnum_t regnum, plframe_fpreg_t *fpreg) {
    return PLFRAME_EUNKNOWN;
}

#endif