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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_IDLCompiler.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructImplCpp.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructCx.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Helper.h>

#include <zsLib/eventing/tool/OutputStream.h>

#include <zsLib/eventing/IHelper.h>
#include <zsLib/eventing/IHasher.h>
#include <zsLib/eventing/IEventingTypes.h>

#include <zsLib/Exception.h>
#include <zsLib/Numeric.h>

#include <sstream>
#include <list>
#include <set>
#include <cctype>

#define ZS_WRAPPER_COMPILER_DIRECTIVE_EXCLUSIZE "EXCLUSIVE"

namespace zsLib { namespace eventing { namespace tool { ZS_DECLARE_SUBSYSTEM(zsLib_eventing_tool) } } }

namespace zsLib
{
  namespace eventing
  {
    ZS_DECLARE_TYPEDEF_PTR(IIDLTypes::Project, Project);

    namespace tool
    {
      ZS_DECLARE_TYPEDEF_PTR(eventing::tool::internal::Helper, UseHelper);
      ZS_DECLARE_TYPEDEF_PTR(eventing::IHasher, UseHasher);
      typedef std::set<String> HashSet;

      namespace internal
      {
        ZS_DECLARE_TYPEDEF_PTR(IDLCompiler::Token, Token);
        typedef IDLCompiler::TokenList TokenList;

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Helpers
        #pragma mark

        //---------------------------------------------------------------------
        static String getPathName(const IIDLTypes::ContextPtr &context)
        {
          if (!context) return String();

          String pathStr = context->getPathName();

          if ("::" == pathStr) return pathStr;
          if (0 != pathStr.find("::")) return pathStr;

          return pathStr.substr(strlen("::"));
        }

        //---------------------------------------------------------------------
        static void skipPreprocessor(
                                     const char * &p,
                                     ULONG &ioLineCount
                                     )
        {
          const char *startPos = p;
          
          while (true)
          {
            Helper::skipToEOL(p);

            // see if this preprocessor statement is multi-line
            while (p != startPos)
            {
              --p;
              if (('\n' == *p) ||
                  ('\r' == *p)) {
                break;
              }

              if (isspace(*p)) continue;

              if ('\\' == *p) {
                Helper::skipToEOL(p);
                if (Helper::skipEOL(p, &ioLineCount)) goto next_line;
              }
              Helper::skipToEOL(p);
              Helper::skipEOL(p, &ioLineCount);
              return;
            }

          next_line:
            {
            }
          }
        }

        //---------------------------------------------------------------------
        static bool isStructDeclaration(const String &str)
        {
          if ("struct" == str) return true;
          if ("interface" == str) return true;
          if ("class" == str) return true;
          if ("interaction" == str) return true;
          return false;
        }
        
        //---------------------------------------------------------------------
        static TokenPtr getCPPDirectiveToken(
                                             const char * &p,
                                             ULONG &ioLineCount
                                             )
        {
          if ('/' != *p) return TokenPtr();
          if ('/' != *(p+1)) return TokenPtr();
          if ('!' != *(p+2)) return TokenPtr();

          p += 3;

          const char *start = p;
          Helper::skipToEOL(p);

          String str(start, static_cast<size_t>(p - start));

          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Directive;
          result->mToken = str;
          result->mLineCount = ioLineCount;

          Helper::skipEOL(p, &ioLineCount);

          return result;
        }
        
        //---------------------------------------------------------------------
        static TokenPtr getCPPDocToken(
                                       const char * &p,
                                       ULONG &ioLineCount
                                       )
        {
          if ('/' != *p) return TokenPtr();
          if ('/' != *(p+1)) return TokenPtr();
          if ('/' != *(p+2)) return TokenPtr();
          
          p += 3;

          if (' ' == *p) ++p;

          const char *start = p;
          Helper::skipToEOL(p);
          
          String str(start, static_cast<size_t>(p - start));
          
          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Documentation;
          result->mToken = str + "\n";
          result->mLineCount = ioLineCount;

          Helper::skipEOL(p, &ioLineCount);

          return result;
        }
        
        //---------------------------------------------------------------------
        static TokenPtr getQuoteToken(
                                      const char * &p,
                                      ULONG &ioLineCount
                                      )
        {
          ULONG currentLine = ioLineCount;
          
          const char *start = p;
          if (!Helper::skipQuote(p, &ioLineCount)) return TokenPtr();

          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Quote;
          result->mToken = String(start, static_cast<size_t>(p - start));
          result->mLineCount = currentLine;
          return result;
        }

        //---------------------------------------------------------------------
        static TokenPtr getCharToken(
                                     const char * &p,
                                     ULONG &ioLineCount
                                     ) throw (FailureWithLine)
        {
          ULONG currentLine = ioLineCount;
          
          const char *start = p;
          if ('\'' != *p) return TokenPtr();
          
          ++p;
          if ('\\' == *p) {
            Helper::decodeCEscape(p, ioLineCount);
          } else {
            ++p;
          }
          if ('\'' != *p) return TokenPtr();
          ++p;

          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Char;
          result->mToken = String(start, static_cast<size_t>(p - start));
          result->mLineCount = currentLine;
          return result;
        }

        //---------------------------------------------------------------------
        static TokenPtr getNumberToken(
                                       const char * &p,
                                       ULONG lineCount
                                       )
        {
          const char *start = p;
          
          bool foundNegative = false;
          bool foundDot = false;
          bool foundExponent = false;

          if ('-' == *start) {
            foundNegative = true;
            ++start;
            Helper::skipWhitespaceExceptEOL(start);
          }

          if (!isdigit(*start)) return TokenPtr();

          p = start;
          
          int base = 10;
          
          if ('0' == *p) {
            switch (*(p+1)) {
              case 'x':
              case 'X': {
                base = 16;
                p += 2;
                break;
              }
              case 'b':
              case 'B': {
                base = 2;
                p += 2;
                break;
              }
              case '.': {
                break;
              }
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7': {
                base = 8;
                ++p;
                break;
              }
            }
          }

          while ('\0' != *p)
          {
            switch (*p)
            {
              case '.': {
                if (10 != base) goto check_exponent;
                ++p;
                foundDot = true;
                continue;
              }
              case '0':
              case '1': {
                ++p;
                continue;
              }
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7': {
                if (base >= 8) {
                  ++p;
                  continue;
                }
                goto check_exponent;
              }
              case '8':
              case '9': {
                if (base >= 10) {
                  ++p;
                  continue;
                }
                goto check_exponent;
              }
              case 'a':
              case 'A':
              case 'b':
              case 'B':
              case 'c':
              case 'C':
              case 'd':
              case 'D':
              case 'e':
              case 'E':
              case 'f':
              case 'F': {
                if (base >= 10) {
                  ++p;
                  continue;
                }
                goto check_exponent;
              }
              default: goto check_exponent;
            }
          }
          
        check_exponent:
          {
            const char *exponentStart = p;
            
            if (('e' != *p) &&
                ('E' != *p)) {
              goto check_postfix;
            }
            if (10 != base) goto check_postfix;

            foundExponent = true;
            ++p;
            
            bool foundExponentNumber = false;
            if (('-' == *p) ||
                ('+' == *p)) {
              ++p;
            }

            while (isdigit(*p)) {
              foundExponentNumber = true;
              ++p;
            }

            if (!foundExponentNumber) {
              // the 'e' does not belong to the number
              p = exponentStart;
              goto done;
            }
          }
          
        check_postfix:
          {
            const char *postFixStart = p;
            
            bool mUnsigned = false;
            bool mFloat = false;
            size_t foundLongs = 0;
            bool lastWasLong = false;
            
            while (true)
            {
              switch (*p) {
                case 'u':
                case 'U':
                {
                  if (mUnsigned) goto invalid_postfix;
                  if (mFloat) goto invalid_postfix;
                  mUnsigned = true;
                  goto not_long;
                }
                case 'l':
                case 'L':
                {
                  if (foundLongs > 0) {
                    if (mFloat) goto invalid_postfix;
                    if (!lastWasLong) goto invalid_postfix;
                  }
                  ++foundLongs;
                  if (foundLongs > 2) goto invalid_postfix;
                  ++p;
                  lastWasLong = true;
                  continue;
                }
                case 'f':
                case 'F':
                {
                  if (10 != base) goto invalid_postfix;
                  if (mUnsigned) goto invalid_postfix;
                  if (foundLongs > 1) goto invalid_postfix;
                  if (mFloat) goto invalid_postfix;
                  mFloat = true;
                  goto not_long;
                }
                default:
                {
                  goto done;
                }
              }
              
            not_long:
              {
                ++p;
                lastWasLong = false;
                continue;
              }
              
            invalid_postfix:
              {
                p = postFixStart;
                goto done;
              }
            }
          }
          
        done:
          {
          }
          
          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Number;
          result->mToken = String(start, static_cast<size_t>(p - start));
          if (foundNegative) {
            result->mToken = String("-") + result->mToken;
          }
          result->mLineCount = lineCount;
          return result;
        }

        //---------------------------------------------------------------------
        static TokenPtr getIdentifierToken(
                                           const char * &p,
                                           ULONG lineCount
                                           )
        {
          if ((!isalpha(*p)) &&
              ('_' != *p)) return TokenPtr();
          
          const char *start = p;
          
          while ((isalnum(*p)) ||
                 ('_' == *p)) {
            ++p;
          }
          
          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Identifier;
          result->mToken = String(start, static_cast<size_t>(p - start));
          result->mLineCount = lineCount;
          return result;
        }
        
        //---------------------------------------------------------------------
        static TokenPtr getOperatorToken(
                                         const char * &p,
                                         ULONG lineCount
                                         )
        {
          static const char *operators[] =
          {
            "{",
            "}",
            "(",
            ")",
            "[",
            "]",
            ";",
            "<",
            ">",
            "::",
            "=",
            ":",
            ",",
            "?",
            //"++",
            //"--",
            //".",
            //"->",
            //"~",
            //"!",
            //"+",
            //"-",
            //"&",
            //"*",
            //".*",
            //"->*",
            //"*",
            //"/",
            //"%",
            //"<<",
            //">>",
            //">=",
            //"<=",
            //"==",
            //"!=",
            //"^",
            //"|",
            //"&&",
            //"||",
            //"*=",
            //"/=",
            //"%=",
            //"+=",
            //"-=",
            //">>=",
            //"<<=",
            //"&=",
            //"^=",
            //"|=",
            NULL
          };

          String valid;
          String test;

          while ('\0' != *p) {
            test = valid;
            test += String(p, static_cast<size_t>(1));
            
            for (int index = 0; NULL != operators[index]; ++index)
            {
              if (test == operators[index]) goto next;
            }
            goto done;
            
          next:
            {
              valid = test;
              ++p;
            }
          }

        done:
          {
          }
          
          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Operator;
          if (";" == valid) {
            result->mTokenType = IDLCompiler::TokenType_SemiColon;
          } else if ("(" == valid) {
            result->mTokenType = IDLCompiler::TokenType_Brace;
          } else if (")" == valid) {
            result->mTokenType = IDLCompiler::TokenType_Brace;
          } else if ("{" == valid) {
            result->mTokenType = IDLCompiler::TokenType_CurlyBrace;
          } else if ("}" == valid) {
            result->mTokenType = IDLCompiler::TokenType_CurlyBrace;
          } else if ("[" == valid) {
            result->mTokenType = IDLCompiler::TokenType_SquareBrace;
          } else if ("]" == valid) {
            result->mTokenType = IDLCompiler::TokenType_SquareBrace;
          } else if ("<" == valid) {
            result->mTokenType = IDLCompiler::TokenType_AngleBrace;
          } else if (">" == valid) {
            result->mTokenType = IDLCompiler::TokenType_AngleBrace;
          } else if ("::" == valid) {
            result->mTokenType = IDLCompiler::TokenType_ScopeOperator;
          } else if ("," == valid) {
            result->mTokenType = IDLCompiler::TokenType_CommaOperator;
          } else if (":" == valid) {
            result->mTokenType = IDLCompiler::TokenType_ColonOperator;
          } else if ("=" == valid) {
            result->mTokenType = IDLCompiler::TokenType_EqualsOperator;
          }

          result->mToken = valid;
          result->mLineCount = lineCount;
          return result;
        }
        
        //---------------------------------------------------------------------
        static TokenPtr getUnknownToken(
                                        const char * &p,
                                        ULONG lineCount
                                        )
        {
          if (!p) return TokenPtr();
          
          if ('\0' == *p) return TokenPtr();
          
          auto result = make_shared<Token>();
          result->mTokenType = IDLCompiler::TokenType_Unknown;
          result->mToken = String(p, static_cast<size_t>(1));
          result->mLineCount = lineCount;
          ++p;
          return result;
        }

        //---------------------------------------------------------------------
        static TokenPtr getNextToken(
                                     const char * &p,
                                     bool &ioStartOfLine,
                                     ULONG &ioLineCount
                                     )
        {
          if (!p) return TokenPtr();
          
          while ('\0' != *p)
          {
            ULONG activeLine = ioLineCount;
            
            if (Helper::skipWhitespaceExceptEOL(p)) continue;
            if (Helper::skipEOL(p, &ioLineCount)) {
              ioStartOfLine = true;
              continue;
            }
            
            if (ioStartOfLine) {
              if ('#' == *p) {
                skipPreprocessor(p, ioLineCount);
                ioStartOfLine = true;
                continue;
              }
            }

            if (Helper::skipCComments(p, &ioLineCount)) {
              if (activeLine != ioLineCount) ioStartOfLine = true;
              continue;
            }

            {
              auto result = getCPPDirectiveToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = true;
                return result;
              }
            }

            {
              auto result = getCPPDocToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = true;
                return result;
              }
            }

            if (Helper::skipCPPComments(p)) {
              Helper::skipEOL(p, &ioLineCount);
              ioStartOfLine = true;
              continue;
            }
            
            {
              auto result = getQuoteToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = false;
                return result;
              }
            }
            
            {
              auto result = getNumberToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = false;
                return result;
              }
            }
            
            {
              auto result = getIdentifierToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = false;
                return result;
              }
            }
            
            {
              auto result = getOperatorToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = false;
                return result;
              }
            }
            
            {
              auto result = getUnknownToken(p, ioLineCount);
              if (result) {
                ioStartOfLine = false;
                return result;
              }
            }
          }

          return TokenPtr();
        }

        //---------------------------------------------------------------------
        void tokenize(
                      const char *p,
                      TokenList &outTokens,
                      ULONG startLineNumber = 1
                      )
        {
          bool startOfLine = true;
          ULONG lineCount = startLineNumber;
          
          while (true)
          {
            auto token = getNextToken(p, startOfLine, lineCount);
            if (!token) break;

            outTokens.push_back(token);
          }
        }
        
        //---------------------------------------------------------------------
        void replaceAliases(
                            TokenList &ioTokens,
                            const IEventingTypes::AliasMap &aliases
                            )
        {
          for (auto iter_doNotUse = ioTokens.begin(); iter_doNotUse != ioTokens.end(); )
          {
            auto current = iter_doNotUse;
            ++iter_doNotUse;
            
            auto token = (*current);
            auto found = aliases.find(token->mToken);
            if (found == aliases.end()) continue;

            TokenList replacementTokens;
            tokenize((*found).second.c_str(), replacementTokens, token->mLineCount);
            
            for (auto iterReplace = replacementTokens.rbegin(); iterReplace != replacementTokens.rend(); ++iterReplace)
            {
              auto replaceToken = (*iterReplace);
              ioTokens.insert(current, replaceToken);
            }

            ioTokens.erase(current);
          }
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IDLCompiler
        #pragma mark

        //---------------------------------------------------------------------
        bool IDLCompiler::Token::isBrace() const
        {
          switch (mTokenType) {
            case TokenType_Brace:
            case TokenType_CurlyBrace:
            case TokenType_SquareBrace:
            case TokenType_AngleBrace:    return true;
            default:                      break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::Token::isOpenBrace() const
        {
          switch (mTokenType) {
            case TokenType_Brace:         return "(" == mToken;
            case TokenType_CurlyBrace:    return "{" == mToken;
            case TokenType_SquareBrace:   return "[" == mToken;
            case TokenType_AngleBrace:    return "<" == mToken;
            default:                      break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::Token::isCloseBrace() const
        {
          switch (mTokenType) {
            case TokenType_Brace:         return ")" == mToken;
            case TokenType_CurlyBrace:    return "}" == mToken;
            case TokenType_SquareBrace:   return "]" == mToken;
            case TokenType_AngleBrace:    return ">" == mToken;
            default:                      break;
          }
          return false;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::Token::isOpenBrace(TokenTypes type) const
        {
          if (!isOpenBrace()) return false;
          return type == mTokenType;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::Token::isCloseBrace(TokenTypes type) const
        {
          if (!isCloseBrace()) return false;
          return type == mTokenType;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::Token::isIdentifier(const char *identifier) const
        {
          if (TokenType_Identifier != mTokenType) return false;
          return identifier == mToken;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IDLCompiler
        #pragma mark

        //---------------------------------------------------------------------
        IDLCompiler::IDLCompiler(
                                 const make_private &,
                                 const Config &config
                                 ) :
          mConfig(config)
        {
        }

        //---------------------------------------------------------------------
        IDLCompiler::~IDLCompiler()
        {
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IDLCompiler => ICompiler
        #pragma mark

        //---------------------------------------------------------------------
        IDLCompilerPtr IDLCompiler::create(const Config &config)
        {
          IDLCompilerPtr pThis(std::make_shared<IDLCompiler>(make_private{}, config));
          pThis->mThisWeak = pThis;
          return pThis;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::process() throw (Failure, FailureWithLine)
        {
          outputSkeleton();
          read();
          validate();
          if ((mConfig.mOutputName.hasData()) &&
              (mConfig.mProject)) {
            String pathStr = mConfig.mOutputName;
            pathStr.trimRight("/\\");
            pathStr += "/";

            for (auto iter = mConfig.mIDLOutputs.begin(); iter != mConfig.mIDLOutputs.end(); ++iter)
            {
              auto target = (*iter).second;
              target->targetOutput(pathStr, mConfig);
            }
          }
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IDLCompiler => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        void IDLCompiler::outputSkeleton()
        {
        }

        //---------------------------------------------------------------------
        void IDLCompiler::read() throw (Failure, FailureWithLine)
        {
          HashSet processedHashes;

          ProjectPtr &project = mConfig.mProject;

          StringList prioritySourceFiles = mConfig.mSourceFiles;
          StringList sourceFiles;
          prioritySourceFiles.push_front(mConfig.mConfigFile);

          while ((sourceFiles.size() > 0) ||
                 (prioritySourceFiles.size() > 0))
          {
            StringList *useList = &sourceFiles;

            if (prioritySourceFiles.size() > 0) {
              useList = &prioritySourceFiles;
            }
            String fileName = (*useList).front();
            (*useList).pop_front();

            SecureByteBlockPtr file;
            try {
              file = UseHelper::loadFile(fileName);
            } catch (const StdError &e) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load file: ") + fileName + ", error=" + string(e.result()) + ", reason=" + e.message());
            }
            if (!file) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load file: ") + fileName);
            }
            auto hashResult = UseHasher::hashAsString(file);
            auto found = processedHashes.find(hashResult);
            if (found != processedHashes.end()) {
              tool::output() << "[Info] Duplicate file found thus ignoring: " << fileName << "\n";
              continue;
            }
            const char *fileAsStr = reinterpret_cast<const char *>(file->BytePtr());
            auto isJSON = Helper::isLikelyJSON(fileAsStr);

            if (isJSON) {
              try {
                tool::output() << "\n[Info] Reading JSON configuration: " << fileName << "\n\n";
                auto rootEl = UseHelper::read(file);
                if (!rootEl) {
                  ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load file as JSON: ") + fileName);
                }
                if (!project) {
                  project = Project::create(rootEl);
                } else {
                  project->parse(rootEl);
                }

                StringList sources = sourceFiles;
                sourceFiles.clear();

                ElementPtr sourcesEl = rootEl->findFirstChildElement("includes");
                if (sourcesEl) {
                  ElementPtr sourceEl = sourcesEl->findFirstChildElement("include");
                  while (sourceEl) {
                    auto source = UseHelper::getElementTextAndDecode(sourceEl);

                    if (source.hasData()) {
                      sourceFiles.push_back(UseHelper::fixRelativeFilePath(fileName, source));
                    }
                    sourceEl = sourceEl->findNextSiblingElement("include");
                  }
                }

                ElementPtr includesEl = rootEl->findFirstChildElement("sources");
                if (includesEl) {
                  ElementPtr includeEl = includesEl->findFirstChildElement("source");
                  while (includeEl) {
                    auto source = UseHelper::getElementTextAndDecode(includeEl);

                    if (source.hasData()) {
                      sourceFiles.push_back(UseHelper::fixRelativeFilePath(fileName, source));
                    }
                    includeEl = includeEl->findNextSiblingElement("source");
                  }
                }

                // put back the original configuration files
                for (auto iter = sources.begin(); iter != sources.end(); ++iter) {
                  sourceFiles.push_back(*iter);
                }

              } catch (const InvalidContent &e) {
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Failed to parse JSON configuration: " + e.message());
              }
              continue;
            }

            if (!project) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Project configuration is missing!");
            }

            tool::output() << "\n[Info] Reading C/C++ source file: " << fileName << "\n\n";

            try {
              const char *pos = reinterpret_cast<const char *>(file->BytePtr());

              mTokenListStack = TokenListStack();

              pushTokens(make_shared<TokenList>());

              tokenize(pos, *getTokens());

              replaceAliases(*getTokens(), project->mAliases);

              if (!project->mGlobal) {
                project->mGlobal = Namespace::create(project);
              }

              parseNamespaceContents(project->mGlobal);

            } catch (const InvalidContent &e) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Invalid content found: " + e.message());
            } catch (const InvalidContentWithLine &e) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, e.lineNumber(), "Invalid content found: " + e.message());
            }
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::validate() throw (Failure)
        {
          auto &project = mConfig.mProject;
          if (!project) return;

          project->fixTemplateHashMapping();

//          if (project->mUniqueHash.isEmpty()) {
//            project->mUniqueHash = project->uniqueEventingHash();
//          }
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseNamespace(NamespacePtr parent) throw (FailureWithLine)
        {
          const char *what = "namespace";
          auto token = peekNextToken(what);
          if (!token->isIdentifier("namespace")) return false;
          extractNextToken(what);  // skip "namespace"

          token = extractNextToken(what);
          
          if (TokenType_Identifier != token->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " missing identifier");
          }
          
          String namespaceStr = token->mToken;

          token = extractNextToken(what);

          if (!token->isOpenBrace(TokenType_CurlyBrace)) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("namespace expecting \"{\""));
          }

          NamespacePtr namespaceObj;

          {
            auto found = parent->mNamespaces.find(namespaceStr);
            if (found == parent->mNamespaces.end()) {
              namespaceObj = Namespace::create(parent);
              namespaceObj->mName = namespaceStr;
              parent->mNamespaces[namespaceStr] = namespaceObj;

              tool::output() << "[Info] Found namespace: " << getPathName(namespaceObj) << "\n";

            } else {
              namespaceObj = (*found).second;
            }
          }

          fillContext(namespaceObj);

          parseNamespaceContents(namespaceObj);

          token = extractNextToken(what);
          if (!token->isCloseBrace(TokenType_CurlyBrace)) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("namespace expecting \"}\""));
          }

          return true;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::parseNamespaceContents(NamespacePtr namespaceObj) throw (FailureWithLine)
        {
          //const char *what = "namespace";
          while (hasMoreTokens()) {
            if (parseDocumentation()) continue;
            if (parseSemiColon()) continue;
            if (parseDirective()) continue;
            if (parseModifiers()) continue;
            if (parseNamespace(namespaceObj)) continue;
            if (parseUsing(namespaceObj)) continue;
            if (parseEnum(namespaceObj)) continue;
            if (parseTypedef(namespaceObj)) continue;
            if (parseStruct(namespaceObj)) continue;

            break;
          }
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseUsing(NamespacePtr namespaceObj) throw (FailureWithLine)
        {
          const char *what = "using";
          auto token = peekNextToken(what);
          if (!token->isIdentifier("using")) return false;
          extractNextToken(what);  // skip "using"

          token = peekNextToken(what);
          if (token->isIdentifier("namespace")) {
            extractNextToken(what);  // skip "namespace"

            // extract until ";" found
            String namespacePathStr;
              
            token = extractNextToken(what);
            while (TokenType_SemiColon != token->mTokenType) {
              namespacePathStr += token->mToken;
              token = extractNextToken(what); // skip it
            }

            auto foundNamespace = namespaceObj->findNamespace(namespacePathStr);
            if (!foundNamespace) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("using namespace was not found:") + namespacePathStr);
            }

            tool::output() << "[Info] Found using namespace: " << getPathName(foundNamespace) << "\n";

            processUsingNamespace(namespaceObj, foundNamespace);
            return true;
          }

          // extract until ";" found
          String typePathStr;

          token = peekNextToken(what);
          while (TokenType_SemiColon != token->mTokenType) {
            extractNextToken(what); // skip it
            typePathStr += token->mToken;
          }

          auto foundType = namespaceObj->toContext()->findType(typePathStr);
          if (!foundType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("using type was not found:") + typePathStr);
          }

          tool::output() << "[Info] Found using type: " << getPathName(foundType) << "\n";

          processUsingType(namespaceObj, foundType);
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseTypedef(ContextPtr context) throw (FailureWithLine)
        {
          const char *what = "typedef";
          auto token = peekNextToken(what);
          if (!token->isIdentifier("typedef")) return false;
          extractNextToken(what);  // skip "typedef"
          
          TokenList typeTokens;

          token = extractNextToken(what);
          while (TokenType_SemiColon != token->mTokenType) {
            typeTokens.push_back(token);
            token = extractNextToken(what);
          }

          if (typeTokens.size() < 2) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("typedef typename was not found"));
          }

          TokenPtr lastToken = typeTokens.back();
          typeTokens.pop_back();
          
          if (TokenType_Identifier != lastToken->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("typedef identifier was not found"));
          }
          
          String typeName = lastToken->mToken;

          tool::output() << "[Info] Found typedef: " << typeName << "\n";

          processTypedef(context, typeTokens, typeName);
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseStruct(ContextPtr context) throw (FailureWithLine)
        {
          const char *what = "interface/struct";
          
          auto token = peekNextToken(what);
          if (TokenType_Identifier != token->mTokenType) return false;

          bool foundTemplate = false;
          TokenList templateTokens;
          if (token->isIdentifier("template")) {
            extractNextToken(what);  // skip "template"
            foundTemplate = true;

            if (!extractToClosingBraceToken(what, templateTokens, false)) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " template expecting arguments");
            }

            token = peekNextToken(what);  // get type of struct/internface
          }

          if (!isStructDeclaration(token->mToken)) {
            if (foundTemplate) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " template expecting keyword struct or interface");
            }
            return false;
          }
          
          extractNextToken(what); // skip "struct/interface" keyword

          token = extractNextToken(what);

          String structName = token->mToken;

          if (TokenType_Identifier != token->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting name identifier");
          }

          token = peekNextToken(what);
          if (TokenType_SemiColon == token->mTokenType) {
            if (foundTemplate) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " template is missing template body");
            }
            processStructForward(context, structName);
            return true;
          }

          bool created {};
          auto newStruct = processStructForward(context, structName, &created);
          if (!created) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " struct/interface was not created: " + structName);
          }

          tool::output() << "[Info] Found struct: " << getPathName(newStruct) << (foundTemplate ? " (template)" : "") << "\n";

          if (foundTemplate) {
            bool foundDefault = false;

            pushTokens(templateTokens);
            while (hasMoreTokens()) {
              if (parseComma()) continue;
              token = extractNextToken(what); // get generic name
              
              if (TokenType_Identifier != token->mTokenType) {
                ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " template expecting generic name");
              }

              auto genericType = GenericType::create(newStruct);
              genericType->mName = token->mToken;
              
              TypePtr defaultType;

              TokenList typeTokens;
              if (hasMoreTokens()) {
                token = peekNextToken(what);
                if (TokenType_EqualsOperator == token->mTokenType) {
                  extractNextToken(what); // skip "="
                  extractToComma(what, typeTokens);
                  TypedefTypePtr createdTypedef;
                  defaultType = findTypeOrCreateTypedef(newStruct, typeTokens, createdTypedef);
                  foundDefault = true;
                }
              }
              
              if ((foundDefault) &&
                  (!defaultType)) {
                ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " template expecting default type");
              }

              newStruct->mGenerics.push_back(genericType);
              newStruct->mGenericDefaultTypes.push_back(defaultType);
            }
            popTokens(); //templateTokens
          }

          token = peekNextToken(what);
          
          if (TokenType_ColonOperator == token->mTokenType) {
            extractNextToken(what); // skip ":"

            TokenList inheritTypeTokens;

            token = peekNextToken(what);
            while (TokenType_CurlyBrace != token->mTokenType) {
              if (parseComma()) {
                if (inheritTypeTokens.size() < 1) {
                  ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting related type name");
                }
                processRelated(newStruct, inheritTypeTokens);
                inheritTypeTokens.clear();
                goto next;
              }

              inheritTypeTokens.push_back(token);
              extractNextToken(what); // skip token;

            next:
              {
                token = peekNextToken(what);
                continue;
              }
            }

            if (inheritTypeTokens.size() < 1) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting related type name");
            }
            processRelated(newStruct, inheritTypeTokens);
          }

          if (!token->isOpenBrace(TokenType_CurlyBrace)) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " template expecting generic name");
          }
          
          TokenList structTokens;
          extractToClosingBraceToken(what, structTokens);
          
          pushTokens(structTokens);

          TokenTypeSet searchTypes;
          searchTypes.insert(TokenType_SemiColon);
          searchTypes.insert(TokenType_EqualsOperator);
          searchTypes.insert(TokenType_Brace);
          searchTypes.insert(TokenType_CurlyBrace);

          while (hasMoreTokens()) {
            if (parseDocumentation()) continue;
            if (parseSemiColon()) continue;
            if (parseDirective()) continue;
            if (parseModifiers()) continue;
            if (parseEnum(newStruct)) continue;
            if (parseTypedef(newStruct)) continue;
            if (parseStruct(newStruct)) continue;

            {
              auto foundToken = peekAheadToFirstTokenOfType(searchTypes);
              if (!foundToken) goto did_not_find_peek_ahead_token;

              switch (foundToken->mTokenType)
              {
                case TokenType_SemiColon:
                case TokenType_EqualsOperator:
                case TokenType_CurlyBrace:
                {
                  parseProperty(newStruct);
                  break;
                }
                case TokenType_Brace:
                {
                  parseMethod(newStruct);
                  break;
                }
                default:  goto did_not_find_peek_ahead_token;
              }
              continue;
            }

          did_not_find_peek_ahead_token:
            {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a peek ahead token to be found that does not exist of type \"{}()=;\"");
            }
          }
          
          popTokens();  // structTokens

          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseEnum(ContextPtr context) throw (FailureWithLine)
        {
          const char *what = "enum";

          auto token = peekNextToken(what);
          if (!token->isIdentifier("enum")) return false;
          extractNextToken(what); // skip "enum"

          token = extractNextToken(what);
          if (TokenType_Identifier != token->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting an identifier name");
          }

          auto enumType = EnumType::create(context);
          fillContext(enumType);
          enumType->mName = token->mToken;

          tool::output() << "[Info] Found enum: " << getPathName(enumType) << "\n";

          token = peekNextToken(what);
          if (TokenType_ColonOperator == token->mTokenType) {
            extractNextToken(what); // skip ":"

            TokenList enumTypeTokens;
            extractToTokenType(what, TokenType_CurlyBrace, enumTypeTokens);

            TypedefTypePtr createdTypedef;
            auto existingType = findTypeOrCreateTypedef(enumType, enumTypeTokens, createdTypedef);
            if (!existingType) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a type to resolve");
            }
            auto basicType = existingType->toBasicType();
            if (!basicType) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " only supports deriving from basic types (and generics in templates are not supported either)");
            }
            enumType->mBaseType = basicType->mBaseType;
          }

          token = extractNextToken(what);
          if (!token->isOpenBrace(TokenType_CurlyBrace)) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " is missing definition");
          }

          bool foundEndBrace = false;

          while (hasMoreTokens()) {
            if (parseDocumentation()) continue;
            if (parseComma()) continue;

            token = extractNextToken(what);
            if (token->isCloseBrace(TokenType_CurlyBrace)) {
              foundEndBrace = true;
              break;
            }

            if (TokenType_Identifier != token->mTokenType) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " is expecting an identifier");
            }

            auto enumValue = EnumTypeValue::create(enumType);
            fillContext(enumValue);
            enumValue->mName = token->mToken;

            token = peekNextToken(what);
            if (TokenType_EqualsOperator == token->mTokenType) {
              extractNextToken(what); // skip equals

              token = extractNextToken(what);
              enumValue->mValue = token->mToken;
            }

            tool::output() << "[Info] Found enum value: " << getPathName(enumType) << " -> " << enumValue->mName << (enumValue->mValue.hasData() ? (String("=") + enumValue->mValue) : String()) << "\n";

            enumType->mValues.push_back(enumValue);
          }

          if (!foundEndBrace) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " is missing closing brace");
          }

          {
            auto namespaceObj = context->toNamespace();
            if (namespaceObj) {
              namespaceObj->mEnums[enumType->getMappingName()] = enumType;
            }
          }

          {
            auto structObj = context->toStruct();
            if (structObj) {
              structObj->mEnums[enumType->getMappingName()] = enumType;
            }
          }

          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseProperty(StructPtr context) throw (FailureWithLine)
        {
          const char *what = "property";

          TokenList typeTokens;

          TokenPtr nameToken;
          TokenPtr stopToken;

          while (hasMoreTokens())
          {
            if (parseDocumentation()) continue;

            auto token = extractNextToken(what);
            if ((TokenType_SemiColon == token->mTokenType) ||
                (TokenType_EqualsOperator == token->mTokenType) ||
                (TokenType_CurlyBrace == token->mTokenType)) {
              stopToken = token;
              break;
            }
            if (nameToken) {
              typeTokens.push_back(nameToken);
            }
            nameToken = token;
          }

          if (!stopToken) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting more tokens");
          }
          if (!nameToken) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a property name");
          }
          if (TokenType_Identifier != nameToken->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a property name to be an identifier but found: " + nameToken->mToken);
          }
          if (typeTokens.size() < 1) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting property types");
          }

          auto property = Property::create(context);          
          fillContext(property);
          property->mName = nameToken->mToken;

          tool::output() << "[Info] Found property: " << getPathName(context) << " -> " << property->mName << "\n";

          TypedefTypePtr createdTypedef;
          property->mType = findTypeOrCreateTypedef(property, typeTokens, createdTypedef);
          if (!property->mType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " did not find valid property type");
          }

          if ((TokenType_EqualsOperator == stopToken->mTokenType) ||
              (stopToken->isOpenBrace(TokenType_CurlyBrace))) {

            auto defaultValueToken = extractNextToken(what);
            property->mDefaultValue = defaultValueToken->mToken;

            if (stopToken->isOpenBrace(TokenType_CurlyBrace)) {
              auto finalToken = extractNextToken(what);
              if (!finalToken->isCloseBrace(TokenType_CurlyBrace)) {
                ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " did not find default value closing brace \"}\"");
              }
            }
          }

          context->mProperties.push_back(property);
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseMethod(StructPtr context) throw (FailureWithLine)
        {
          const char *what = "method";
          TokenList typeTokens;

          TokenPtr nameToken;
          TokenPtr stopToken;

          while (hasMoreTokens())
          {
            if (parseDocumentation()) continue;

            auto token = extractNextToken(what);
            if (token->isOpenBrace(TokenType_Brace)) {
              stopToken = token;
              break;
            }

            if (nameToken) {
              typeTokens.push_back(nameToken);
            }
            nameToken = token;
          }

          if (!stopToken) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting more tokens");
          }
          if (!nameToken) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a method name");
          }
          if (TokenType_Identifier != nameToken->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a method name to be an identifier but found: " + nameToken->mToken);
          }
          if (typeTokens.size() < 1) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting method return type");
          }

          auto method = Method::create(context);
          fillContext(method);
          method->mName = nameToken->mToken;

          tool::output() << "[Info] Found method: " << getPathName(context) << " -> " << method->mName << "\n";

          TypedefTypePtr createdTypedef;
          method->mResult = findTypeOrCreateTypedef(method, typeTokens, createdTypedef);
          if (!method->mResult) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " did not find valid property type");
          }

          putBackToken(stopToken);

          TokenList argumentsTokens;
          extractToClosingBraceToken(what, argumentsTokens);

          pushTokens(argumentsTokens);

          while (hasMoreTokens())
          {
            if (parseComma()) continue;
            TokenList argumentTokens;
            extractToComma(what, argumentTokens);
            
            pushTokens(argumentTokens);

            TokenPtr argumentNameToken;
            TokenList argumentTypeTokens;
            while (hasMoreTokens())
            {
              if (parseDocumentation()) continue;
              if (parseModifiers()) continue;

              auto token = extractNextToken(what);
              if (argumentNameToken) {
                argumentTypeTokens.push_back(argumentNameToken);
              }
              argumentNameToken = token;
            }
            popTokens();

            if (!argumentNameToken) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting an argument name");
            }
            if (TokenType_Identifier != argumentNameToken->mTokenType) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a argument name to be an identifier but found: " + nameToken->mToken);
            }
            if (argumentTypeTokens.size() < 1) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting an argument type");
            }

            auto property = Property::create(context);
            fillContext(property);
            property->mName = argumentNameToken->mToken;
            TypedefTypePtr createdTypedef;
            property->mType = findTypeOrCreateTypedef(method, argumentTypeTokens, createdTypedef);
            if (!property->mType) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " did not find valid method argument type");
            }
            method->mArguments.push_back(property);
          }

          popTokens();

          auto token = peekNextToken(what);

          if (token->isIdentifier("throws")) {
            extractNextToken(what); // skip "throws"

            token = peekNextToken(what);
            if (!token->isOpenBrace(TokenType_Brace)) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting a list of thrown objects");
            }

            TokenList throwTokens;
            extractToClosingBraceToken(what, throwTokens);

            pushTokens(throwTokens);

            while (hasMoreTokens())
            {
              if (parseComma()) continue;

              TokenList typeTokens;
              extractToComma(what, typeTokens);

              TypedefTypePtr createdTypedef;
              auto type = findTypeOrCreateTypedef(context, typeTokens, createdTypedef);
              method->mThrows.push_back(type);
            }

            popTokens();
          }

          context->mMethods.push_back(method);
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseDocumentation()
        {
          bool found = false;

          while (hasMoreTokens()) {
            auto token = peekNextToken("documentation");
            if (TokenType_Documentation != token->mTokenType) return found;

            found = true;
            mPendingDocumentation.push_back(extractNextToken("documentation"));
          }

          return found;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseSemiColon()
        {
          auto token = peekNextToken(";");

          if (TokenType_SemiColon != token->mTokenType) return false;
          extractNextToken(";");
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseComma()
        {
          const char *what = ",";
          auto token = peekNextToken(what);

          if (TokenType_CommaOperator != token->mTokenType) return false;
          extractNextToken(what);
          return true;
        }
        
        //---------------------------------------------------------------------
        bool IDLCompiler::parseModifiers() throw (FailureWithLine)
        {
          const char *what = "modifiers";
          
          auto token = peekNextToken(what);
          if (!token->isOpenBrace(TokenType_SquareBrace)) return false;

          TokenList allModifierTokens;
          extractToClosingBraceToken(what, allModifierTokens);

          pushTokens(allModifierTokens);
          
          while (hasMoreTokens()) {
            if (parseComma()) continue; // skip over comma
            TokenList modifierTokens;
            extractToComma(what, modifierTokens);

            pushTokens(modifierTokens);

            token = extractNextToken(what);
            if (TokenType_Identifier != token->mTokenType) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting identifier");
            }
            
            String modiferNameStr = token->mToken;
            modiferNameStr.toLower();
            
            try {
              auto modifier = toModifier(modiferNameStr);
              auto totalParams = getTotalParams(modifier);

              StringList values;

              if (hasMoreTokens()) {
                TokenList modifierParamTokens;
                extractToClosingBraceToken(what, modifierParamTokens);

                pushTokens(modifierParamTokens);

                while (hasMoreTokens()) {
                  if (parseComma()) continue;

                  TokenList paramTokens;
                  extractToComma(what, paramTokens);

                  std::stringstream value;
                  bool added {false};

                  pushTokens(paramTokens);
                  while (hasMoreTokens()) {
                    auto token = extractNextToken(what);
                    if (added) {
                      value << " ";
                    }
                    value << token->mToken;
                    added = true;
                  }
                  popTokens();  // paramTokens

                  values.push_back(value.str());
                }
                popTokens(); // modifierParamTokens
              } else {
                if (totalParams > 0) {
                  ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting parameters");
                }
              }
              
              if (-1 != totalParams) {
                if (totalParams != values.size()) {
                  ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting total parameter mismatch: " + string(totalParams) + ", found=" + string(values.size()));
                }
              }

              if (mPendingModifiers.end() != mPendingModifiers.find(modiferNameStr)) {
                ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " modifier is already set:" + modiferNameStr);
              }


              String valuesStr = UseHelper::combine(values, ";");

              tool::output() << "[Info] Found modifier: " << modiferNameStr << (valuesStr.hasData() ? " -> " : "") << valuesStr << "\n";
              mPendingModifiers[modiferNameStr] = values;

            } catch (const InvalidArgument &) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " modifier is not recognized:" + token->mToken);
            }

            popTokens();  // modifierTokens
          }

          popTokens();  // allModifierTokens
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseDirective() throw (FailureWithLine)
        {
          const char *what = "directive";
          auto token = peekNextToken(what);

          if (TokenType_Directive != token->mTokenType) return false;
          extractNextToken(what); // extract directive

          pushDirectiveTokens(token);

          {
            bool ignoreMode = false;
            do {
              if (!parseDirectiveExclusive(ignoreMode)) goto done;

              if (!ignoreMode) goto done;
              popTokens();

              while (hasMoreTokens()) {
                token = extractNextToken(what);
                if (pushDirectiveTokens(token)) goto check_exclusive_again;
              }
              break;

            check_exclusive_again:
              {
              }

            } while (ignoreMode);
          }

        done:
          {
          }

          popTokens();

          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::pushDirectiveTokens(TokenPtr token) throw (FailureWithLine)
        {
          if (!token) return false;
          if (TokenType_Directive != token->mTokenType) return false;

          TokenList tokens;
          tokenize(token->mToken.c_str(), tokens, token->mLineCount);

          pushTokens(tokens);
          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::parseDirectiveExclusive(bool &outIgnoreMode) throw (FailureWithLine)
        {
          const char *what = "Directive " ZS_WRAPPER_COMPILER_DIRECTIVE_EXCLUSIZE;
          auto token = peekNextToken(what);

          if (!token->isIdentifier(ZS_WRAPPER_COMPILER_DIRECTIVE_EXCLUSIZE)) return false;

          outIgnoreMode = true;

          extractNextToken(what); // extract exclusive directive

          token = extractNextToken(what);
          if (TokenType_Identifier != token->mTokenType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " expecting identifier");
          }

          String exclusiveId = token->mToken;

          if ((0 == exclusiveId.compareNoCase("x")) ||
              (mConfig.mProject->mDefinedExclusives.end() != mConfig.mProject->mDefinedExclusives.find(exclusiveId)))
          {
            outIgnoreMode = false;
          }

          tool::output() << "[Info] Found exclusive directive: " << exclusiveId << (outIgnoreMode ? " (ignoring content that follows)" : "" ) << "\n";
          return true;
        }

        //---------------------------------------------------------------------
        ElementPtr IDLCompiler::getDocumentation()
        {
          if (mPendingDocumentation.size() < 1) return ElementPtr();

          String resultStr = "<documentation>";
          bool first = true;
          while (mPendingDocumentation.size() > 0) {
            auto token = mPendingDocumentation.front();
            if (!first) {
              resultStr += " ";
            }

            resultStr += token->mToken;
            first = false;

            mPendingDocumentation.pop_front();
          }

          resultStr += "</documentation>";
          return UseHelper::toXML(resultStr);
        }

        //---------------------------------------------------------------------
        ElementPtr IDLCompiler::getDirectives()
        {
          if (mPendingDirectives.size() < 1) return ElementPtr();
          
          ElementPtr rootEl = Element::create("directives");

          while (mPendingDirectives.size() > 0) {
            auto el = mPendingDirectives.front();
            rootEl->adoptAsLastChild(el);
            mPendingDirectives.pop_front();
          }

          return rootEl;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::mergeDocumentation(ElementPtr &existingDocumentation)
        {
          auto rootEl = getDocumentation();
          if (!rootEl) return;

          if (!existingDocumentation) {
            existingDocumentation = rootEl;
            return;
          }
          
          auto childEl = rootEl->getFirstChild();
          while (childEl) {
            auto nextEl = childEl->getNextSibling();
            childEl->orphan();
            existingDocumentation->adoptAsLastChild(childEl);
            childEl = nextEl;
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::mergeDirectives(ElementPtr &existingDirectives)
        {
          if (mPendingDirectives.size() < 1) return;

          if (!existingDirectives) {
            existingDirectives = getDirectives();
            return;
          }

          while (mPendingDirectives.size() > 0) {
            auto el = mPendingDirectives.front();
            existingDirectives->adoptAsLastChild(el);
            mPendingDirectives.pop_front();
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::mergeModifiers(ContextPtr context) throw (FailureWithLine)
        {
          const char *what = "merge modifiers";
          
          if (!context) return;
          
          for (auto iter = mPendingModifiers.begin(); iter != mPendingModifiers.end(); ++iter)
          {
            auto &name = (*iter).first;
            auto &values = (*iter).second;
            auto modifier = toModifier(name);
            if (context->hasModifier(modifier)) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_UNEXPECTED_EOF, getLastLineNumber(), String(what) + " has duplicate modifier: " + name);
            }
            context->setModifier(modifier, values);
          }

          mPendingModifiers.clear();
        }

        //---------------------------------------------------------------------
        void IDLCompiler::fillContext(ContextPtr context)
        {
          if (!context) return;
          mergeDocumentation(context->mDocumentation);
          mergeModifiers(context);
        }

        //---------------------------------------------------------------------
        String IDLCompiler::makeTypenameFromTokens(const TokenList &tokens) throw (InvalidContent)
        {
          String result;
          
          bool lastWasIdentifier = false;
          bool lastWasScope = false;
          
          for (auto iter = tokens.begin(); iter != tokens.end(); ++iter)
          {
            auto token = (*iter);
            if (TokenType_Identifier == token->mTokenType) {
              if (lastWasIdentifier) {
                ZS_THROW_CUSTOM(InvalidContent, "two identifiers found");
              }
              result += token->mToken;
              lastWasIdentifier = true;
              lastWasScope = false;
            } else if (TokenType_ScopeOperator == token->mTokenType) {
              if (lastWasScope) {
                ZS_THROW_CUSTOM(InvalidContent, "two scopes found");
              }
              result += token->mToken;
              lastWasIdentifier = false;
              lastWasScope = true;
            }
          }
          
          return result;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::pushTokens(const TokenList &tokens)
        {
          mTokenListStack.push(make_shared<TokenList>(tokens));
          if (tokens.size() > 0) {
            mLastTokenStack.push(tokens.front());
          } else {
            mLastTokenStack.push(TokenPtr());
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::pushTokens(TokenListPtr tokens)
        {
          mTokenListStack.push(tokens);
          if (tokens->size() > 0) {
            mLastTokenStack.push(tokens->front());
          } else {
            mLastTokenStack.push(TokenPtr());
          }
        }

        //---------------------------------------------------------------------
        IDLCompiler::TokenListPtr IDLCompiler::getTokens() const
        {
          if (mTokenListStack.size() < 1) return TokenListPtr();
          return mTokenListStack.top();
        }

        //---------------------------------------------------------------------
        IDLCompiler::TokenListPtr IDLCompiler::popTokens()
        {
          TokenListPtr result = mTokenListStack.top();

          mTokenListStack.pop();
          mLastTokenStack.pop();

          if (mLastTokenStack.size() > 0) {
            auto token = mLastTokenStack.top();
            if (token) mLastToken = token;
          }

          return result;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::hasMoreTokens() const
        {
          if (mTokenListStack.size() < 1) return false;
          if (getTokens()->size() < 1) return false;
          return true;
        }

        //---------------------------------------------------------------------
        TokenPtr IDLCompiler::peekNextToken(const char *whatExpectingMoreTokens) throw (FailureWithLine)
        {
          if (mTokenListStack.size() > 0) {
            if (getTokens()->size() > 0) return getTokens()->front();
          }

          TokenPtr lastToken;
          if (mLastTokenStack.size() > 0) {
            mLastTokenStack.top();
          } else {
            lastToken = mLastToken;
          }

          ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_UNEXPECTED_EOF, lastToken ? lastToken->mLineCount : 0, String(whatExpectingMoreTokens) + " unexpectedly reached EOF");
          return TokenPtr();
        }

        //---------------------------------------------------------------------
        TokenPtr IDLCompiler::extractNextToken(const char *whatExpectingMoreTokens) throw (FailureWithLine)
        {
          if (mTokenListStack.size() > 0) {
            if (getTokens()->size() > 0) {
              mLastToken = getTokens()->front();
              mLastTokenStack.pop();
              mLastTokenStack.push(mLastToken);
              getTokens()->pop_front();
              return mLastToken;
            }
          }

          TokenPtr lastToken;
          if (mLastTokenStack.size() > 0) {
            mLastTokenStack.top();
          } else {
            lastToken = mLastToken;
          }

          ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_UNEXPECTED_EOF, lastToken ? lastToken->mLineCount : 0, String(whatExpectingMoreTokens) + " unexpectedly reached EOF");
          return TokenPtr();
        }

        //---------------------------------------------------------------------
        void IDLCompiler::putBackToken(TokenPtr token)
        {
          if (mTokenListStack.size() < 1) {
            ZS_THROW_INVALID_USAGE("must have active stack of tokens");
          }

          auto tokens = getTokens();
          tokens->push_front(token);

          mLastToken = token;
          mLastTokenStack.pop();
          mLastTokenStack.push(token);
        }

        //---------------------------------------------------------------------
        void IDLCompiler::putBackTokens(const TokenList &tokens)
        {
          if (mTokenListStack.size() < 1) {
            ZS_THROW_INVALID_USAGE("must have active stack of tokens");
          }

          auto existingTokens = getTokens();
          
          insertBefore(*existingTokens, tokens);
          
          TokenPtr firstToken;
          if (existingTokens->size() > 0) {
            firstToken = existingTokens->front();
            mLastToken = firstToken;
          }

          mLastTokenStack.pop();
          mLastTokenStack.push(firstToken);
        }

        //---------------------------------------------------------------------
        ULONG IDLCompiler::getLastLineNumber() const
        {
          if (!mLastToken) return 1;
          return mLastToken->mLineCount;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::insertBefore(
                                           TokenList &tokens,
                                           const TokenList &insertTheseTokens
                                           )
        {
          if (tokens.size() < 1) {
            tokens = insertTheseTokens;
            return;
          }
          
          for (auto iter = insertTheseTokens.rbegin(); iter != insertTheseTokens.rend(); ++iter)
          {
            tokens.push_front(*iter);
          }
        }
        
        //---------------------------------------------------------------------
        void IDLCompiler::insertAfter(
                                          TokenList &tokens,
                                          const TokenList &insertTheseTokens
                                          )
        {
          if (tokens.size() < 1) {
            tokens = insertTheseTokens;
            return;
          }
          
          for (auto iter = insertTheseTokens.begin(); iter != insertTheseTokens.end(); ++iter)
          {
            tokens.push_back(*iter);
          }
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::extractToClosingBraceToken(
                                                     const char *whatExpectingClosingToken,
                                                     TokenList &outTokens,
                                                     bool includeOuterBrace
                                                     ) throw (FailureWithLine)
        {
          auto token = peekNextToken(whatExpectingClosingToken);
          if (!token->isBrace()) return false;
          if (!token->isOpenBrace()) return false;

          size_t countBrace = 0;
          size_t countCurly = 0;
          size_t countSquare = 0;
          size_t countAngle = 0;

          do {
            token = extractNextToken(whatExpectingClosingToken);
            outTokens.push_back(token);

            if (token->isBrace()) {
              if (token->isOpenBrace()) {
                switch (token->mTokenType) {
                  case TokenType_Brace:       ++countBrace; break;
                  case TokenType_CurlyBrace:  ++countCurly; break;
                  case TokenType_SquareBrace: ++countSquare; break;
                  case TokenType_AngleBrace:  ++countAngle; break;
                  default:                    break;
                }
              } else {
                switch (token->mTokenType) {
                  case TokenType_Brace:       if (countBrace < 1) goto brace_mismatch; --countBrace; break;
                  case TokenType_CurlyBrace:  if (countCurly < 1) goto brace_mismatch; --countCurly; break;
                  case TokenType_SquareBrace: if (countSquare < 1) goto brace_mismatch; --countSquare; break;
                  case TokenType_AngleBrace:  if (countAngle < 1) goto brace_mismatch; --countAngle; break;
                  default:                    break;
                }
              }
            }
          } while ((countBrace > 0) ||
                   (countCurly > 0) ||
                   (countSquare > 0) ||
                   (countAngle > 0));

          {
            goto done;
          }

        brace_mismatch:
          {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(whatExpectingClosingToken) + " brace mismatch");
          }

        done:
          {
            if (!includeOuterBrace) {
              if (outTokens.size() > 1) {
                outTokens.pop_front();
                outTokens.pop_back();
              }
            }
          }

          return true;
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::extractToComma(
                                         const char *whatExpectingComma,
                                         TokenList &outTokens
                                         ) throw (FailureWithLine)
        {
          return extractToTokenType(whatExpectingComma, TokenType_CommaOperator, outTokens);
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::extractToEquals(
                                          const char *whatExpectingComma,
                                          TokenList &outTokens
                                          ) throw (FailureWithLine)
        {
          return extractToTokenType(whatExpectingComma, TokenType_EqualsOperator, outTokens);
        }

        //---------------------------------------------------------------------
        bool IDLCompiler::extractToTokenType(
                                             const char *whatExpectingComma,
                                             TokenTypes searchTokenType,
                                             TokenList &outTokens,
                                             bool includeFoundToken,
                                             bool processBrackets
                                             ) throw (FailureWithLine)
        {
          while (hasMoreTokens()) {
            auto token = extractNextToken(whatExpectingComma);
            if (searchTokenType == token->mTokenType) {
              if (!includeFoundToken) {
                putBackToken(token);
              }
              break;
            }
            
            if ((processBrackets) &&
                (token->isBrace())) {
              putBackToken(token);
              if (token->isCloseBrace()) return true;

              TokenList braceTokens;
              extractToClosingBraceToken(whatExpectingComma, braceTokens, true);
              for (auto iter = braceTokens.begin(); iter != braceTokens.end(); ++iter) {
                outTokens.push_back(*iter);
              }
              continue;
            }
            outTokens.push_back(token);
          }
          return true;
        }

        //---------------------------------------------------------------------
        TokenPtr IDLCompiler::peekAheadToFirstTokenOfType(const TokenTypeSet &tokenTypes)
        {
          auto tokenList = getTokens();
          if (!tokenList) return TokenPtr();

          for (auto iter = tokenList->begin(); iter != tokenList->end(); ++iter)
          {
            auto token = (*iter);

            auto found = tokenTypes.find(token->mTokenType);
            if (found != tokenTypes.end()) return token;
          }

          return TokenPtr();
        }

        //---------------------------------------------------------------------
        void IDLCompiler::processUsingNamespace(
                                                NamespacePtr currentNamespace,
                                                NamespacePtr usingNamespace
                                                )
        {
          if (currentNamespace == usingNamespace) return;

          Context::FindTypeOptions options;
          options.mSearchParents = false;

          for (auto iter = usingNamespace->mEnums.begin(); iter != usingNamespace->mEnums.end(); ++iter)
          {
            auto name = (*iter).first;
            auto type = (*iter).second;
            
            auto found = currentNamespace->findType(String(), name, options);
            if (found) continue;
            
            auto newTypedef = TypedefType::create(currentNamespace);
            newTypedef->mName = name;
            newTypedef->mOriginalType = type;
            currentNamespace->mTypedefs[name] = newTypedef;
          }

          for (auto iter = usingNamespace->mStructs.begin(); iter != usingNamespace->mStructs.end(); ++iter)
          {
            auto name = (*iter).first;
            auto type = (*iter).second;
            
            auto found = currentNamespace->findType(String(), name, options);
            if (found) continue;

            auto newTypedef = TypedefType::create(currentNamespace);
            newTypedef->mName = name;
            newTypedef->mOriginalType = type->getOriginalType();
            currentNamespace->mTypedefs[name] = newTypedef;
          }

          for (auto iter = usingNamespace->mTypedefs.begin(); iter != usingNamespace->mTypedefs.end(); ++iter)
          {
            auto name = (*iter).first;
            auto type = (*iter).second;

            auto found = currentNamespace->findType(String(), name, options);
            if (found) continue;

            auto newTypedef = TypedefType::create(currentNamespace);
            newTypedef->mName = name;
            newTypedef->mOriginalType = type;
            currentNamespace->mTypedefs[name] = newTypedef;
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::processUsingType(
                                           NamespacePtr currentNamespace,
                                           TypePtr usingType
                                           )
        {
          usingType = usingType->getOriginalType();

          auto name = usingType->getMappingName();

          Context::FindTypeOptions options;
          options.mSearchParents = false;

          auto found = currentNamespace->findType(String(), name, options);
          if (found) return;

          auto newTypedef = TypedefType::create(currentNamespace);
          newTypedef->mName = name;
          newTypedef->mOriginalType = usingType;
          currentNamespace->mTypedefs[name] = newTypedef;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::processTypedef(
                                         ContextPtr context,
                                         const TokenList &typeTokens,
                                         const String &typeName
                                         ) throw (FailureWithLine)
        {
          TypedefTypePtr createdTypedef;
          auto type = findTypeOrCreateTypedef(context, typeTokens, createdTypedef);

          TypePtr originalType = type;
          
          if (!createdTypedef) {
            createdTypedef = TypedefType::create(context);
            createdTypedef->mOriginalType = type;
            type = createdTypedef;
          } else {
            originalType = createdTypedef->mOriginalType.lock();
          }
          
          if (!originalType) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("typedef original type was not found"));
          }
          
          createdTypedef->mName = typeName;
          fillContext(createdTypedef);
          
          {
            auto namespaceObj = context->toNamespace();
            if (namespaceObj) {
              
              auto found = namespaceObj->mTypedefs.find(typeName);
              if (found != namespaceObj->mTypedefs.end()) return;  // assume types are the same
              namespaceObj->mTypedefs[createdTypedef->getMappingName()] = createdTypedef;
              return;
            }
          }
          
          {
            auto structObj = context->toStruct();
            if (structObj) {
              auto found = structObj->mTypedefs.find(typeName);
              if (found != structObj->mTypedefs.end()) return; // asumme types are the same
              structObj->mTypedefs[createdTypedef->getMappingName()] = createdTypedef;
              return;
            }
          }
          
          ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("typedef found in context that does not allow typedefs"));
        }

        //---------------------------------------------------------------------
        void IDLCompiler::processRelated(
                                         StructPtr structObj,
                                         const TokenList &typeTokens
                                         ) throw (FailureWithLine)
        {
          const char *what = "struct/interface inherited";

          TypedefTypePtr typedefType;
          auto type = findTypeOrCreateTypedef(structObj, typeTokens, typedefType);

          if (!type) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " related type was not found");
          }

          structObj->mIsARelationships[type->getPathName()] = type;
        }
        
        //---------------------------------------------------------------------
        IDLCompiler::StructPtr IDLCompiler::processStructForward(
                                                                 ContextPtr context,
                                                                 const String &typeName,
                                                                 bool *wasCreated
                                                                 ) throw (FailureWithLine)
        {
          if (wasCreated) *wasCreated = false;
          
          {
            auto namespaceObj = context->toNamespace();
            if (namespaceObj) {
              auto found = namespaceObj->mStructs.find(typeName);
              if (found != namespaceObj->mStructs.end()) {
                auto result = (*found).second;
                fillContext(result);
                return (*found).second;
              }
              
              if (wasCreated) *wasCreated = true;
              StructPtr structObj = Struct::create(context);
              structObj->mName = typeName;
              fillContext(structObj);
              
              namespaceObj->mStructs[structObj->getMappingName()] = structObj;
              return structObj;
            }
          }
          
          {
            auto outerStructObj = context->toStruct();
            if (outerStructObj) {
              auto found = outerStructObj->mStructs.find(typeName);
              if (found != outerStructObj->mStructs.end()) {
                auto result = (*found).second;
                fillContext(result);
                return (*found).second;
              }
              
              if (wasCreated) *wasCreated = true;
              StructPtr structObj = Struct::create(context);
              structObj->mName = typeName;
              fillContext(structObj);

              outerStructObj->mStructs[structObj->getMappingName()] = structObj;
              return structObj;
            }
          }

          ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String("struct/class forward not attached to namespace or struct context"));
          return StructPtr();
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IDLCompilerHelper
        #pragma mark
        
        class IDLCompilerHelper : protected IDLCompiler
        {
        public:
          struct FoundBasicTypeModifiers
          {
            bool mAnyBasicTypeModifiers {false};
            bool mAnyOtherModifier {false};

            bool mSigned {false};
            bool mUnsigned {false};
            bool mChar {false};
            bool mShort {false};
            bool mInt {false};
            size_t mTotalLongs {false};
            bool mFloat {false};
            bool mDouble {false};

            bool mConst {false};

            bool mLastWasTypename {false};
            bool mLastWasScope {false};
            
            String mTypeName;
            
            //-----------------------------------------------------------------
            void throwInvalidModifier() throw (InvalidContent)
            {
              ZS_THROW_CUSTOM(InvalidContent, "has invalid type modifier");
            }
            
            //-----------------------------------------------------------------
            void insert(const String &modifierStr) throw (InvalidContent)
            {
              if ("signed" == modifierStr) {
                if (mUnsigned || mSigned || mFloat || mDouble) throwInvalidModifier();
                mSigned = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("unsigned" == modifierStr) {
                if (mUnsigned || mSigned || mFloat || mDouble) throwInvalidModifier();
                mUnsigned = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("long" == modifierStr) {
                if ((mTotalLongs > 1) || mChar || mShort || mFloat) throwInvalidModifier();
                if ((mTotalLongs > 1) && (mDouble)) throwInvalidModifier();
                ++mTotalLongs;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("char" == modifierStr) {
                if ((mTotalLongs > 0) || mChar || mShort || mInt || mFloat || mDouble) throwInvalidModifier();
                mChar = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("short" == modifierStr) {
                if ((mTotalLongs > 0) || mChar || mShort || mFloat || mDouble) throwInvalidModifier();
                mShort = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("int" == modifierStr) {
                if (mChar || mInt || mFloat || mDouble) throwInvalidModifier();
                mInt = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("long" == modifierStr) {
                if ((mTotalLongs > 1) || mChar || mShort || mFloat) throwInvalidModifier();
                if ((mTotalLongs > 0) && mDouble) throwInvalidModifier();
                ++mTotalLongs;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("float" == modifierStr) {
                if (mSigned || mUnsigned || (mTotalLongs > 0) || mChar || mInt || mFloat || mDouble) throwInvalidModifier();
                mFloat = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("double" == modifierStr) {
                if (mSigned || mUnsigned || (mTotalLongs > 1) || mChar || mInt || mFloat || mDouble) throwInvalidModifier();
                mDouble = true;
                mAnyBasicTypeModifiers = true;
                return;
              }
              if ("const" == modifierStr) {
                if (mConst) throwInvalidModifier();
                mConst = true;
                mAnyOtherModifier = true;
                return;
              }

              if (mLastWasTypename) throwInvalidModifier();
              mLastWasTypename = true;
              mLastWasScope = false;
              mTypeName += modifierStr;
            }
            
            //-----------------------------------------------------------------
            void insertScope() throw (InvalidContent)
            {
              if (mLastWasScope) throwInvalidModifier();
              mLastWasTypename = false;
              mLastWasScope = true;
              mTypeName += "::";
            }
            
            //-----------------------------------------------------------------
            PredefinedTypedefs mergePredefined(PredefinedTypedefs existingBasicType) throw (InvalidContent)
            {
              PredefinedTypedefs &newBasicType = existingBasicType;
              
              switch (existingBasicType) {
                case PredefinedTypedef_void:
                {
                  if (mAnyBasicTypeModifiers) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_bool:
                {
                  if (mAnyBasicTypeModifiers) throwInvalidModifier();
                  break;
                }

                case PredefinedTypedef_uchar: {
                  if (mSigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_char: {
                  if (mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mSigned) newBasicType = PredefinedTypedef_schar;
                  if (mUnsigned) newBasicType = PredefinedTypedef_uchar;
                  break;
                }
                case PredefinedTypedef_schar:
                {
                  if (mUnsigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_ushort:
                {
                  if (mSigned || mChar || mShort || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  break;
                }
                case PredefinedTypedef_short: {
                  if (mChar || mShort || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  if (mSigned) newBasicType = PredefinedTypedef_sshort;
                  if (mUnsigned) newBasicType = PredefinedTypedef_ushort;
                  break;
                }
                case PredefinedTypedef_sshort:
                {
                  if (mUnsigned || mChar || mShort || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  break;
                }
                case PredefinedTypedef_uint:
                {
                  if (mSigned || mChar || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mShort) newBasicType = PredefinedTypedef_ushort;
                  break;
                }
                case PredefinedTypedef_int:
                {
                  if (mChar || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mShort) {
                    if (mSigned)
                      newBasicType = PredefinedTypedef_sshort;
                    else if (mUnsigned)
                      newBasicType = PredefinedTypedef_ushort;
                    else
                      newBasicType = PredefinedTypedef_short;
                  } else {
                    if (mSigned) newBasicType = PredefinedTypedef_sint;
                    if (mUnsigned) newBasicType = PredefinedTypedef_uint;
                  }
                  break;
                }
                case PredefinedTypedef_sint:
                {
                  if (mUnsigned || mChar || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mShort) newBasicType = PredefinedTypedef_sshort;
                  break;
                }
                case PredefinedTypedef_ulong:
                {
                  if (mSigned || mChar || mShort || (mTotalLongs > 1) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  if (mTotalLongs > 0) newBasicType = PredefinedTypedef_ulonglong;
                  break;
                }
                case PredefinedTypedef_long:
                {
                  if (mChar || mShort || (mTotalLongs > 1) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  if (mTotalLongs > 0) {
                    if (mSigned)
                      newBasicType = PredefinedTypedef_slonglong;
                    else if (mUnsigned)
                      newBasicType = PredefinedTypedef_ulonglong;
                    else
                      newBasicType = PredefinedTypedef_longlong;
                  } else {
                    if (mSigned) newBasicType = PredefinedTypedef_slong;
                    if (mUnsigned) newBasicType = PredefinedTypedef_ulong;
                  }
                  break;
                }
                case PredefinedTypedef_slong:
                {
                  if (mUnsigned || mChar || mShort || (mTotalLongs > 1) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  if (mTotalLongs > 0) newBasicType = PredefinedTypedef_slonglong;
                  break;
                }
                case PredefinedTypedef_ulonglong:
                {
                  if (mSigned || mChar || mShort || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  break;
                }
                case PredefinedTypedef_longlong:
                {
                  if (mChar || mShort || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  if (mSigned) newBasicType = PredefinedTypedef_slonglong;
                  if (mUnsigned) newBasicType = PredefinedTypedef_ulonglong;
                  break;
                }
                case PredefinedTypedef_slonglong:
                {
                  if (mUnsigned || mChar || mShort || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  mInt = false;
                  break;
                }
                case PredefinedTypedef_uint8:
                case PredefinedTypedef_uint16:
                case PredefinedTypedef_uint32:
                case PredefinedTypedef_uint64:
                case PredefinedTypedef_byte:
                case PredefinedTypedef_word:
                case PredefinedTypedef_dword:
                case PredefinedTypedef_qword:
                {
                  if (mSigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_int8:
                {
                  if (mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mSigned) newBasicType = PredefinedTypedef_sint8;
                  if (mUnsigned) newBasicType = PredefinedTypedef_uint8;
                  break;
                }
                case PredefinedTypedef_sint8:
                case PredefinedTypedef_sint16:
                case PredefinedTypedef_sint32:
                case PredefinedTypedef_sint64:
                {
                  if (mUnsigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_int16:
                {
                  if (mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mSigned) newBasicType = PredefinedTypedef_sint16;
                  if (mUnsigned) newBasicType = PredefinedTypedef_uint16;
                  break;
                }
                case PredefinedTypedef_int32:
                {
                  if (mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mSigned) newBasicType = PredefinedTypedef_sint32;
                  if (mUnsigned) newBasicType = PredefinedTypedef_uint32;
                  break;
                }
                case PredefinedTypedef_int64:
                {
                  if (mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  if (mSigned) newBasicType = PredefinedTypedef_sint64;
                  if (mUnsigned) newBasicType = PredefinedTypedef_uint64;
                  break;
                }
                  
                case PredefinedTypedef_float:
                case PredefinedTypedef_float32:
                case PredefinedTypedef_float64:
                {
                  if (mSigned || mUnsigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_double:
                {
                  if (mSigned || mUnsigned || mChar || mShort || mInt || (mTotalLongs > 1) || mFloat || mDouble) throwInvalidModifier();
                  if (mTotalLongs > 0) newBasicType = PredefinedTypedef_ldouble;
                  break;
                }
                case PredefinedTypedef_ldouble:
                {
                  if (mSigned || mUnsigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_binary:
                case PredefinedTypedef_string:
                case PredefinedTypedef_astring:
                case PredefinedTypedef_wstring: 
                {
                  if (mSigned || mUnsigned || mChar || mShort || mInt || (mTotalLongs > 0) || mFloat || mDouble) throwInvalidModifier();
                  break;
                }
                case PredefinedTypedef_pointer:
                case PredefinedTypedef_size:    throwInvalidModifier();
              }
              return newBasicType;
            }

            //-----------------------------------------------------------------
            PredefinedTypedefs getBasicType()
            {
              if (mChar) {
                if (mUnsigned) return PredefinedTypedef_uchar;
                if (mSigned) return PredefinedTypedef_schar;
                return PredefinedTypedef_char;
              }
              if (mShort) {
                if (mUnsigned) return PredefinedTypedef_ushort;
                if (mSigned) return PredefinedTypedef_sshort;
                return PredefinedTypedef_short;
              }
              if (mFloat) return PredefinedTypedef_float;
              if (mDouble) {
                if (mTotalLongs > 0) return PredefinedTypedef_ldouble;
                return PredefinedTypedef_double;
              }
              
              if (mTotalLongs > 1) {
                if (mUnsigned) return PredefinedTypedef_ulonglong;
                if (mSigned) return PredefinedTypedef_slonglong;
                return PredefinedTypedef_longlong;
              }

              if (mTotalLongs > 0) {
                if (mUnsigned) return PredefinedTypedef_ulong;
                if (mSigned) return PredefinedTypedef_slong;
                return PredefinedTypedef_long;
              }
              
              if (mInt) {
                if (mUnsigned) return PredefinedTypedef_uint;
                if (mSigned) return PredefinedTypedef_sint;
                return PredefinedTypedef_int;
              }
              ZS_THROW_CUSTOM(InvalidContent, "is not a basic type");
              return PredefinedTypedef_int;
            }

            //-----------------------------------------------------------------
            TypePtr processType(
                                ContextPtr context,
                                TypedefTypePtr &outCreatedTypedef
                                ) throw (InvalidContent)
            {
              if ((mShort) && (mInt)) mInt = false; // strip redundant information

              if (mTypeName.hasData()) {
                auto existingType = context->findType(mTypeName);
                if (!existingType) throwInvalidModifier();

                BasicTypePtr basicType;
                TypedefTypePtr typedefObj = existingType ? existingType->toTypedefType() : TypedefTypePtr();
                
                if (typedefObj) {
                  typedefObj->resolveTypedefs();
                }

                while (typedefObj) {
                  TypePtr foundType = typedefObj->mOriginalType.lock();
                  
                  if (foundType) {
                    basicType = foundType->toBasicType();
                    typedefObj = foundType->toTypedefType();
                  }
                }

                if (basicType) {
                  outCreatedTypedef = TypedefType::create(context);

                  if (typedefObj) {
                    outCreatedTypedef->mModifiers = typedefObj->mModifiers;
                  }

                  PredefinedTypedefs newBasicType = mergePredefined(basicType->mBaseType);
                  auto foundNewBasicType = context->findType(IEventingTypes::toString(newBasicType));
                  if (!foundNewBasicType) {
                    ZS_THROW_CUSTOM(InvalidContent, "did not find new basic type");
                  }
                  outCreatedTypedef->mOriginalType = foundNewBasicType;
                  outCreatedTypedef->resolveTypedefs();

                  auto bypassResult = outCreatedTypedef->getOriginalType();
                  if (bypassResult != outCreatedTypedef) {
                    outCreatedTypedef.reset();
                    return bypassResult;
                  }

                  return outCreatedTypedef;
                }

                if (mAnyBasicTypeModifiers) throwInvalidModifier();
                if (!mAnyOtherModifier) return existingType;

                outCreatedTypedef = TypedefType::create(context);
                outCreatedTypedef->mOriginalType = existingType;
                outCreatedTypedef->resolveTypedefs();

                auto bypassResult = outCreatedTypedef->getOriginalType();
                if (bypassResult != outCreatedTypedef) {
                  outCreatedTypedef.reset();
                  return bypassResult;
                }
                return outCreatedTypedef;
              }

              if (!mAnyBasicTypeModifiers) throwInvalidModifier();

              auto predefinedType = getBasicType();
              auto existingBasicType = context->findType(IEventingTypes::toString(predefinedType));
              if (!existingBasicType) {
                ZS_THROW_CUSTOM(InvalidContent, "did not find basic type");
              }

              if (mAnyOtherModifier) {
                outCreatedTypedef = TypedefType::create(context);
                outCreatedTypedef->mOriginalType = existingBasicType;
                return outCreatedTypedef;
              }
              return existingBasicType;
            }
          };
        };

        //---------------------------------------------------------------------
        IDLCompiler::TypePtr IDLCompiler::findTypeOrCreateTypedef(
                                                                  ContextPtr context,
                                                                  const TokenList &inTokens,
                                                                  TypedefTypePtr &outCreatedTypedef
                                                                  ) throw (FailureWithLine)
        {
          TypePtr result;

          const char *what = "Type search";

          TokenList pretemplateTokens;

          TypeList templateTypes;

          {
            // search for template parameters
            pushTokens(inTokens);

            while (hasMoreTokens()) {
              auto token = extractNextToken(what);

              if (token->isOpenBrace(TokenType_AngleBrace)) {
                putBackToken(token);
                TokenList templateContents;
                extractToClosingBraceToken(what, templateContents);

                pushTokens(templateContents);

                while (hasMoreTokens()) {
                  if (parseComma()) continue;

                  TokenList templateTypeTokens;
                  extractToComma(what, templateTypeTokens);

                  TypedefTypePtr typedefObj;
                  auto foundType = findTypeOrCreateTypedef(context, templateTypeTokens, typedefObj);
                  if (!foundType) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " did not find templated type");
                  }
                  templateTypes.push_back(foundType);
                }

                popTokens();
                break;
              }
              pretemplateTokens.push_back(token);
            }

            popTokens();
          }

          try
          {
            pushTokens(pretemplateTokens);
            
            IDLCompilerHelper::FoundBasicTypeModifiers modifiers;

            while (hasMoreTokens()) {
              auto token = extractNextToken(what);
              switch (token->mTokenType) {
                case TokenType_Identifier: {
                  modifiers.insert(token->mToken);
                  continue;
                }
                case TokenType_ScopeOperator: {
                  modifiers.insertScope();
                  continue;
                }
                default:
                {
                  ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " has not legal type modifier");
                  break;
                }
              }
            }
            popTokens();

            result = modifiers.processType(context, outCreatedTypedef);

            {
              auto originalType = result;

              auto typedefObj = result->toTypedefType();
              if (typedefObj) {
                originalType = typedefObj->mOriginalType.lock();
              }

              auto structObj = originalType->toStruct();
              if (structObj) {
                
                if (templateTypes.size() > 0) {
                  auto templatedStruct = TemplatedStructType::create(structObj);
                  templatedStruct->mTemplateArguments = templateTypes;

                  if (templatedStruct->mTemplateArguments.size() < structObj->mGenerics.size()) {
                    // attempt to fill in remaining part of template with defaults
                    if (structObj->mGenericDefaultTypes.size() > templatedStruct->mTemplateArguments.size()) {
                      size_t skip = templatedStruct->mTemplateArguments.size();
                      for (auto iterDefault = structObj->mGenericDefaultTypes.begin(); iterDefault != structObj->mGenericDefaultTypes.end(); ++iterDefault) {
                        if (skip > 0) {
                          --skip;
                          continue;
                        }
                        auto type = (*iterDefault);
                        if (!type) break;
                        templatedStruct->mTemplateArguments.push_back(type);
                      }
                    }
                  }

                  if (structObj->mGenerics.size() < templatedStruct->mTemplateArguments.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " does not have enough templated parameters");
                  }

                  auto hashID = templatedStruct->calculateTemplateID();
                  templatedStruct->mName = hashID;
                  
                  auto found = structObj->mTemplatedStructs.find(hashID);
                  if (found == structObj->mTemplatedStructs.end()) {
                    structObj->mTemplatedStructs[hashID] = templatedStruct;
                  } else {
                    templatedStruct = (*found).second;
                  }
                  
                  if (outCreatedTypedef) {
                    auto replacementTypedef = TypedefType::create(outCreatedTypedef->getParent());
                    replacementTypedef->copyContentsFrom(outCreatedTypedef);
                    replacementTypedef->mOriginalType = templatedStruct;
                    outCreatedTypedef = replacementTypedef;
                    result = replacementTypedef;
                  } else {
                    result = templatedStruct;
                  }
                }
              } else {
                if (templateTypes.size() > 0) {
                  ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " has template parameters but type referenced isn't a struct or generic template");
                }
              }
            }
          } catch (const InvalidContent &e) {
            ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, getLastLineNumber(), String(what) + " " + e.message());
          }

          auto bypassType = result->getOriginalType();
          if (bypassType != result) {
            outCreatedTypedef = TypedefTypePtr();
          }
          return bypassType;
        }

        //---------------------------------------------------------------------
        void IDLCompiler::writeXML(const String &outputName, const DocumentPtr &doc) throw (Failure)
        {
          if (!doc) return;
          try {
            auto output = UseHelper::writeXML(*doc);
            UseHelper::saveFile(outputName, *output);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save XML file \"" + outputName + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::writeJSON(const String &outputName, const DocumentPtr &doc) throw (Failure)
        {
          if (!doc) return;
          try {
            auto output = UseHelper::writeJSON(*doc);
            UseHelper::saveFile(outputName, *output);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save JSON file \"" + outputName + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::writeBinary(const String &outputName, const SecureByteBlockPtr &buffer) throw (Failure)
        {
          if ((!buffer) ||
              (0 == buffer->SizeInBytes())) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save file \"" + outputName + "\": file is empty");
          }
          try {
            UseHelper::saveFile(outputName, *buffer);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save file \"" + outputName + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
        }

        //---------------------------------------------------------------------
        void IDLCompiler::installDefaultTargets()
        {
          ICompiler::installTarget(internal::GenerateStructHeader::create());
          ICompiler::installTarget(internal::GenerateStructImplCpp::create());
          ICompiler::installTarget(internal::GenerateStructCx::create());
        }

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
