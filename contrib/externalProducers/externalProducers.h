/*
 * Copyright (c) Yann Collet, Meta Platforms, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 *
 * Hardware interface for ZSTD accelerators
 * Author: Bulent Abali <abali@us.ibm.com>
 *
 */

#ifndef EXTERNAL_PRODUCERS_H
#define EXTERNAL_PRODUCERS_H

#include <stdint.h>
#include <endian.h>

#ifdef XXH_PRIVATE_API
#include <xxhash.h>
#define XXHASH_H_INCLUDED
#endif

/*
 * An external xxhash provider accelerates the ZSTD streaming API function
 * XXH64_update (XXH64_state_t* statePtr, const void* input, size_t length)
 * (in zstd/lib/common/xxhash.h).
 *
 * Hashing may be performed by the External Sequence Provider
 * since the input stream is already available to the provider.
 *
 * The hw_xxh64_state_t layout is identical to XXH64_state_t.  Caller
 * provides the initial state, input and length to the external
 * provider. The provider computes the hash of input and returns the
 * updated state in the same state struct.
 *
 * Main value of an external xxhash provider is eliminating processor
 * cycles consumed by the hashing functions.
 *
 * Related XXH64 functions XXH64_reset, XXH64_createState,
 * XXH64_freestate, and XXH64_digest are expected to be performed in
 * software as they have small and constant overheads relative to the
 * XXH6_update overhead which is proportional to the input size.
 *
 * Endianness of hw_xxh64_state_t members are same as that of the host.
 */

typedef struct {
    uint64_t total_len;    /*!< Total length hashed. This is always 64-bit. */
    uint64_t v[4];         /*!< Accumulator lanes */
    uint64_t mem64[4];     /*!< Internal buffer for partial reads. Treated as unsigned char[32]. */
    uint32_t memsize;      /*!< Amount of data in @ref mem64 */
    uint32_t reserved32;   /*!< Reserved field, needed for padding anyways*/
    uint64_t reserved64;   /*!< Reserved field. Do not read or write to it. */
} hw_xxh64_state_t;


/*
 * An external histogram provider accelerates collection of ZSTD
 * sequence statistics. Frequency of literals, literals_length_codes,
 * match_length_codes, and offset_codes are returned in struct
 * histogram_t.  The histogram_t is expected to be used by ZSTD while
 * building Huffman code tables and FSE code tables.
 *
 * Main value of an externally provided histogram_t is eliminating one
 * pass of memory read over the input stream of sequences and the
 * table lookup and arithmetic cost while counting the sequence
 * symbols.
 *
 * Sequence statistics may be collected by the External Sequence
 * Provider since it outputs the sequences.
 *
 *  Endianness of histogram_t are same as that of the host.
 */

typedef struct {
    uint32_t literal[256];
    /* count of literals 0x00 through 0xFF */

    uint32_t literals_length_code[36];
    /* https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md#literals-length-codes */

    uint32_t match_length_code[53];
    /* https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md#match-length-codes */

    uint32_t offset_code [32];
    /* https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md#offset-codes */
} histogram_t __attribute__ ((aligned (8)));


/*
 * The histogram output for sequences and the input/output XXH64 state
 * of the input stream is exchanged with the accelerator via
 * sequence_provider_parameters_t.
 *
 * The caller supplies the initial XXH64_state_t in addition to the
 * input stream and length.  The external sequence provider returns
 * the ZSTD sequences and optionally returns the updated
 * XXH_64_state_t and the sequence statistics in histogram_t.
 */

#define XXHASH_REQUEST      0x00000001
#define XXHASH_AVAILABLE    0x00000002
#define HISTOGRAM_REQUEST   0x00000004
#define HISTOGRAM_AVAILABLE 0x00000008

typedef struct {
    uint32_t request_response;
    union {
#ifdef XXHASH_H_INCLUDED
	XXH64_state_t      zstd_xxh64_state;
#endif
	hw_xxh64_state_t   hw_xxh64_state;
    };

    histogram_t histogram;
} sequence_provider_parameters_t;

#endif /* EXTERNAL_PRODUCERS_H */
