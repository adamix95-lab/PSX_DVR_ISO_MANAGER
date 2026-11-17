#include "v12_roughtime_monocypher.h"
#include "v12_roughtime_verify.h"
#include "v12_rt_test_vector.h"
#include "monocypher-ed25519.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    v12_rt_crypto crypto = v12_rt_monocypher_backend();
    v12_rt_signed_time time = {0, 0};
    uint8_t reply[sizeof(rt_fixture_reply)], wrong[32] = {0};
    uint8_t root[32], secret[64], seed[32];
    size_t size = sizeof(reply);

    /* Published root_key in the upstream vector is a test-only seed. */
    memcpy(seed, rt_fixture_root_seed, sizeof(seed));
    crypto_ed25519_key_pair(secret, root, seed);
    assert(v12_rt_verify_draft08(rt_fixture_reply, size, root,
                                rt_fixture_nonce, &crypto, &time) == 0);
    assert(time.midpoint_utc == 50 && time.radius_seconds == 5);
    assert(v12_rt_verify_draft08(rt_fixture_reply, size - 1, root,
                                rt_fixture_nonce, &crypto, &time) == V12_RT_MALFORMED);
    assert(v12_rt_verify_draft08(rt_fixture_reply, size, root,
                                wrong, &crypto, &time) == V12_RT_NONCE_MISMATCH);
    assert(v12_rt_verify_draft08(rt_fixture_reply, size, wrong,
                                rt_fixture_nonce, &crypto, &time) == V12_RT_DELEGATION_SIGNATURE);

    memcpy(reply, rt_fixture_reply, size);
    reply[68] ^= 1; /* First value in this fixture is the response signature. */
    assert(v12_rt_verify_draft08(reply, size, root,
                                rt_fixture_nonce, &crypto, &time) == V12_RT_RESPONSE_SIGNATURE);
    memcpy(reply, rt_fixture_reply, size);
    reply[size - 4] = 1; /* INDX=1 with an empty Merkle path. */
    assert(v12_rt_verify_draft08(reply, size, root,
                                rt_fixture_nonce, &crypto, &time) == V12_RT_MERKLE_ROOT);
    assert(time.midpoint_utc == 50 && time.radius_seconds == 5);
    puts("Roughtime fixture: valid reply accepted, bad frame/nonce/signatures/Merkle root rejected");
    return 0;
}
