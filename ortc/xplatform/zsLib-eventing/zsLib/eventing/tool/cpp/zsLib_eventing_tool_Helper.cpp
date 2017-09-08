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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Helper.h>

#include <zsLib/Exception.h>
#include <zsLib/Numeric.h>

#include <sstream>

namespace zsLib { namespace eventing { namespace tool { ZS_DECLARE_SUBSYSTEM(zsLib_eventing_tool) } } }

namespace zsLib
{
  namespace eventing
  {
    namespace tool
    {
      namespace internal
      {
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark Helpers
        #pragma mark

        //---------------------------------------------------------------------
        String Helper::fileNameAfterPath(const String &filePath)
        {
          String result(filePath);
          result.replaceAll("\\", "/");
          auto pos = result.rfind('/');
          if (pos == String::npos) return result;
          
          return result.substr(pos + 1);
        }
        

        //---------------------------------------------------------------------
        String Helper::fixRelativeFilePath(const String &originalFileName, const String &newFileName)
        {
          String result(newFileName);
          result.trim();
          result.replaceAll("\\", "/");
          if (0 == result.find('/')) return result;

          String path(originalFileName);
          path.trim();
          path.replaceAll("\\", "/");
          auto pos = path.rfind('/');
          if (String::npos == pos) return result;

          path = path.substr(0, pos + 1);
          result = path + result;
          return result;
        }

        //-----------------------------------------------------------------------
        bool Helper::isLikelyJSON(const char *p)
        {
          while ('\0' != *p)
          {
            if (iswspace(*p)) {
              ++p;
              continue;
            }
            if (('{' == *p) || ('[' == *p)) return true;
            break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        bool Helper::skipEOL(
                            const char * &p,
                            ULONG *currentLine
                            )
        {
          if (('\r' != *p) && ('\n' != *p)) return false;

          do
          {
            if (NULL != currentLine) {
              if ('\n' == *p) (++(*currentLine));
            }
            ++p;
          } while (('\r' == *p) || ('\n' == *p));

          return true;
        }

        //---------------------------------------------------------------------
        void Helper::skipToEOL(const char * &p)
        {
          while ('\0' != *p)
          {
            if (('\r' != *p) && ('\n' != *p)) {
              ++p;
              continue;
            }
            break;
          }
        }

        //-----------------------------------------------------------------------
        bool Helper::skipWhitespaceExceptEOL(const char * &p)
        {
          bool found = false;
          
          while ('\0' != *p)
          {
            if (('\r' == *p) || ('\n' == *p)) break;
            if (!isspace(*p)) break;
            ++p;
          }
          return found;
        }
        
        //-----------------------------------------------------------------------
        bool Helper::skipCComments(
                                   const char * &p,
                                   ULONG *lineCount
                                   )
        {
          if ('/' != *p) return false;
          if ('*' != *(p + 1)) return false;
          
          p += 2;
          
          while ('\0' != *p)
          {
            if (NULL != lineCount) {
              if ('\n' == *p) (++(*lineCount));
            }
            if ('*' != *p) {
              ++p;
              continue;
            }
            if ('/' != *(p + 1)) {
              ++p;
              continue;
            }
            
            p += 2;
            break;
          }
          
          return true;
        }
        
        //---------------------------------------------------------------------
        bool Helper::skipCPPComments(const char * &p)
        {
          if ('/' != *p) return false;
          if ('/' != *(p + 1)) return false;

          p += 2;

          skipToEOL(p);
          return true;
        }

        //---------------------------------------------------------------------
        bool Helper::skipQuote(
                              const char * &p,
                              ULONG *currentLine
                              )
        {
          const char *start = p;
          
          if (('L' == *start) ||
              ('u' == *start)) {
            ++start;
          }

          if ('\"' != *start) return false;

          p = start;

          ++p;
          while ('\0' != *p) {
            switch (*p) {
              case '\n': {
                if (NULL != currentLine) {
                  ++(*currentLine);
                }
                break;
              }
              case '\"': {
                ++p;
                return true;
              }
              case '\\': {
                ++p;
                if ('\0' == *p) return true;
                ++p;
                continue;
              }
            }
            ++p;
          }

          return true;
        }

        //---------------------------------------------------------------------
        bool Helper::isQuotes(const String &str)
        {
          if (str.length() < 2) return false;

          const char *start = str.c_str();

          if (('L' == *start) ||
              ('u' == *start)) {
            ++start;
            if (str.length() < 3) return false;
          }

          if ('\"' != (*start)) return false;
          if ('\"' != (*(str.c_str() + str.length() - 1))) return false;
          return true;
        }
        
        //---------------------------------------------------------------------
        String Helper::decodeCEscape(
                                     const char * &p,
                                     ULONG lineCount
                                     ) throw (FailureWithLine)
        {
          std::stringstream ss;
          
          const char *start = p;
          
          size_t pendingDigits = 0;
          size_t filledDigits = 0;
          bool isOctal = false;
          char digits[9] {};
          bool isEscape = true;
          
          while ('\0' != *p) {
            {
              char value = *p;
              ++p;
              
              if (pendingDigits > 0) {
                if (!isOctal) {
                  if (!(((value >= '0') && (value <= '9')) ||
                        ((value >= 'a') && (value <= 'f')) ||
                        ((value >= 'A') && (value <= 'F')))) {
                    --p;  // backup once
                    goto digit_sequence_ended;
                  }
                } else {
                  if (!((value >= '0') && (value < '8'))) {
                    --p;  // backup once
                    goto digit_sequence_ended;
                  }
                }
                
                digits[filledDigits] = value;
                
                ++filledDigits;
                --pendingDigits;
                if (0 == pendingDigits) goto digit_sequence_ended;
                continue;
              }
              
              switch (value)
              {
                case '\\': ss << '\\'; break;
                case '\'': ss << '\''; break;
                case '\"': ss << '\"'; break;
                case '?': ss << '\?'; break;
                case 'a': ss << '\a'; break;
                case 'b': ss << '\b'; break;
                case 'f': ss << '\f'; break;
                case 'n': ss << '\n'; break;
                case 'r': ss << '\r'; break;
                case 't': ss << '\t'; break;
                case 'v': ss << '\v'; break;
                case 'u': pendingDigits = 4; goto found_digit_sequence;
                case 'U': pendingDigits = 8; goto found_digit_sequence;
                case 'x': pendingDigits = 2; goto found_digit_sequence;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': {
                  isOctal = true;
                  pendingDigits = 2;
                  goto found_digit_sequence;
                }
                default: {
                  goto invalid_sequence;
                }
              }
              
              if (0 != pendingDigits) goto invalid_sequence;
              
              isEscape = false;
              goto done;
            }

          found_digit_sequence:
            {
              if (0 != pendingDigits) goto invalid_sequence;

              filledDigits = 0;
              memset(&(digits[0]), 0, sizeof(digits));
              continue;
            }
            
          digit_sequence_ended:
            {
              if (0 == filledDigits) {
                goto invalid_sequence;
              }
              
              try {
                if (!isOctal) {
                  wchar_t temp[2] = {};
                  temp[0] = static_cast<wchar_t>(Numeric<wchar_t>(&(digits[0]), true, 16));
                  ss << String(&(temp[0]));
                } else {
                  char temp = static_cast<char>(Numeric<unsigned char>(&(digits[0]), true, 8));
                  ss << temp;
                }
              } catch (const Numeric<unsigned char>::ValueOutOfRange &) {
                goto invalid_sequence;
              } catch (const Numeric<wchar_t>::ValueOutOfRange &) {
                goto invalid_sequence;
              }
              
              isOctal = false;
              pendingDigits = 0;
              filledDigits = 0;
              isEscape = false;
              goto done;
            }
          }
          
          goto done;
          
        invalid_sequence:
          {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, lineCount, "Invalid escape sequence: " + String(start, static_cast<size_t>(p - start + 1)));
          }
          
        done:
          {
            if (isEscape) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, lineCount, "Invalid string escape sequence: " + String(start, static_cast<size_t>(p - start + 1)));
            }
          }
          
          return ss.str();
        }

        //---------------------------------------------------------------------
        String Helper::decodeQuotes(
                                    const String &str,
                                    ULONG lineCount
                                    ) throw (FailureWithLine)
        {
          if (!isQuotes(str)) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, lineCount, "Failed to parse quoted string: " + str);
          }

          std::stringstream ss;

          const char *pos = str.c_str()+1;
          if (('L' == *(str.c_str())) ||
              ('u' == *(str.c_str()))) {
            ++pos;
          }
          const char *stopPos = str.c_str() + str.length() - 1;

          while (pos < stopPos) {
            char value = *pos;
            ++pos;

            if ('\\' == value) {
              ++pos;
              auto value = Helper::decodeCEscape(pos, lineCount);
              ss << value;
              continue;
            }

            ss << value;
            continue;
          }

          return ss.str();
        }

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
