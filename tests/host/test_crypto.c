#include "unity.h"

#include "crypto.h"
#include "hexutil.h"
#include "sha256.h"

#include <stdio.h>
#include <string.h>

#define VECTORS_FILE "tests/vectors/verify_cases.txt"

static const char *sample_pubkey_hex =
    "60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6"
    "7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299";
static const char *sample_digest_hex =
    "af2bdbe1aa9b6ec1e2ade1d694f41fc71a831d0268e9891562113d8a62add1bf";
static const char *sample_sig_hex =
    "EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716"
    "F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8";
static const char *test_digest_hex =
    "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08";
static const char *test_sig_hex =
    "F1ABB023518351CD71D881567B1EA663ED3EFCF6C5132B354F28D3B0B7D38367"
    "019F4113742A2B14BD25926B49C649155F267E60D3814B4C0CC84250E46F0083";

void crypto_test_set_up(void) {}

static size_t decode_field(const char *hex, uint8_t *out, size_t cap)
{
    size_t n = hex_decode(hex, out, cap);
    return n;
}

static crypto_status_t verify_pair(const char *pubkey_hex, const char *digest_hex,
                                   const char *sig_hex)
{
    uint8_t pubkey[CRYPTO_PUBKEY_LEN];
    uint8_t digest[CRYPTO_DIGEST_LEN];
    uint8_t signature[CRYPTO_SIGNATURE_LEN];

    TEST_ASSERT_EQUAL_UINT32(CRYPTO_PUBKEY_LEN,
                             decode_field(pubkey_hex, pubkey, sizeof(pubkey)));
    TEST_ASSERT_EQUAL_UINT32(CRYPTO_DIGEST_LEN,
                             decode_field(digest_hex, digest, sizeof(digest)));
    TEST_ASSERT_EQUAL_UINT32(CRYPTO_SIGNATURE_LEN,
                             decode_field(sig_hex, signature, sizeof(signature)));

    return crypto_verify_signature(pubkey, digest, signature);
}

static void test_rfc6979_sample_vector_verifies(void)
{
    TEST_ASSERT_EQUAL(CRYPTO_OK, verify_pair(sample_pubkey_hex, sample_digest_hex, sample_sig_hex));
}

static void test_rfc6979_test_vector_verifies(void)
{
    TEST_ASSERT_EQUAL(CRYPTO_OK, verify_pair(sample_pubkey_hex, test_digest_hex, test_sig_hex));
}

static void test_digest_from_our_sha256_verifies(void)
{
    uint8_t pubkey[CRYPTO_PUBKEY_LEN];
    uint8_t signature[CRYPTO_SIGNATURE_LEN];
    uint8_t digest[SHA256_DIGEST_LEN];

    decode_field(sample_pubkey_hex, pubkey, sizeof(pubkey));
    decode_field(sample_sig_hex, signature, sizeof(signature));
    sha256((const uint8_t *)"sample", 6, digest);

    TEST_ASSERT_EQUAL(CRYPTO_OK, crypto_verify_signature(pubkey, digest, signature));
}

static void test_single_bit_digest_tamper_rejected(void)
{
    uint8_t pubkey[CRYPTO_PUBKEY_LEN];
    uint8_t digest[CRYPTO_DIGEST_LEN];
    uint8_t signature[CRYPTO_SIGNATURE_LEN];

    decode_field(sample_pubkey_hex, pubkey, sizeof(pubkey));
    decode_field(sample_digest_hex, digest, sizeof(digest));
    decode_field(sample_sig_hex, signature, sizeof(signature));
    digest[0] ^= 0x01u;

    TEST_ASSERT_EQUAL(CRYPTO_ERR_SIGNATURE, crypto_verify_signature(pubkey, digest, signature));
}

static void test_single_bit_signature_tamper_rejected(void)
{
    uint8_t pubkey[CRYPTO_PUBKEY_LEN];
    uint8_t digest[CRYPTO_DIGEST_LEN];
    uint8_t signature[CRYPTO_SIGNATURE_LEN];

    decode_field(sample_pubkey_hex, pubkey, sizeof(pubkey));
    decode_field(sample_digest_hex, digest, sizeof(digest));
    decode_field(sample_sig_hex, signature, sizeof(signature));
    signature[0] ^= 0x01u;

    TEST_ASSERT_EQUAL(CRYPTO_ERR_SIGNATURE, crypto_verify_signature(pubkey, digest, signature));
}

static void test_single_bit_pubkey_tamper_rejected(void)
{
    uint8_t pubkey[CRYPTO_PUBKEY_LEN];
    uint8_t digest[CRYPTO_DIGEST_LEN];
    uint8_t signature[CRYPTO_SIGNATURE_LEN];

    decode_field(sample_pubkey_hex, pubkey, sizeof(pubkey));
    decode_field(sample_digest_hex, digest, sizeof(digest));
    decode_field(sample_sig_hex, signature, sizeof(signature));
    pubkey[0] ^= 0x01u;

    TEST_ASSERT_EQUAL(CRYPTO_ERR_SIGNATURE, crypto_verify_signature(pubkey, digest, signature));
}

static void test_null_and_short_inputs_rejected(void)
{
    uint8_t pubkey[CRYPTO_PUBKEY_LEN] = { 0 };
    uint8_t digest[CRYPTO_DIGEST_LEN] = { 0 };
    uint8_t signature[CRYPTO_SIGNATURE_LEN] = { 0 };
    uint8_t buffer[8];

    TEST_ASSERT_EQUAL(CRYPTO_ERR_PARAM, crypto_verify_signature(NULL, digest, signature));
    TEST_ASSERT_EQUAL(CRYPTO_ERR_PARAM, crypto_verify_signature(pubkey, NULL, signature));
    TEST_ASSERT_EQUAL(CRYPTO_ERR_PARAM, crypto_verify_signature(pubkey, digest, NULL));
    TEST_ASSERT_EQUAL(CRYPTO_ERR_PARAM, crypto_verify_image(NULL, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(CRYPTO_ERR_IMAGE, crypto_verify_image(pubkey, buffer, 0));
}

static crypto_status_t expected_status(const char *token)
{
    if (strcmp(token, "OK") == 0) {
        return CRYPTO_OK;
    }
    if (strcmp(token, "IMAGE") == 0) {
        return CRYPTO_ERR_IMAGE;
    }
    if (strcmp(token, "DIGEST") == 0) {
        return CRYPTO_ERR_DIGEST;
    }
    if (strcmp(token, "SIGNATURE") == 0) {
        return CRYPTO_ERR_SIGNATURE;
    }
    return CRYPTO_ERR_PARAM;
}

static void test_python_signed_image_vectors(void)
{
    FILE *fp = fopen(VECTORS_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char line[2048];
    int cases = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') {
            continue;
        }

        char token[16];
        char pubkey_hex[2 * CRYPTO_PUBKEY_LEN + 2];
        char image_hex[1600];

        int fields = sscanf(line, "%15s %128s %1599s", token, pubkey_hex, image_hex);
        TEST_ASSERT_EQUAL_INT(3, fields);

        uint8_t pubkey[CRYPTO_PUBKEY_LEN];
        uint8_t image[800];

        TEST_ASSERT_EQUAL_UINT32(CRYPTO_PUBKEY_LEN,
                                 hex_decode(pubkey_hex, pubkey, sizeof(pubkey)));
        size_t image_len = hex_decode(image_hex, image, sizeof(image));
        TEST_ASSERT_TRUE(image_len > 0);

        crypto_status_t status = crypto_verify_image(pubkey, image, image_len);
        TEST_ASSERT_EQUAL(expected_status(token), status);
        cases++;
    }

    fclose(fp);
    TEST_ASSERT_TRUE(cases >= 8);
}

void run_crypto_tests(void)
{
    RUN_TEST(test_rfc6979_sample_vector_verifies);
    RUN_TEST(test_rfc6979_test_vector_verifies);
    RUN_TEST(test_digest_from_our_sha256_verifies);
    RUN_TEST(test_single_bit_digest_tamper_rejected);
    RUN_TEST(test_single_bit_signature_tamper_rejected);
    RUN_TEST(test_single_bit_pubkey_tamper_rejected);
    RUN_TEST(test_null_and_short_inputs_rejected);
    RUN_TEST(test_python_signed_image_vectors);
}
