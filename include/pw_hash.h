/**
 * @file pw_hash.h
 * @brief Password hashing via SHA-256 (pure C, static allocation only).
 *
 * Passwords are never stored or compared in plaintext.  Every credential
 * is transformed to a 64-character lowercase hex digest before storage
 * or comparison.
 *
 * Algorithm : SHA-256 (FIPS PUB 180-4)
 * Allocation: 100 % stack / static — no heap allocation
 * Standard  : IEC 62304 Class B / OWASP password storage guidance
 *
 * @req SWR-SEC-004
 */
#ifndef PW_HASH_H
#define PW_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

/** Length of a SHA-256 hex digest string including the NUL terminator. */
#define PW_HASH_HEX_LEN  65   /* 64 hex chars + '\0' */

/**
 * @brief Hash a plaintext password to a 64-char lowercase hex SHA-256 digest.
 *
 * @param out      Caller-supplied buffer of at least PW_HASH_HEX_LEN bytes.
 * @param plaintext NUL-terminated password string to hash.
 *
 * Both @p out and @p plaintext must be non-NULL.  If either is NULL the
 * function writes an empty string to @p out (if @p out is non-NULL) and
 * returns immediately.
 */
void pw_hash(char out[PW_HASH_HEX_LEN], const char *plaintext);

#ifdef __cplusplus
}
#endif

#endif /* PW_HASH_H */
