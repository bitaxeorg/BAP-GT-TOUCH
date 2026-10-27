#!/usr/bin/env python3
"""Exercise production price trust bytes with the selected SDK's real verifier.

Native host configuration, not device execution. Requires CMake, a C compiler,
OpenSSL 3, and the complete ESP-IDF source (including mbedTLS submodule).
"""
import argparse
import ast
import hashlib
from pathlib import Path
import re
import ssl
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FINGERPRINT = "349dfa4058c5e263123b398ae795573c4e1313c83fe68f93556cd5e8031b3c7d"


def run(*args, **kwargs):
    return subprocess.run([str(x) for x in args], check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--idf-path", required=True, type=Path)
    parser.add_argument("--openssl", default="openssl")
    args = parser.parse_args()
    header = (ROOT / "main/price_root_ca.h").read_text()
    pem = "".join(ast.literal_eval(x) for x in re.findall(r'"[^"\n]*"', header))
    assert hashlib.sha256(ssl.PEM_cert_to_DER_cert(pem)).hexdigest() == FINGERPRINT
    assert pem in (ROOT / "certificates/cacert-2026-10-20.pem").read_text()
    source = (ROOT / "main/price.c").read_text()
    assert ".cert_pem = price_root_ca," in source
    assert "crt_bundle_attach" not in source
    assert "skip_cert_common_name_check" not in source
    sdk = args.idf_path.resolve()
    mbedtls = sdk / "components/mbedtls/mbedtls"
    with tempfile.TemporaryDirectory(prefix="price-ca-") as name:
        work = Path(name)
        run("cmake", "-S", mbedtls, "-B", work / "native", "-DENABLE_TESTING=OFF",
            "-DENABLE_PROGRAMS=OFF", "-DUSE_SHARED_MBEDTLS_LIBRARY=OFF")
        run("cmake", "--build", work / "native", "--target", "mbedx509", "--parallel", "4")
        # Compile the unchanged SDK callback and initialization. Only platform
        # logging/error definitions are replaced, not verification behavior.
        callback = (sdk / "components/mbedtls/esp_crt_bundle/esp_crt_bundle.c").read_text()
        callback = callback[:callback.index("esp_err_t esp_crt_bundle_attach(")]
        for include in ("esp_check.h", "esp_crt_bundle.h", "esp_log.h"):
            callback = callback.replace('#include "' + include + '"', "")
        prefix = '''#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "mbedtls/x509_crt.h"
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_NO_MEM 2
#define CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_MAX_CERTS 200
#define unlikely(x) (x)
#define likely(x) (x)
#define ESP_LOGE(tag, ...) do { printf(__VA_ARGS__); puts(""); } while (0)
#define ESP_LOGI(tag, ...) do {} while (0)
#define ESP_LOGD(tag, ...) do {} while (0)
'''
        (work / "sdk_bundle_prefix.c").write_text(prefix + callback)
        run("cc", "-O2", "-I" + str(work), "-I" + str(ROOT / "main"),
            "-I" + str(mbedtls / "include"), "-I" + str(mbedtls / "library"),
            ROOT / "tools/price_ca_verify.c", work / "native/library/libmbedx509.a",
            work / "native/library/libmbedcrypto.a", "-o", work / "verify")
        run("python3", sdk / "components/mbedtls/esp_crt_bundle/gen_crt_bundle.py",
            "--input", ROOT / "certificates/cacert-2026-10-20.pem", cwd=work)
        checks = 0

        def verify(mode, chain, host, trust, expected, label):
            nonlocal checks
            result = subprocess.run([str(work / "verify"), mode, str(chain), host, str(trust)],
                                    capture_output=True, text=True)
            assert result.returncode == expected, (label, result.returncode, result.stdout, result.stderr)
            checks += 1
            if label.startswith("incident "):
                flag_match = re.search(r"flags=0x([0-9a-f]+)", result.stdout)
                assert flag_match and int(flag_match.group(1), 16) & 8, (label, result.stdout)
            print("PASS", label, result.stdout.strip(), flush=True)

        for host in ("api.coingecko.com", "api.coinbase.com"):
            chain = ROOT / "certificates/test" / (host + "-full.pem")
            verify("compact", chain, host, work / "x509_crt_bundle", 2, host + " old full chain")
            verify("parsed", chain, host, "production", 0, host + " fixed full chain")
            verify("parsed", chain, "wrong.example", "production", 2, host + " wrong hostname")
            certs = re.findall(r"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----\n",
                               chain.read_text(), re.S)
            shortened = work / "shortened.pem"
            shortened.write_text("".join(certs[:2]))
            verify("compact", shortened, host, work / "x509_crt_bundle", 0, host + " old shortened chain")
            verify("parsed", shortened, host, "production", 0, host + " fixed shortened chain")
        # Same production trust bytes against each historical incident chain.
        # Require NOT_TRUSTED explicitly, so expiry alone cannot pass these tests.
        with zipfile.ZipFile(ROOT / "certificates/test/emudhra-chains-2026-10-20.zip") as archive:
            for filename in archive.namelist():
                if not filename.endswith(".pem"):
                    continue
                chain = work / Path(filename).name
                chain.write_bytes(archive.read(filename))
                verify("parsed", chain, chain.stem, "production", 2, "incident " + chain.stem)
        run(args.openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
            "-subj", "/CN=api.coingecko.com", "-keyout", work / "key.pem", "-out", work / "self.pem",
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        chain = ROOT / "certificates/test/api.coingecko.com-full.pem"
        verify("parsed", work / "self.pem", "api.coingecko.com", "production", 2, "self signed")
        verify("parsed", chain, "api.coingecko.com", work / "self.pem", 2, "missing approved trust")
        run(args.openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
            "-subj", "/C=US/O=Google Trust Services LLC/CN=GTS Root R4",
            "-keyout", work / "impostor-key.pem", "-out", work / "impostor.pem",
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        verify("parsed", chain, "api.coingecko.com", work / "impostor.pem", 2,
               "same subject different key cannot anchor")
        (work / "corrupt.pem").write_text("not a certificate\n")
        verify("parsed", chain, "api.coingecko.com", work / "corrupt.pem", 69, "corrupt trust fails closed")
        print(f"{checks} SDK price trust expectations passed")


if __name__ == "__main__":
    main()
