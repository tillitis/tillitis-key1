
int8_t uint8_to_str(uint8_t *buf, uint8_t bufsize, uint8_t n)
{
    uint8_t *start;

#ifdef USE_NEGATIVE_NUMS
    if (n < 0) { // Handle negative numbers.
        if (!bufsize) {
            return -1;
        }
        *buf++ = '-';
        bufsize--;
    }
#endif

    start = buf; // Remember the start of the string. This will come into play at the end.

    do {
        // Handle the current digit.
        uint8_t digit;
        if (!bufsize) {
            return -1;
        }
        digit = n % 10;
#ifdef USE_NEGATIVE_NUMS
        if (digit < 0) {
            digit *= -1;
        }
#endif
        *buf++ = digit + '0';
        bufsize--;
        n /= 10;
    } while (n);

    // Terminate the string.
    if (!bufsize) {
        return -1;
    }
    *buf = 0;

    // We wrote the string backwards, i.e. with least significant digits first. Now reverse the string.
    --buf;
    while (start < buf) {
        uint8_t a = *start;
        *start = *buf;
        *buf = a;
        ++start;
        --buf;
    }

    return 0;
}
#endif

#ifdef USE_NUM_U32
int8_t uint32_to_str(uint8_t *buf, uint8_t bufsize, uint32_t n)
{
    uint8_t *start;

#ifdef USE_NEGATIVE_NUMS
    if (n < 0) { // Handle negative numbers.
        if (!bufsize) {
            return -1;
        }
        *buf++ = '-';
        bufsize--;
    }
#endif

    start = buf; // Remember the start of the string. This will come into play at the end.

    do {
        // Handle the current digit.
        uint8_t digit;
        if (!bufsize) {
            return -1;
        }
        digit = n % 10;
#ifdef USE_NEGATIVE_NUMS
        if (digit < 0) {
            digit *= -1;
        }
#endif
        *buf++ = digit + '0';
        bufsize--;
        n /= 10;
    } while (n);

    // Terminate the string.
    if (!bufsize) {
        return -1;
    }
    *buf = 0;

    // We wrote the string backwards, i.e. with least significant digits first. Now reverse the string.
    --buf;
    while (start < buf) {
        uint8_t a = *start;
        *start = *buf;
        *buf = a;
        ++start;
        --buf;
    }

    return 0;
}
#endif

#if 0
//uint8_t hexstr_to_uint8(const char *hex, uint8_t len)
uint8_t hex_to_uint8(const char *hex, uint8_t len)
{
    uint8_t val = 0;
    while (len) {
        char c = *hex++;
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= c - '0';
        } else if (c >= 'A' && c <= 'F') {
            val |= c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            val |= c - 'a' + 10;
        } else {
            return 0; // Invalid character
        }
        len--;
    }
    return val;
}
#endif

#if 0
//uint16_t hexstr_to_uint16(const char *hex, uint8_t len)
uint16_t hex_to_uint16(const char *hex, uint8_t len)
{
    uint16_t val = 0;
    while (len) {
        char c = *hex++;
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= c - '0';
        } else if (c >= 'A' && c <= 'F') {
            val |= c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            val |= c - 'a' + 10;
        } else {
            return 0; // Invalid character
        }
        len--;
    }
    return val;
}
#endif

//uint32_t hexstr_to_uint32(const char *hex, uint8_t len)
uint32_t hex_to_uint32(const char *hex, uint8_t len)
{
    uint32_t val = 0;
    while (len) {
        char c = *hex++;
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= c - '0';
        } else if (c >= 'A' && c <= 'F') {
            val |= c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            val |= c - 'a' + 10;
        } else {
            return 0; // Invalid character
        }
        len--;
    }
    return val;
}

// uint8_t hexchar_to_nibble(uint8_t c)
uint8_t ascii_hex_char_to_byte(uint8_t c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0; // Invalid character, should not happen if input is valid
}

#if 0
int ascii_hex_string_to_bytes(uint8_t *hex_str, uint8_t *out_bytes, size_t out_len)
{
    if (!hex_str || !out_bytes || out_len < 32)
        return -1; // Error handling

    for (size_t i = 0; i < 32; i++) {
        out_bytes[i] = (ascii_hex_char_to_byte(hex_str[i * 2]) << 4) | ascii_hex_char_to_byte(hex_str[i * 2 + 1]);
    }

    return 0; // Success
}
#endif

#if 0
int hexstr_to_bin32(uint8_t *hex_str, uint8_t *out_bytes, size_t out_len)
{
    if (!hex_str || !out_bytes || out_len < 32)
        return -1; // Error handling

    for (size_t i = 0; i < 32; i++) {
        out_bytes[i] = (hexchar_to_nibble(hex_str[i * 2]) << 4) | hexchar_to_nibble(hex_str[i * 2 + 1]);
    }

    return 0; // Success
}
#endif
