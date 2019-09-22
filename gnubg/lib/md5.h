/*
 * md5.h
 *
 * Declaration of functions and data types used for MD5 sum computing
 * library functions.
 * Copyright (C) 1995, 1996, 1997, 1999, 2000 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: md5.h,v 1.8 2014/05/11 15:20:32 plm Exp $
 */

/* License changed from the GNU LGPL to the GNU GPL (as permitted
 * under Term 3 of the GNU LGPL) by Gary Wong for distribution
 * with GNU Backgammon. */

#ifndef MD5_H
#define MD5_H 1
#include <glib.h>
#include <stdio.h>
#include "common.h"
typedef guint32 md5_uint32;

/* Structure to save state of computation between the single steps.  */
struct md5_ctx {
    md5_uint32 A;
    md5_uint32 B;
    md5_uint32 C;
    md5_uint32 D;

    md5_uint32 total[2];
    md5_uint32 buflen;
    char buffer[128] __attribute__ ((__aligned__(__alignof__(md5_uint32))));
};

/*
 * The following three functions are build up the low level used in
 * the functions `md5_stream' and `md5_buffer'.
 */

/* Initialize structure containing state of computation.
 * (RFC 1321, 3.3: Step 3)  */
extern void md5_init_ctx(struct md5_ctx *ctx);

/* Starting with the result of former calls of this function (or the
 * initialization function) update the context for the next LEN bytes
 * starting at BUFFER.
 * It is necessary that LEN is a multiple of 64!!! */
extern void md5_process_block(const void *buffer, size_t len, struct md5_ctx *ctx);

/* Starting with the result of former calls of this function (or the
 * initialization function) update the context for the next LEN bytes
 * starting at BUFFER.
 * It is NOT required that LEN is a multiple of 64.  */
extern void md5_process_bytes(const void *buffer, size_t len, struct md5_ctx *ctx);

/* Process the remaining bytes in the buffer and put result from CTX
 * in first 16 bytes following RESBUF.  The result is always in little
 * endian byte order, so that a byte-wise output yields to the wanted
 * ASCII representation of the message digest.
 * 
 * IMPORTANT: On some systems it is required that RESBUF is correctly
 * aligned for a 32 bits value.  */
extern void *md5_finish_ctx(struct md5_ctx *ctx, void *resbuf);


/* Put result from CTX in first 16 bytes following RESBUF.  The result is
 * always in little endian byte order, so that a byte-wise output yields
 * to the wanted ASCII representation of the message digest.
 * 
 * IMPORTANT: On some systems it is required that RESBUF is correctly
 * aligned for a 32 bits value.  */
extern void *md5_read_ctx(const struct md5_ctx *ctx, void *resbuf);


#if 0
/* Compute MD5 message digest for bytes read from STREAM.  The
 * resulting message digest number will be written into the 16 bytes
 * beginning at RESBLOCK.  */
extern int md5_stream(FILE * stream, void *resblock);
#endif

/* Compute MD5 message digest for LEN bytes beginning at BUFFER.  The
 * result is always in little endian byte order, so that a byte-wise
 * output yields to the wanted ASCII representation of the message
 * digest.  */
extern void *md5_buffer(const char *buffer, size_t len, void *resblock);

#endif                          /* md5.h */
