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

#include <zsLib/eventing/internal/zsLib_eventing_Hasher.h>
#include <zsLib/eventing/internal/zsLib_eventing_Helper.h>

#include <zsLib/Numeric.h>
#include <zsLib/Log.h>

#include <cstdio>

#include <cryptopp/hex.h>

namespace zsLib { namespace eventing { ZS_DECLARE_SUBSYSTEM(zsLib_eventing); } }


namespace zsLib
{
  namespace eventing
  {
    ZS_DECLARE_TYPEDEF_PTR(eventing::internal::Helper, UseEventingHelper);

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Helper
      #pragma mark

      //-----------------------------------------------------------------------
      Log::Params Hasher::slog(const char *message)
      {
        return Log::Params(message, "eventing::Hasher");
      }
    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IHasherAlgorithm
    #pragma mark

    //-------------------------------------------------------------------------
    String IHasherAlgorithm::digestAsString() const
    {
      return IHelper::convertToHex(digest(), digestSize());
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IHasher
    #pragma mark

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hash(const char *str, IHasherAlgorithmPtr algorithm)
    {
      if (!str) str = "";
      auto len = strlen(str);
      return hash(reinterpret_cast<const BYTE *>(str), len, algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hash(const std::string &str, IHasherAlgorithmPtr algorithm)
    {
      return hash(reinterpret_cast<const BYTE *>(str.c_str()), str.length(), algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hash(const BYTE *buffer, size_t length, IHasherAlgorithmPtr algorithm)
    {
      algorithm->update(buffer, length);
      algorithm->finalize();

      return make_shared<SecureByteBlock>(algorithm->digest(), algorithm->digestSize());
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hash(const SecureByteBlock &buffer, IHasherAlgorithmPtr algorithm)
    {
      return hash(buffer.BytePtr(), buffer.SizeInBytes(), algorithm);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hash(const SecureByteBlockPtr &buffer, IHasherAlgorithmPtr algorithm)
    {
      if (!buffer) return SecureByteBlockPtr();
      return hash(buffer->BytePtr(), buffer->SizeInBytes(), algorithm);
    }

    //-------------------------------------------------------------------------
    String IHasher::hashAsString(const char *str, IHasherAlgorithmPtr algorithm)
    {
      if (!str) str = "";
      auto len = strlen(str);
      return hashAsString(reinterpret_cast<const BYTE *>(str), len, algorithm);
    }

    //-------------------------------------------------------------------------
    String IHasher::hashAsString(const std::string &str, IHasherAlgorithmPtr algorithm)
    {
      return hashAsString(reinterpret_cast<const BYTE *>(str.c_str()), str.length(), algorithm);
    }

    //-------------------------------------------------------------------------
    String IHasher::hashAsString(const BYTE *buffer, size_t length, IHasherAlgorithmPtr algorithm)
    {
      auto result = hash(buffer, length, algorithm);
      return UseEventingHelper::convertToHex(*result);
    }

    //-------------------------------------------------------------------------
    String IHasher::hashAsString(const SecureByteBlock &buffer, IHasherAlgorithmPtr algorithm)
    {
      return hashAsString(buffer.BytePtr(), buffer.SizeInBytes(), algorithm);
    }

    //-------------------------------------------------------------------------
    String IHasher::hashAsString(const SecureByteBlockPtr &buffer, IHasherAlgorithmPtr algorithm)
    {
      if (!buffer) return String();
      return hashAsString(buffer->BytePtr(), buffer->SizeInBytes(), algorithm);
    }

    //-----------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hmacKeyFromPassphrase(const char *passphrase)
    {
      return IHelper::convertToBuffer(passphrase);
    }

    //-----------------------------------------------------------------------
    SecureByteBlockPtr IHasher::hmacKeyFromPassphrase(const std::string &passphrase)
    {
      return IHelper::convertToBuffer(passphrase.c_str());
    }

  } // namespace eventing
} // namespace zsLib
