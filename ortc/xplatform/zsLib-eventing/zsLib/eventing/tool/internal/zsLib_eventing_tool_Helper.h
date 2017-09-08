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

#include <zsLib/eventing/tool/internal/types.h>

#include <zsLib/eventing/IHelper.h>

namespace zsLib
{
  namespace eventing
  {
    namespace tool
    {
      namespace internal
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Helper
        #pragma mark

        class Helper : public eventing::IHelper
        {
        public:
          static String fileNameAfterPath(const String &filePath);
          static String fixRelativeFilePath(const String &originalFileName, const String &newFileName);

          static bool isLikelyJSON(const char *p);

          static bool skipEOL(
                              const char * &p,
                              ULONG *currentLine
                              );
          static void skipToEOL(const char * &p);
          static bool skipWhitespaceExceptEOL(const char * &p);
          
          static bool skipCComments(
                                    const char * &p,
                                    ULONG *lineCount
                                    );
          
          static bool skipCPPComments(const char * &p);
          
          static bool skipQuote(
                                const char * &p,
                                ULONG *currentLine
                                );
          
          static bool isQuotes(const String &str);

          static String decodeCEscape(
                                      const char * &p,
                                      ULONG lineCount
                                      ) throw (FailureWithLine);
          static String decodeQuotes(
                                     const String &str,
                                     ULONG lineCount
                                     ) throw (FailureWithLine);
        };

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
