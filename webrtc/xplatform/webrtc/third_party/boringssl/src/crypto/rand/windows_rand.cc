/* Copyright (c) 2014, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <openssl/rand.h>

#if defined(OPENSSL_WINDOWS)

#ifdef WINRT

#include <limits.h>
#include <stdlib.h>
#include <Windows.h>

using namespace Platform;
using namespace Windows::Security::Cryptography;

extern "C" void CRYPTO_sysrand(uint8_t *out, size_t requested) {
  auto buffer = CryptographicBuffer::GenerateRandom(requested);
  if (buffer == nullptr) {
    abort();
    return;
  }
  auto array = ref new Array<uint8_t>(requested);
  CryptographicBuffer::CopyToByteArray(buffer, &array);
  memcpy_s(out, requested, array->Data, requested);
}
#endif // WINRT

#endif  /* OPENSSL_WINDOWS */
