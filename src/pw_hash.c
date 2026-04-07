/**
 * @file pw_hash.c
 * @brief SHA-256 password hashing — pure C, static allocation only.
 *
 * Implementation follows FIPS PUB 180-4 exactly.  No dynamic memory is used;
 * the SHA-256 context lives on the caller's stack.
 *
 * @req SWR-SEC-004
 */
#include "pw_hash.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * SHA-256 round constants (first 32 bits of the fractional parts of the
 * cube roots of the first 64 primes — FIPS 180-4 §4.2.2)
 * ----------------------------------------------------------------------- */
static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90beffeau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

/* SHA-256 initial hash values (FIPS 180-4 §5.3.3) */
static const uint32_t H0[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

/* -----------------------------------------------------------------------
 * Bit-rotation and SHA-256 logical functions (FIPS 180-4 §4.1.2)
 * ----------------------------------------------------------------------- */
#define ROTR(x, n)  (((x) >> (n)) | ((x) << (32u - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z)(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIG0(x)     (ROTR(x, 2)  ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIG1(x)     (ROTR(x, 6)  ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sig0(x)     (ROTR(x, 7)  ^ ROTR(x, 18) ^ ((x) >> 3))
#define sig1(x)     (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/* -----------------------------------------------------------------------
 * Internal SHA-256 context (lives on the caller's stack)
 * ----------------------------------------------------------------------- */
typedef struct {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t  buf[64];
    uint32_t buflen;
} sha256_ctx;

/* Process one 512-bit (64-byte) message block. */
static void sha256_compress(uint32_t state[8], const uint8_t block[64])
{
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h, T1, T2;
    int i;

    /* Prepare message schedule W */
    for (i = 0; i < 16; ++i) {
        W[i] = ((uint32_t)block[i * 4    ] << 24)
             | ((uint32_t)block[i * 4 + 1] << 16)
             | ((uint32_t)block[i * 4 + 2] <<  8)
             | ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; ++i) {
        W[i] = sig1(W[i - 2]) + W[i - 7] + sig0(W[i - 15]) + W[i - 16];
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (i = 0; i < 64; ++i) {
        T1 = h + SIG1(e) + CH(e, f, g) + K[i] + W[i];
        T2 = SIG0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256_init(sha256_ctx *ctx)
{
    memcpy(ctx->state, H0, sizeof(H0));
    ctx->bitcount = 0u;
    ctx->buflen   = 0u;
}

static void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        ctx->buf[ctx->buflen++] = data[i];
        ctx->bitcount += 8u;
        if (ctx->buflen == 64u) {
            sha256_compress(ctx->state, ctx->buf);
            ctx->buflen = 0u;
        }
    }
}

static void sha256_final(sha256_ctx *ctx, uint8_t digest[32])
{
    uint64_t bc = ctx->bitcount;
    int i;

    /* Append 0x80 padding byte */
    ctx->buf[ctx->buflen++] = 0x80u;

    /* If no room for the 8-byte length, compress the current block first */
    if (ctx->buflen > 56u) {
        while (ctx->buflen < 64u) ctx->buf[ctx->buflen++] = 0x00u;
        sha256_compress(ctx->state, ctx->buf);
        ctx->buflen = 0u;
    }

    /* Pad to 56 bytes, then append big-endian 64-bit bit count */
    while (ctx->buflen < 56u) ctx->buf[ctx->buflen++] = 0x00u;
    ctx->buf[56] = (uint8_t)(bc >> 56);
    ctx->buf[57] = (uint8_t)(bc >> 48);
    ctx->buf[58] = (uint8_t)(bc >> 40);
    ctx->buf[59] = (uint8_t)(bc >> 32);
    ctx->buf[60] = (uint8_t)(bc >> 24);
    ctx->buf[61] = (uint8_t)(bc >> 16);
    ctx->buf[62] = (uint8_t)(bc >>  8);
    ctx->buf[63] = (uint8_t)(bc      );
    sha256_compress(ctx->state, ctx->buf);

    /* Serialise state to big-endian byte digest */
    for (i = 0; i < 8; ++i) {
        digest[i * 4    ] = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >>  8);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i]      );
    }
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
void pw_hash(char out[PW_HASH_HEX_LEN], const char *plaintext)
{
    sha256_ctx  ctx;
    uint8_t     digest[32];
    int         i;
    static const char hex[] = "0123456789abcdef";

    if (!out) return;
    out[0] = '\0';
    if (!plaintext) return;

    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)plaintext, strlen(plaintext));
    sha256_final(&ctx, digest);

    for (i = 0; i < 32; ++i) {
        out[i * 2    ] = hex[(digest[i] >> 4) & 0x0fu];
        out[i * 2 + 1] = hex[ digest[i]       & 0x0fu];
    }
    out[64] = '\0';
}
