//======================================================================
//
// blake2s.c
// ---------
//
// A simple BLAKE2s reference implementation.
//
// See LICENSE for license terms.
// See README.md in the repo root for info about source code origin.
//======================================================================

#include <stdint.h>
#include "blake2s.h"

#define VERBOSE 0
#define SHOW_V 0
#define SHOW_M_WORDS 0

#if VERBOSE || SHOW_V || SHOW_M_WORDS
#include <stdio.h>
#endif

// Cyclic right rotation.
#ifndef ROTR32
#define ROTR32(x, y)  (((x) >> (y)) ^ ((x) << (32 - (y))))
#endif


// Little-endian byte access.
#define B2S_GET32(p)                            \
    (((uint32_t) ((uint8_t *) (p))[0]) ^        \
    (((uint32_t) ((uint8_t *) (p))[1]) << 8) ^  \
    (((uint32_t) ((uint8_t *) (p))[2]) << 16) ^ \
    (((uint32_t) ((uint8_t *) (p))[3]) << 24))


// Initialization Vector.
static const uint32_t blake2s_iv[8] = {
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};


#if VERBOSE || SHOW_V
//------------------------------------------------------------------
//------------------------------------------------------------------
void print_v(uint32_t *v) {
  printf("0x%08x, 0x%08x, 0x%08x, 0x%08x\n", v[0],  v[1],  v[2],  v[3]);
  printf("0x%08x, 0x%08x, 0x%08x, 0x%08x\n", v[4],  v[5],  v[6],  v[7]);
  printf("0x%08x, 0x%08x, 0x%08x, 0x%08x\n", v[8],  v[9],  v[10], v[11]);
  printf("0x%08x, 0x%08x, 0x%08x, 0x%08x\n", v[12], v[13], v[14], v[15]);
  printf("\n");
}


//------------------------------------------------------------------
// print_ctx()
// Print the contents of the context data structure.
//------------------------------------------------------------------
void print_ctx(blake2s_ctx *ctx) {
  printf("Chained state (h):\n");
  printf("0x%08x, 0x%08x, 0x%08x, 0x%08x, ",
         ctx->h[0], ctx->h[1], ctx->h[2], ctx->h[3]);
  printf("0x%08x, 0x%08x, 0x%08x, 0x%08x",
         ctx->h[4], ctx->h[5], ctx->h[6], ctx->h[7]);
  printf("\n");

  printf("Byte counter (t):\n");
  printf("0x%08x, 0x%08x", ctx->t[0], ctx->t[1]);
  printf("\n");

  printf("\n");
}

#endif

//------------------------------------------------------------------
// B2S_G macro redefined as a G function.
// Allows us to output intermediate values for debugging.
//------------------------------------------------------------------
void G(uint32_t *v, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t y) {
#if VERBOSE
    printf("G started.\n");
#endif

#if SHOW_V
    printf("v before processing:\n");
    print_v(&v[0]);
#endif

#if SHOW_M_WORDS
    printf("x: 0x%08x, y: 0x%08x\n", x, y);
#endif

  v[a] = v[a] + v[b] + x;
  v[d] = ROTR32(v[d] ^ v[a], 16);
  v[c] = v[c] + v[d];
  v[b] = ROTR32(v[b] ^ v[c], 12);
  v[a] = v[a] + v[b] + y;
  v[d] = ROTR32(v[d] ^ v[a], 8);
  v[c] = v[c] + v[d];
  v[b] = ROTR32(v[b] ^ v[c], 7);

#if SHOW_V
    printf("v after processing:\n");
    print_v(&v[0]);
#endif

#if VERBOSE
    printf("G completed.\n\n");
#endif
}


//------------------------------------------------------------------
// Compression function. "last" flag indicates last block.
//------------------------------------------------------------------
static void blake2s_compress(blake2s_ctx *ctx, int last)
{
    const uint8_t sigma[10][16] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
        {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
        {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
        {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
        {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
        {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
        {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
        {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
        {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}
    };

    int i;
    uint32_t v[16], m[16];

#if VERBOSE
      printf("blake2s_compress started.\n");
#endif

    // init work variables
    for (i = 0; i < 8; i++) {
        v[i] = ctx->h[i];
        v[i + 8] = blake2s_iv[i];
    }

    // low 32 bits of offset
    // high 32 bits
#if VERBOSE
      printf("t[0]: 0x%08x, t[1]: 0x%08x\n", ctx->t[0], ctx->t[1]);
#endif
    v[12] ^= ctx->t[0];
    v[13] ^= ctx->t[1];

    // last block flag set ?
    if (last) {
        v[14] = ~v[14];
    }

    // get little-endian words
    for (i = 0; i < 16; i++) {
        m[i] = B2S_GET32(&ctx->b[4 * i]);
    }

#if VERBOSE
      printf("v before G processing:\n");
      print_v(&v[0]);
#endif

    // Ten rounds of the G function applied on rows, diagonal.
    for (i = 0; i < 10; i++) {
#if VERBOSE
        printf("Round %02d:\n", (i + 1));
        printf("Row processing started.\n");
#endif

      G(&v[0], 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
      G(&v[0], 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
      G(&v[0], 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
      G(&v[0], 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);

#if VERBOSE
        printf("Row processing completed.\n");
        printf("Diagonal processing started.\n");
#endif

      G(&v[0], 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
      G(&v[0], 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
      G(&v[0], 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
      G(&v[0], 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);

#if VERBOSE
        printf("Diagonal processing completed.\n");
        printf("\n");
#endif
    }

#if VERBOSE
      printf("v after G processing:\n");
      print_v(&v[0]);
#endif

    // Update the hash state.
    for (i = 0; i < 8; ++i) {
      ctx->h[i] ^= v[i] ^ v[i + 8];
    }

#if VERBOSE
      printf("blake2s_compress completed.\n");
#endif
}


//------------------------------------------------------------------
// Initialize the hashing context "ctx" with optional key "key".
//      1 <= outlen <= 32 gives the digest size in bytes.
//      Secret key (also <= 32 bytes) is optional (keylen = 0).
//------------------------------------------------------------------
int blake2s_init(blake2s_ctx *ctx, size_t outlen,
    const void *key, size_t keylen)     // (keylen=0: no key)
{
    size_t i;

#if VERBOSE
      printf("blake2s_init started.\n");
      printf("Context before blake2s_init processing:\n");
      print_ctx(ctx);
#endif

    if (outlen == 0 || outlen > 32 || keylen > 32)
        return -1;                      // illegal parameters

    for (i = 0; i < 8; i++)             // state, "param block"
        ctx->h[i] = blake2s_iv[i];
    ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

    ctx->t[0] = 0;                      // input count low word
    ctx->t[1] = 0;                      // input count high word
    ctx->c = 0;                         // pointer within buffer
    ctx->outlen = outlen;

    for (i = keylen; i < 64; i++)       // zero input block
        ctx->b[i] = 0;
    if (keylen > 0) {
        blake2s_update(ctx, key, keylen);
        ctx->c = 64;                    // at the end
    }

#if VERBOSE
      printf("Context after blake2s_init processing:\n");
      print_ctx(ctx);
      printf("blake2s_init completed.\n");
#endif

    return 0;
}


//------------------------------------------------------------------
// Add "inlen" bytes from "in" into the hash.
//------------------------------------------------------------------
void blake2s_update(blake2s_ctx *ctx,
    const void *in, size_t inlen)       // data bytes
{
    size_t i;

#if VERBOSE
      printf("blake2s_update started.\n");
      printf("Context before blake2s_update processing:\n");
      print_ctx(ctx);
#endif

    for (i = 0; i < inlen; i++) {
        if (ctx->c == 64) {             // buffer full ?
            ctx->t[0] += ctx->c;        // add counters
            if (ctx->t[0] < ctx->c)     // carry overflow ?
                ctx->t[1]++;            // high word
            blake2s_compress(ctx, 0);   // compress (not last)
            ctx->c = 0;                 // counter to zero
        }
        ctx->b[ctx->c++] = ((const uint8_t *) in)[i];
    }

#if VERBOSE
      printf("Context after blake2s_update processing:\n");
      print_ctx(ctx);
      printf("blake2s_update completed.\n");
#endif
}


//------------------------------------------------------------------
// Generate the message digest (size given in init).
//      Result placed in "out".
//------------------------------------------------------------------
void blake2s_final(blake2s_ctx *ctx, void *out)
{
    size_t i;

#if VERBOSE
     printf("blake2s_final started.\n");
     printf("Context before blake2s_final processing:\n");
     print_ctx(ctx);
#endif

    ctx->t[0] += ctx->c;                // mark last block offset

    // carry overflow
    // high word
    if (ctx->t[0] < ctx->c) {
        ctx->t[1]++;
    }

    // fill up with zeros
    // final block flag = 1
    while (ctx->c < 64) {
        ctx->b[ctx->c++] = 0;
    }
    blake2s_compress(ctx, 1);

    // little endian convert and store
    for (i = 0; i < ctx->outlen; i++) {
        ((uint8_t *) out)[i] =
            (ctx->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
    }

#if VERBOSE
     printf("Context after blake2s_final processing:\n");
     print_ctx(ctx);
     printf("blake2s_final completed.\n");
#endif
}


//------------------------------------------------------------------
// Convenience function for all-in-one computation.
//------------------------------------------------------------------
int blake2s(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen)
{
    blake2s_ctx ctx;

    if (blake2s_init(&ctx, outlen, key, keylen))
        return -1;

    blake2s_update(&ctx, in, inlen);

    blake2s_final(&ctx, out);

    return 0;
}

//======================================================================
//======================================================================
