/*

Copyright (c) 2016, Robin Raymond
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

*/

#pragma once

#include <zsLib/eventing/types.h>

#include <cryptopp/sha.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>

namespace zsLib
{
  namespace eventing
  {
    interaction IHasherAlgorithm
    {
      virtual const BYTE *digest() const = 0;
      virtual size_t digestSize() const = 0;
      String digestAsString() const;
      virtual void update(const BYTE *buffer, size_t length) = 0;
      virtual const BYTE *finalize() = 0;
      String finalizeAsString()                                                { finalize(); return digestAsString(); }

      void update(const char *str)                                             { if (!str) return; update(reinterpret_cast<const BYTE *>(str), strlen(str)); }
      void update(const std::string &str)                                      { if (str.size() < 1) return; update(reinterpret_cast<const BYTE *>(str.c_str()), str.length()); }

      void update(bool value)                                                  { BYTE tmp {value ? (BYTE)1 : (BYTE)0}; update(static_cast<const BYTE *>(&tmp), sizeof(tmp)); }
      void update(CHAR value)                                                  { update((const BYTE *)(&value), sizeof(value)); }
      void update(UCHAR value)                                                 { update((const BYTE *)(&value), sizeof(value)); }
      void update(SHORT value)                                                 { update((const BYTE *)(&value), sizeof(value)); }
      void update(USHORT value)                                                { update((const BYTE *)(&value), sizeof(value)); }
      void update(INT value)                                                   { update((const BYTE *)(&value), sizeof(value)); }
      void update(UINT value)                                                  { update((const BYTE *)(&value), sizeof(value)); }
      void update(LONG value)                                                  { update((const BYTE *)(&value), sizeof(value)); }
      void update(ULONG value)                                                 { update((const BYTE *)(&value), sizeof(value)); }
      void update(LONGLONG value)                                              { update((const BYTE *)(&value), sizeof(value)); }
      void update(ULONGLONG value)                                             { update((const BYTE *)(&value), sizeof(value)); }
      void update(FLOAT value)                                                 { update((const BYTE *)(&value), sizeof(value)); }
      void update(DOUBLE value)                                                { update((const BYTE *)(&value), sizeof(value)); }

      void update(const Time &value)                                           { Time::duration::rep tmp = value.time_since_epoch().count(); update((const BYTE *)(&tmp), sizeof(tmp)); }
      void update(const Hours &value)                                          { auto tmp = value.count(); update((const BYTE *)(&tmp), sizeof(tmp)); }
      void update(const Minutes &value)                                        { auto tmp = value.count(); update((const BYTE *)(&tmp), sizeof(tmp)); }
      void update(const Seconds &value)                                        { auto tmp = value.count(); update((const BYTE *)(&tmp), sizeof(tmp)); }
      void update(const Milliseconds &value)                                   { auto tmp = value.count(); update((const BYTE *)(&tmp), sizeof(tmp)); }
      void update(const Microseconds &value)                                   { auto tmp = value.count(); update((const BYTE *)(&tmp), sizeof(tmp)); }
      void update(const Nanoseconds &value)                                    { auto tmp = value.count(); update((const BYTE *)(&tmp), sizeof(tmp)); }

      void update(
                  const Optional<String> &value,
                  const char *hashValueWhenNotPresent
                  )                                                            { if (value.hasValue()) update(value.value()); else update(hashValueWhenNotPresent); }

      void update(const Optional<bool> &value)                                 { if (value.hasValue()) update(value.value()); }
      void update(const Optional<CHAR> &value)                                 { if (value.hasValue()) update(value.value()); }
      void update(const Optional<UCHAR> &value)                                { if (value.hasValue()) update(value.value()); }
      void update(const Optional<SHORT> &value)                                { if (value.hasValue()) update(value.value()); }
      void update(const Optional<USHORT> &value)                               { if (value.hasValue()) update(value.value()); }
      void update(const Optional<INT> &value)                                  { if (value.hasValue()) update(value.value()); }
      void update(const Optional<UINT> &value)                                 { if (value.hasValue()) update(value.value()); }
      void update(const Optional<LONG> &value)                                 { if (value.hasValue()) update(value.value()); }
      void update(const Optional<ULONG> &value)                                { if (value.hasValue()) update(value.value()); }
      void update(const Optional<LONGLONG> &value)                             { if (value.hasValue()) update(value.value()); }
      void update(const Optional<ULONGLONG> &value)                            { if (value.hasValue()) update(value.value()); }
      void update(const Optional<FLOAT> &value)                                { if (value.hasValue()) update(value.value()); }
      void update(const Optional<DOUBLE> &value)                               { if (value.hasValue()) update(value.value()); }

      void update(const Optional<Time> &value)                                 { if (value.hasValue()) update(value.value()); }
      void update(const Optional<Hours> &value)                                { if (value.hasValue()) update(value.value()); }
      void update(const Optional<Minutes> &value)                              { if (value.hasValue()) update(value.value()); }
      void update(const Optional<Seconds> &value)                              { if (value.hasValue()) update(value.value()); }
      void update(const Optional<Milliseconds> &value)                         { if (value.hasValue()) update(value.value()); }
      void update(const Optional<Microseconds> &value)                         { if (value.hasValue()) update(value.value()); }
      void update(const Optional<Nanoseconds> &value)                          { if (value.hasValue()) update(value.value()); }
    };

    interaction IHasher
    {
      typedef CryptoPP::Weak::MD5 MD5;
      typedef CryptoPP::SHA256 SHA256;
      typedef CryptoPP::SHA1 SHA1;
      typedef CryptoPP::HMAC<MD5> HMACMD5;
      typedef CryptoPP::HMAC<SHA1> HMACSHA1;
      typedef CryptoPP::HMAC<SHA256> HMACSHA256;

      template <typename THasher>
      class HasherAlgorithm : public IHasherAlgorithm
      {
      public:
        HasherAlgorithm() {}

        static IHasherAlgorithmPtr create() { return make_shared<HasherAlgorithm>(); }

        virtual const BYTE *digest() const override                            { return &(mOutput[0]); }
        virtual size_t digestSize() const override                             { return static_cast<size_t>(mHasher.DigestSize());  }
        virtual void update(const BYTE *buffer, size_t length) override        { mHasher.Update(buffer, length); }
        virtual const BYTE *finalize() override                                { mHasher.Final(&(mOutput[0])); return &(mOutput[0]); }

      private:
        BYTE mOutput[sizeof(THasher)] {};
        mutable THasher mHasher;
      };

      template <typename THMACHasher>
      class HasherHMACAlgorithm : public IHasherAlgorithm
      {
      public:
        HasherHMACAlgorithm(const BYTE *key, size_t keySize)
          : mHasher(key, keySize)                                              {}

        static IHasherAlgorithmPtr create(const BYTE *key, size_t keySize)     { return make_shared<HasherHMACAlgorithm>(key, keySize); }

        virtual const BYTE *digest() const override                            { return &(mOutput[0]); }
        virtual size_t digestSize() const override                             { return static_cast<size_t>(mHasher.DigestSize()); }
        virtual void update(const BYTE *buffer, size_t length) override        { mHasher.Update(buffer, length); }
        virtual const BYTE *finalize() override                                { mHasher.Final(&(mOutput[0])); return &(mOutput[0]); }

      private:
        BYTE mOutput[sizeof(THMACHasher)]{};
        mutable THMACHasher mHasher;
      };

      static IHasherAlgorithmPtr md5()                                         { return HasherAlgorithm<MD5>::create(); }
      static IHasherAlgorithmPtr sha1()                                        { return HasherAlgorithm<SHA1>::create(); }
      static IHasherAlgorithmPtr sha256()                                      { return HasherAlgorithm<SHA256>::create(); }

      static IHasherAlgorithmPtr hmacMD5(const BYTE *key, size_t keySize)      { return HasherHMACAlgorithm<HMACMD5>::create(key, keySize); }
      static IHasherAlgorithmPtr hmacSHA1(const BYTE *key, size_t keySize)     { return HasherHMACAlgorithm<HMACSHA1>::create(key, keySize); }
      static IHasherAlgorithmPtr hmacSHA256(const BYTE *key, size_t keySize)   { return HasherHMACAlgorithm<HMACSHA256>::create(key, keySize); }

      static IHasherAlgorithmPtr hmacMD5(const SecureByteBlock &key)           { return hmacMD5(key.BytePtr(), key.SizeInBytes()); }
      static IHasherAlgorithmPtr hmacSHA1(const SecureByteBlock &key)          { return hmacSHA1(key.BytePtr(), key.SizeInBytes()); }
      static IHasherAlgorithmPtr hmacSHA256(const SecureByteBlock &key)        { return hmacSHA256(key.BytePtr(), key.SizeInBytes()); }

      static IHasherAlgorithmPtr hmacMD5(SecureByteBlockPtr key)               { BYTE temp{}; if (!key) return hmacMD5(&temp, 0); return hmacMD5(key->BytePtr(), key->SizeInBytes()); }
      static IHasherAlgorithmPtr hmacSHA1(SecureByteBlockPtr key)              { BYTE temp{}; if (!key) return hmacSHA1(&temp, 0); return hmacSHA1(key->BytePtr(), key->SizeInBytes()); }
      static IHasherAlgorithmPtr hmacSHA256(SecureByteBlockPtr key)            { BYTE temp{}; if (!key) return hmacSHA256(&temp, 0); return hmacSHA256(key->BytePtr(), key->SizeInBytes()); }

      template <typename Thasher>
      static size_t digestSize()                                               { Thasher temp; return temp.DigestSize(); }

      static size_t md5DigestSize()                                            { return digestSize<MD5>(); }
      static size_t sha1DigestSize()                                           { return digestSize<SHA1>(); }
      static size_t sha256DigestSize()                                         { return digestSize<SHA256>(); }
      static size_t hmacMD5DigestSize()                                        { return digestSize<MD5>(); }
      static size_t hmacSHA1DigestSize()                                       { return digestSize<SHA1>(); }
      static size_t hmacSHA256DigestSize()                                     { return digestSize<SHA256>(); }

      static SecureByteBlockPtr hash(const char *str, IHasherAlgorithmPtr algorithm = sha1());
      static SecureByteBlockPtr hash(const std::string &str, IHasherAlgorithmPtr algorithm = sha1());
      static SecureByteBlockPtr hash(const BYTE *buffer, size_t length, IHasherAlgorithmPtr algorithm = sha1());
      static SecureByteBlockPtr hash(const SecureByteBlock &buffer, IHasherAlgorithmPtr algorithm = sha1());
      static SecureByteBlockPtr hash(const SecureByteBlockPtr &buffer, IHasherAlgorithmPtr algorithm = sha1());

      static String hashAsString(const char *str, IHasherAlgorithmPtr algorithm = sha1());
      static String hashAsString(const std::string &str, IHasherAlgorithmPtr algorithm = sha1());
      static String hashAsString(const BYTE *buffer, size_t length, IHasherAlgorithmPtr algorithm = sha1());
      static String hashAsString(const SecureByteBlock &buffer, IHasherAlgorithmPtr algorithm = sha1());
      static String hashAsString(const SecureByteBlockPtr &buffer, IHasherAlgorithmPtr algorithm = sha1());

      static SecureByteBlockPtr hmacKeyFromPassphrase(const char *passphrase);
      static SecureByteBlockPtr hmacKeyFromPassphrase(const std::string &passphrase);
    };
  }
}
