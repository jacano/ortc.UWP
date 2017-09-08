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
#include <zsLib/IHelper.h>

#ifdef _WIN32
#include <intsafe.h>
#endif //_WIN32

namespace zsLib
{
  namespace eventing
  {
    interaction IHelper : public zsLib::IHelper
    {
      static void setup();
#ifdef WINRT
      static void setup(Windows::UI::Core::CoreDispatcher ^dispatcher);
#endif //WINRT

      static SecureByteBlockPtr loadFile(const char *path) throw (StdError);
      static void saveFile(const char *path, SecureByteBlock &buffer) throw (StdError);
      static void mkdir(const char *path, bool ignoreExists = true) throw (StdError);

      static ElementPtr read(const SecureByteBlockPtr buffer);
      static ElementPtr read(const SecureByteBlock &buffer);
      static SecureByteBlockPtr writeJSON(
                                          const Document &doc,
                                          bool prettyPrint = true
                                          );
      static SecureByteBlockPtr writeJSON(
                                          DocumentPtr doc,
                                          bool prettyPrint = true
                                          );
      static SecureByteBlockPtr writeXML(const Document &doc);
      static SecureByteBlockPtr writeXML(DocumentPtr doc);

      static String randomString(size_t lengthInChars);

      static SecureByteBlockPtr random(size_t lengthInBytes);

      static size_t random(size_t minValue, size_t maxValue);

      static int compare(
                         const SecureByteBlock &left,
                         const SecureByteBlock &right
                         );

      static bool isEmpty(SecureByteBlockPtr buffer);
      static bool isEmpty(const SecureByteBlock &buffer);

      static bool hasData(SecureByteBlockPtr buffer);
      static bool hasData(const SecureByteBlock &buffer);

      static SecureByteBlockPtr clone(SecureByteBlockPtr pBuffer);
      static SecureByteBlockPtr clone(const SecureByteBlock &buffer);

      static String convertToString(const SecureByteBlock &buffer);
      static SecureByteBlockPtr convertToBuffer(const char *input);
      static SecureByteBlockPtr convertToBuffer(const std::string &input);
      static SecureByteBlockPtr convertToBuffer(
                                               const BYTE *buffer,
                                               size_t bufferLengthInBytes
                                               );
      static SecureByteBlockPtr convertToBuffer(
                                                const std::unique_ptr<char[]> &arrayStr,
                                                size_t lengthInChars = SIZE_MAX,
                                                bool wipeOriginal = true
                                                );

      static String convertToBase64(
                                    const BYTE *buffer,
                                    size_t bufferLengthInBytes
                                    );

      static String convertToBase64(const SecureByteBlock &input);

      static String convertToBase64(const String &input);

      static SecureByteBlockPtr convertFromBase64(const String &input);

      static String convertToHex(
                                 const BYTE *buffer,
                                 size_t bufferLengthInBytes,
                                 bool outputUpperCase = false
                                 );

      static String convertToHex(
                                 const SecureByteBlock &input,
                                 bool outputUpperCase = false
                                 );

      static SecureByteBlockPtr convertFromHex(const String &input);

      static String getDebugString(
                                   const SecureByteBlock &buffer,
                                   size_t bytesPerGroup = 4,
                                   size_t maxLineLength = 160
                                   );
    };
  }
}
