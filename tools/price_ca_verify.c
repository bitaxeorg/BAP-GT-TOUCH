/* Host regression driver for the SDK verifier and production price trust bytes. */
#include "sdk_bundle_prefix.c"
#include "price_root_ca.h"

int main(int argc, char **argv)
{
    if (argc != 5) return 64;
    mbedtls_x509_crt chain, ca;
    mbedtls_x509_crt_init(&chain);
    mbedtls_x509_crt_init(&ca);
    uint32_t flags = 0;
    int ret = mbedtls_x509_crt_parse_file(&chain, argv[2]);
    if (ret != 0) return 65;
    int compact = strcmp(argv[1], "compact") == 0;
    if (compact) {
        FILE *file = fopen(argv[4], "rb");
        if (!file) return 66;
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);
        unsigned char *data = malloc(size);
        if (!data || fread(data, 1, size, file) != (size_t)size) return 67;
        fclose(file);
        ret = esp_crt_bundle_init(data, size);
        if (ret != 0) return 68;
        ret = mbedtls_x509_crt_verify(&chain, &ca, NULL, argv[3], &flags,
                                     esp_crt_verify_callback, NULL);
        free(data);
    } else {
        ret = strcmp(argv[4], "production") == 0
            ? mbedtls_x509_crt_parse(&ca, (const unsigned char *)price_root_ca,
                                     sizeof(price_root_ca))
            : mbedtls_x509_crt_parse_file(&ca, argv[4]);
        if (ret != 0) { printf("trust_parse=%d\n", ret); return 69; }
        ret = mbedtls_x509_crt_verify(&chain, &ca, NULL, argv[3], &flags, NULL, NULL);
    }
    printf("verify_return=%d flags=0x%x\n", ret, flags);
    mbedtls_x509_crt_free(&chain);
    mbedtls_x509_crt_free(&ca);
    return ret == 0 ? 0 : 2;
}
