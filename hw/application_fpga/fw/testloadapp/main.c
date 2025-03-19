#include <blake2s/blake2s.h>
#include <monocypher/monocypher-ed25519.h>
#include <stdint.h>
#include <tkey/tk1_mem.h>
#include <tkey/debug.h>

#include "../testapp/syscall.h"
#include "../tk1/syscall_num.h"
#include "blink.h"
#include "tkey/assert.h"

// clang-format off
static volatile uint32_t *cdi           = (volatile uint32_t *) TK1_MMIO_TK1_CDI_FIRST;
// clang-format on

int install_app(uint8_t secret_key[64])
{
	uint8_t app_digest[32];
	uint8_t app_signature[64];
	size_t app_size = 30;

	if (syscall(TK1_SYSCALL_REG_MGMT, 0, 0, 0) < 0) {
		puts(IO_CDC, "couldn't register as mgmt\r\n");
		return -1;
	}

	int err = syscall(TK1_SYSCALL_PRELOAD_STORE, 0, (uint32_t)blink,
			  sizeof(blink));

	if (err < 0) {
		puts(IO_CDC, "couldn't store app, error: ");
		putinthex(IO_CDC, err);
		puts(IO_CDC, "\r\n");

		return -1;
	}

	if (blake2s(app_digest, 32, NULL, 0, blink, sizeof(blink)) != 0) {
		puts(IO_CDC, "couldn't compute digest\r\n");
		return -1;
	}

	crypto_ed25519_sign(app_signature, secret_key, app_digest,
			    sizeof(app_digest));

	if (syscall(TK1_SYSCALL_PRELOAD_STORE_FIN, app_size,
		    (uint32_t)app_digest, (uint32_t)app_signature) < 0) {
		puts(IO_CDC, "couldn't finalize storing app\n");
		return -1;
	}

	return 0;
}

int verify(uint8_t pubkey[32])
{
	uint8_t app_digest[32];
	uint8_t app_signature[64];

	// pubkey we already have
	// read signature
	// read digest

	if (!crypto_ed25519_check(app_signature, pubkey, app_digest,
				  sizeof(app_digest))) {
		// failed!!!
	}

	// syscall reset flash2_ver with app_digest

	return 0;
}

int main(void)
{
	uint8_t secret_key[64];
	uint8_t pubkey[32];
	enum ioend endpoint;
	uint8_t available;
	uint8_t in = 0;

	// Generate a key pair from CDI
	crypto_ed25519_key_pair(secret_key, pubkey, (uint8_t *)cdi);

	if (readselect(IO_CDC, &endpoint, &available) < 0) {
		// readselect failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	if (read(IO_CDC, &in, 1, 1) < 0) {
		// read failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	puts(IO_CDC, "Hello from testloadapp! 0 = install app in slot 1, 1 = "
		     "verify app\r\n");

	for (;;) {
		if (readselect(IO_CDC, &endpoint, &available) < 0) {
			// readselect failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		if (read(IO_CDC, &in, 1, 1) < 0) {
			// read failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		switch (in) {
		case '0':
			if (install_app(secret_key) < 0) {
				puts(IO_CDC, "Failed to install app\r\n");
			} else {
				puts(IO_CDC, "Installed app!\r\n");
			}

			break;

		case '1':
			if (verify(pubkey) < 0) {
				puts(IO_CDC, "Failed to verify app\r\n");
			} else {
				puts(IO_CDC, "Verified app!\r\n");
			}

			break;

		default:
			break;
		}
	}
}
