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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_EventingCompiler.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_CommandLine.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_IDLCompiler.h>
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


#define ZS_EVENTING_PREFIX "ZS_EVENTING_"

#define ZS_EVENTING_METHOD_TOTAL_PARAMS (7)

#define ZS_EVENTING_METHOD_COMPACT_PREFIX "COMPACT_"
#define ZS_EVENTING_METHOD_PROVIDER "PROVIDER"
#define ZS_EVENTING_METHOD_ALIAS "ALIAS"
#define ZS_EVENTING_METHOD_INCLUDE "INCLUDE"
#define ZS_EVENTING_METHOD_SOURCE "SOURCE"
#define ZS_EVENTING_METHOD_CHANNEL "CHANNEL"
#define ZS_EVENTING_METHOD_TASK "TASK"
#define ZS_EVENTING_METHOD_KEYWORD "KEYWORD"
#define ZS_EVENTING_METHOD_OPCODE "OPCODE"
#define ZS_EVENTING_METHOD_TASK_OPCODE "TASK_OPCODE"
#define ZS_EVENTING_METHOD_REGISTER "REGISTER"
#define ZS_EVENTING_METHOD_UNREGISTER "UNREGISTER"
#define ZS_EVENTING_METHOD_EXCLUSIVE "EXCLUSIVE"

#define ZS_EVENTING_METHOD_ASSIGN_VALUE "ASSIGN_VALUE"

#define ZS_EVENTING_METHOD_SUBSYSTEM_DEFAULT_LEVEL "SUBSYSTEM_DEFAULT_LEVEL"

namespace zsLib { namespace eventing { namespace tool { ZS_DECLARE_SUBSYSTEM(zsLib_eventing_tool) } } }

namespace zsLib
{
  namespace eventing
  {
    ZS_DECLARE_TYPEDEF_PTR(IEventingTypes::Provider, Provider);

    namespace tool
    {
      ZS_DECLARE_TYPEDEF_PTR(eventing::IHelper, UseEventingHelper);
      ZS_DECLARE_TYPEDEF_PTR(eventing::IHasher, UseHasher);
      typedef std::set<String> HashSet;
      typedef std::map<size_t, String> ArgumentMap;
      typedef std::set<size_t> IndexSet;
      typedef std::set<uint64_t> Index64Set;

      namespace internal
      {
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark Helpers
        #pragma mark

        struct ParseState
        {
          const char *mPos {};
          bool mStartOfLine {true};
          ULONG mStartOfLineCount {0};
          ULONG mLineCount {1};
        };

        //-----------------------------------------------------------------------
        static ICompilerTypes::Config &prepareProvider(ICompilerTypes::Config &config)
        {
          if (config.mProvider) return config;
          config.mProvider = Provider::create();
          return config;
        }

        //-----------------------------------------------------------------------
        static bool isNumber(const char *p)
        {
          while ('\0' != *p)
          {
            if (!isdigit(*p)) return false;
            ++p;
          }
          return true;
        }

        //---------------------------------------------------------------------
        static bool skipPreprocessorDirective(
                                              const char * &p,
                                              ULONG *currentLine
                                              )
        {
          if ('#' != *p) return false;

          while ('\0' != *p)
          {
            if (Helper::skipCComments(p, currentLine)) break;
            if (Helper::skipCPPComments(p)) continue;
            if (Helper::skipEOL(p, currentLine)) break;

            ++p;
          }
          return true;
        }

        //---------------------------------------------------------------------
        static String getEventingLine(ParseState &state) throw (InvalidContentWithLine)
        {
          auto prefixLength = strlen(ZS_EVENTING_PREFIX);
          state.mStartOfLineCount = 0;

          const char * &p = state.mPos;

          while ('\0' != *p)
          {
            if (Helper::skipWhitespaceExceptEOL(p)) continue;
            if (Helper::skipEOL(p, &(state.mLineCount))) {
              state.mStartOfLine = true;
              continue;
            }
            if (Helper::skipCComments(p, &(state.mLineCount))) continue;
            if (Helper::skipCPPComments(p)) continue;

            if (state.mStartOfLine) {
              // start of line
              if (skipPreprocessorDirective(p, &(state.mLineCount))) continue;
            }

            state.mStartOfLine = false;
            if (0 != strncmp(ZS_EVENTING_PREFIX, p, prefixLength)) {
              Helper::skipToEOL(p);
              state.mStartOfLine = true;
              continue;
            }

            state.mStartOfLineCount = state.mLineCount;

            p += prefixLength;

            const char *startPos = p;

            bool foundBracket = false;
            size_t bracketDepth = 0;

            while ('\0' != *p)
            {
              if (Helper::skipWhitespaceExceptEOL(p)) continue;
              if (Helper::skipEOL(p, &(state.mLineCount))) {
                state.mStartOfLine = true;
                continue;
              }
              if (Helper::skipCComments(p, &(state.mLineCount))) continue;
              if (Helper::skipCPPComments(p)) continue;
              if (state.mStartOfLine) {
                // start of line
                if (skipPreprocessorDirective(p, &(state.mLineCount))) continue;
              }
              if (Helper::skipQuote(p, &(state.mLineCount))) continue;

              char value = *p;
              ++p;

              if (')' == value) {
                if (0 == bracketDepth) {
                  ZS_THROW_CUSTOM_PROPERTIES_1(InvalidContentWithLine, state.mLineCount, String("Eventing mechanism found but closing bracket \')\' prematurely found"));
                }
                --bracketDepth;
                if (0 == bracketDepth) {
                  break;
                }
                continue;
              }
              if ('(' == value) {
                ++bracketDepth;
                foundBracket = true;
                continue;
              }
            }
            if (0 != bracketDepth) {
              ZS_THROW_CUSTOM_PROPERTIES_1(InvalidContentWithLine, state.mLineCount, String("Eventing mechanism found but closing bracket \')\' not found"));
            }
            if (!foundBracket) {
              ZS_THROW_CUSTOM_PROPERTIES_1(InvalidContentWithLine, state.mLineCount, String("Eventing mechanism found but opening bracket \'(\' not found"));
            }

            return String(startPos, static_cast<uintptr_t>(p - startPos));
          }

          return String();
        }

        //---------------------------------------------------------------------
        static void parseLine(
                              const char *p,
                              String &outMethod,
                              ArgumentMap &outArguments,
                              ULONG lineCount
                              ) throw (InvalidContentWithLine)
        {
          ZS_DECLARE_TYPEDEF_PTR(std::stringstream, StringStream);
          
          const char *startPos = p;

          while ('\0' != *p)
          {
            if (isalnum(*p)) {
              ++p;
              continue;
            }
            if ('_' == *p) {
              ++p;
              continue;
            }
            break;
          }

          outMethod = String(startPos, static_cast<uintptr_t>(p - startPos));

          bool startOfLine = false;
          bool lastWasSpace = true;
          bool foundBracket = false;
          size_t bracketDepth = 0;
          bool done = false;
          size_t index = 0;

          StringStreamUniPtr ss(new StringStream());

          while ('\0' != *p)
          {
            {
              if (Helper::skipWhitespaceExceptEOL(p)) goto found_space;
              if (Helper::skipEOL(p, &lineCount)) {
                startOfLine = true;
                goto found_space;
              }
              if (Helper::skipCComments(p, &lineCount)) goto found_space;
              if (Helper::skipCPPComments(p)) goto found_space;
              if (startOfLine) {
                // start of line
                if (skipPreprocessorDirective(p, &lineCount)) goto found_space;
              }

              startOfLine = false;
              lastWasSpace = false;

              char value = *p;
              ++p;

              switch (value) {
                case '(':
                {
                  foundBracket = true;
                  ++bracketDepth;
                  if (1 != bracketDepth) {
                    (*ss) << value;
                  }
                  break;
                }
                case ')':
                {
                  if (!foundBracket) {
                    ZS_THROW_CUSTOM_PROPERTIES_1(InvalidContentWithLine, lineCount, String("Eventing mechanism closing bracket \')\' prematurely found"));
                  }
                  if (bracketDepth > 1) {
                    (*ss) << value;
                  }
                  --bracketDepth;
                  if (0 == bracketDepth) {
                    done = true;
                    goto found_result;
                  }
                  break;
                }
                case ',':
                {
                  if (!foundBracket) {
                    ZS_THROW_CUSTOM_PROPERTIES_1(InvalidContentWithLine, lineCount, String("Eventing mechanism found illegal comma \',\'"));
                  }
                  if (1 != bracketDepth) {
                    (*ss) << value;
                    break;
                  }
                  goto found_result;
                }
                default: {
                  (*ss) << value;
                  break;
                }
              }
              continue;
            }

          found_space:
            {
              if (!lastWasSpace) {
                (*ss) << ' ';
                lastWasSpace = true;
              }
              continue;
            }

          found_result:
            {
              String result = ss->str();
              result.trim();

              if (result.hasData()) {
                outArguments[index] = result;
                ++index;
              }
              
              StringStreamUniPtr emptySS(new StringStream());
              ss.swap(emptySS);
              lastWasSpace = true;
              if (!done) continue;
              break;
            }
          }
        }

        //---------------------------------------------------------------------
        static bool insert(
                           IndexSet &indexes,
                           size_t index,
                           bool throwIfFound = true
                           ) throw (InvalidArgument)
        {
          if (0 == index) return false;
          
          auto found = indexes.find(index);
          if (found == indexes.end()) {
            indexes.insert(index);
            return true;
          }

          if (throwIfFound) {
            ZS_THROW_INVALID_ARGUMENT(String("Duplicate value: ") + string(index));
          }
          return false;
        }

        //---------------------------------------------------------------------
        static bool insert64(
                             Index64Set &indexes,
                             uint64_t index,
                             bool throwIfFound = true
                             ) throw (InvalidArgument)
        {
          if (0 == index) return false;
          
          auto found = indexes.find(index);
          if (found == indexes.end()) {
            indexes.insert(index);
            return true;
          }
          
          if (throwIfFound) {
            ZS_THROW_INVALID_ARGUMENT(String("Duplicate value: ") + string(index));
          }
          return false;
        }

        //---------------------------------------------------------------------
        static String toSymbol(const String &str)
        {
          String temp(str);
          temp.replaceAll("-", "_");
          temp.replaceAll("/", "_");
          temp.trim();
          temp.toUpper();
          return temp;
        }

        //---------------------------------------------------------------------
        static ElementPtr createStringEl(const String &id, const char *value)
        {
          ElementPtr stringEl = Element::create("string");
          stringEl->setAttribute("id", id);
          stringEl->setAttribute("value", value);
          return stringEl;
        }

        //---------------------------------------------------------------------
        static ElementPtr createDataEl(const String &inType, const char *name)
        {
          ElementPtr dataEl = Element::create("data");
          dataEl->setAttribute("inType", "win:" + inType);
          dataEl->setAttribute("name", name);
          return dataEl;
        }

        //---------------------------------------------------------------------
        static void addEOL(NodePtr parentEl)
        {
          auto text = Text::create();
          text->setValue("\n");
          parentEl->adoptAsLastChild(text);
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark EventingCompiler
        #pragma mark

        //---------------------------------------------------------------------
        EventingCompiler::EventingCompiler(
                                           const make_private &,
                                           const Config &config
                                           ) :
          mConfig(config)
        {
        }

        //---------------------------------------------------------------------
        EventingCompiler::~EventingCompiler()
        {
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark EventingCompiler => ICompiler
        #pragma mark

        //---------------------------------------------------------------------
        EventingCompilerPtr EventingCompiler::create(const Config &config)
        {
          EventingCompilerPtr pThis(std::make_shared<EventingCompiler>(make_private{}, config));
          pThis->mThisWeak = pThis;
          return pThis;
        }

        //---------------------------------------------------------------------
        void EventingCompiler::process() throw (Failure, FailureWithLine)
        {
          outputMacros();
          read();
          prepareIndex();
          validate();
          if ((mConfig.mOutputName.hasData()) &&
              (mConfig.mProvider)) {
            writeXML(mConfig.mOutputName + "_win_etw.man", generateManifest("_win_etw.dll"));
            writeXML(mConfig.mOutputName + "_win_etw.wprp", generateWprp());
            writeJSON(mConfig.mOutputName + ".jman", generateJsonMan());
            String outputXPlatformNameStr = mConfig.mOutputName + ".h";
            String outputWindowsNameStr = mConfig.mOutputName + "_win.h";
            String outputWindowsETWNameStr = mConfig.mOutputName + "_win_etw.h";
            writeBinary(outputXPlatformNameStr, generateXPlatformEventsHeader(outputXPlatformNameStr, outputWindowsNameStr));
            writeBinary(outputWindowsNameStr, generateWindowsEventsHeader(outputXPlatformNameStr, outputWindowsNameStr, outputWindowsETWNameStr));
          }
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark EventingCompiler => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        void EventingCompiler::outputMacros()
        {
#if 0
          //#define ZS_EVENTING_1(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode, xType1, xName1, xValue1)
          {
            for (size_t index = 0; index <= 38; ++index)
            {
              tool::output() << "#define ZS_EVENTING_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
              for (size_t total = 1; total <= index; ++total)
              {
                tool::output() << ", xType" << string(total) << ", xName" << string(total) << ", xValue" << string(total);
              }

              tool::output() << ")\n";
            }
          }
          //#define ZS_EVENTING_COMPACT_1(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode, xType1AndName1, xValue1)
          {
            for (size_t index = 0; index <= 65; ++index)
            {
              tool::output() << "#define ZS_EVENTING_COMPACT_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
              for (size_t total = 1; total <= index; ++total)
              {
                tool::output() << ", xType" << string(total) << "AndName" << string(total) << ", xValue" << string(total);
              }

              tool::output() << ")\n";
            }
          }
          //#define ZS_EVENTING_1(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode, xType1, xName1, xValue1) ZS_INTERNAL_EVENTING_EVENT_##xSymbol(xSubsystem, xValue1)
          {
            tool::output() << "\n\n";
            for (size_t index = 0; index <= 38; ++index)
            {
              tool::output() << "#define ZS_EVENTING_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
              for (size_t total = 1; total <= index; ++total)
              {
                tool::output() << ", xType" << string(total) << ", xName" << string(total) << ", xValue" << string(total);
              }

              tool::output() << ") \\\n";
              tool::output() << "                                                                                ZS_INTERNAL_EVENTING_EVENT_##xSymbol(xSubsystem";
              for (size_t total = 1; total <= index; ++total)
              {
                tool::output() << ", xValue" << string(total);
              }
              tool::output() << ")\n\n";
            }
          }
          //#define ZS_EVENTING_COMPACT_1(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode, xType1, xName1, xValue1) ZS_INTERNAL_EVENTING_EVENT_##xSymbol(xSubsystem, xValue1)
          {
            tool::output() << "\n\n";
            for (size_t index = 0; index <= 65; ++index)
            {
              tool::output() << "#define ZS_EVENTING_COMPACT_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
              for (size_t total = 1; total <= index; ++total)
              {
                tool::output() << ", xType" << string(total) << "AndName" << string(total) << ", xValue" << string(total);
              }

              tool::output() << ") \\\n";
              tool::output() << "                                                                                ZS_INTERNAL_EVENTING_EVENT_##xSymbol(xSubsystem";
              for (size_t total = 1; total <= index; ++total)
              {
                tool::output() << ", xValue" << string(total);
              }
              tool::output() << ")\n\n";
            }
          }
#endif //0
        }

        //---------------------------------------------------------------------
        void EventingCompiler::read() throw (Failure, FailureWithLine)
        {
          typedef std::set<String> NameSet;

          HashSet processedHashes;

          ProviderPtr &provider = mConfig.mProvider;

          SecureByteBlockPtr configRaw;

          try {
            configRaw = UseEventingHelper::loadFile(mConfig.mConfigFile);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load main configuration file: ") + mConfig.mConfigFile + ", error=" + string(e.result()) + ", reason=" + e.message());
          }
          if (!configRaw) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load main configuration file: ") + mConfig.mConfigFile);
          }
          processedHashes.insert(UseHasher::hashAsString(configRaw));
          auto rootEl = UseEventingHelper::read(configRaw);

          try {
            provider = Provider::create(rootEl);
          } catch (const InvalidContent &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Failed to parse main configuration: " + e.message());
          }

          StringList sources = mConfig.mSourceFiles;
          mConfig.mSourceFiles.clear();

          ElementPtr sourcesEl = rootEl->findFirstChildElement("includes");
          if (sourcesEl) {
            ElementPtr sourceEl = sourcesEl->findFirstChildElement("include");
            while (sourceEl) {
              auto source = UseEventingHelper::getElementTextAndDecode(sourceEl);

              if (source.hasData()) {
                mConfig.mSourceFiles.push_back(source);
              }
              sourceEl = sourceEl->findNextSiblingElement("include");
            }
          }

          // put back the original configuration files
          for (auto iter = sources.begin(); iter != sources.end(); ++iter) {
            mConfig.mSourceFiles.push_back(*iter);
          }

          ElementPtr includesEl = rootEl->findFirstChildElement("sources");
          if (includesEl) {
            ElementPtr includeEl = includesEl->findFirstChildElement("source");
            while (includeEl) {
              auto source = UseEventingHelper::getElementTextAndDecode(includeEl);

              if (source.hasData()) {
                mConfig.mSourceFiles.push_back(source);
              }
              includeEl = includeEl->findNextSiblingElement("source");
            }
          }

          while (mConfig.mSourceFiles.size() > 0)
          {
            String fileName = mConfig.mSourceFiles.front();
            mConfig.mSourceFiles.pop_front();

            SecureByteBlockPtr file;
            try {
              file = UseEventingHelper::loadFile(fileName);
            } catch (const StdError &e) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load main configuration file: ") + mConfig.mConfigFile + ", error=" + string(e.result()) + ", reason=" + e.message());
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
                auto rootEl = UseEventingHelper::read(file);
                if (!rootEl) {
                  ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load file as JSON: ") + fileName);
                }
                if (!provider) {
                  provider = Provider::create(rootEl);
                } else {
                  provider->parse(rootEl);
                }
              } catch (const InvalidContent &e) {
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Failed to parse JSON configuration: " + e.message());
              }
              continue;
            }

            tool::output() << "\n[Info] Reading C/C++ source file: " << fileName << "\n\n";

            try {
              ParseState state;
              bool excluseMode = false;
              state.mPos = reinterpret_cast<const char *>(file->BytePtr());
              while ('\0' != *(state.mPos))
              {
                String line = getEventingLine(state);
                if (line.isEmpty()) continue;

                ULONG currentLine = state.mStartOfLineCount;

                String method;
                ArgumentMap args;
                parseLine(line.c_str(), method, args, currentLine);

                if (method.isEmpty()) continue;

                if (excluseMode) {
                  if (ZS_EVENTING_METHOD_EXCLUSIVE != method) {
                    tool::output() << "[Info] Exclusive mode skipping: " << line << "\n";
                    continue;
                  }
                }

                if (0 == (method.substr(0, strlen(ZS_EVENTING_METHOD_COMPACT_PREFIX)).compare(ZS_EVENTING_METHOD_COMPACT_PREFIX)))
                {
                  method = method.substr(strlen(ZS_EVENTING_METHOD_COMPACT_PREFIX));
                  ArgumentMap tempArgs;

                  for (size_t index = 0; index < args.size(); ++index)
                  {
                    if (index < ZS_EVENTING_METHOD_TOTAL_PARAMS) {
                      tempArgs[index] = args[index];
                      continue;
                    }

                    size_t resultIndex = index - ZS_EVENTING_METHOD_TOTAL_PARAMS;
                    if (0 == (resultIndex % 2)) {
                      String tempArg = args[index];
                      auto findPos = tempArg.find("/");
                      if (findPos == String::npos) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Compact parameter missing slash \"/\" in line: " + line);
                      }
                      String typeStr = tempArg.substr(0, findPos);
                      String nameStr = tempArg.substr(findPos + 1);
                      findPos = tempArg.find("/", findPos+1);
                      if (findPos != String::npos) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Compact parameter missing has extra slash \"/\" in line: " + line);
                      }
                      typeStr.trim();
                      nameStr.trim();
                      tempArgs[ZS_EVENTING_METHOD_TOTAL_PARAMS + ((resultIndex / 2)*3)] = typeStr;
                      tempArgs[ZS_EVENTING_METHOD_TOTAL_PARAMS + ((resultIndex / 2)*3) + 1] = nameStr;
                    } else {
                      tempArgs[ZS_EVENTING_METHOD_TOTAL_PARAMS + ((resultIndex / 2) * 3) + 2] = args[index];
                    }
                  }

                  args.clear();
                  args = tempArgs;
                }

                if (isNumber(method.c_str())) {
                  size_t totalParams{};
                  try
                  {
                    totalParams = Numeric<decltype(totalParams)>(method);
                  } catch (const Numeric<decltype(totalParams)>::ValueOutOfRange &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INTERNAL_ERROR, currentLine, "Event value out of range: " + line);
                  }

                  if (args.size() != (ZS_EVENTING_METHOD_TOTAL_PARAMS + (3 * totalParams))) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Number of parameters mismatch: " + line);
                  }

                  prepareProvider(mConfig);

                  //#define ZS_EVENTING_5(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode, xType1, xName1, xValue1, xType2, xName2, xValue2, xType3, xName3, xValue3, xType4, xName4, xValue4, xType5, xName5, xValue5)

                  auto event = IEventingTypes::Event::create();
                  event->mSubsystem = provider->aliasLookup(args[0]);
                  try {
                    event->mSeverity = Log::toSeverity(provider->aliasLookup(args[1]));
                  } catch (const InvalidArgument &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event has invalid severity: " + line);
                  }
                  try {
                    event->mLevel = Log::toLevel(provider->aliasLookup(args[2]));
                  } catch (const InvalidArgument &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event has invalid level: " + line);
                  }

                  event->mName = provider->aliasLookup(args[3]);
                  String channelID = provider->aliasLookup(args[4]);
                  String taskID = provider->aliasLookup(args[5]);
                  String opCode = provider->aliasLookup(args[6]);
                  
                  std::list<String> keywordIDs;

                  auto findKeywordPos = taskID.find("/");
                  if (findKeywordPos != String::npos) {
                    String keywordsStr = taskID.substr(findKeywordPos + 1);
                    taskID = taskID.substr(0, findKeywordPos);
                    taskID.trim();
                    taskID = provider->aliasLookup(taskID);
                    
                    while (true)
                    {
                      findKeywordPos = keywordsStr.find("/");
                      if (findKeywordPos == String::npos) {
                        keywordsStr.trim();
                        keywordIDs.push_back(provider->aliasLookup(keywordsStr));
                        break;
                      }
                      
                      String keywordID = keywordsStr.substr(0, findKeywordPos);
                      keywordID.trim();
                      keywordIDs.push_back(provider->aliasLookup(keywordID));
                      
                      keywordsStr = keywordsStr.substr(findKeywordPos + 1);
                    }
                  }

                  // map channel
                  {
                    auto found = provider->mChannels.find(channelID);
                    if (found == provider->mChannels.end()) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event has invalid channel: " + line);
                    }
                    event->mChannel = (*found).second;
                  }

                  // map task + task opcode
                  {
                    // map task
                    {
                      auto found = provider->mTasks.find(taskID);
                      if (found == provider->mTasks.end()) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event has invalid task: " + line);
                      }
                      event->mTask = (*found).second;
                    }
                    // map task opcode (if revelant)
                    {
                      auto found = event->mTask->mOpCodes.find(opCode);
                      if (found != event->mTask->mOpCodes.end()) {
                        event->mOpCode = (*found).second;
                      }
                    }
                  }
                  
                  // map keywords
                  {
                    for (auto iterKeywords = keywordIDs.begin(); iterKeywords != keywordIDs.end(); ++iterKeywords)
                    {
                      auto &keywordID = (*iterKeywords);
                      auto found = provider->mKeywords.find(keywordID);
                      if (found == provider->mKeywords.end()) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event has invalid keywords: " + line);
                      }

                      auto keyword = (*found).second;
                      event->mKeywords[keyword->mName] = keyword;
                    }
                  }

                  // map opcode
                  if (!event->mOpCode)
                  {
                    auto found = provider->mOpCodes.find(opCode);
                    if (found == provider->mOpCodes.end()) {
                      try {
                        auto predefinedOpCode = IEventingTypes::toPredefinedOpCode(opCode);
                        event->mOpCode = IEventingTypes::OpCode::create();
                        event->mOpCode->mName = opCode;
                        event->mOpCode->mValue = predefinedOpCode;
                        provider->mOpCodes[opCode] = event->mOpCode;
                        goto found_predefined_opcode;
                      } catch (const InvalidArgument &) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event has invalid opCode: " + line);
                      }
                    }
                    event->mOpCode = (*found).second;
                  }

                found_predefined_opcode: {}

                  IEventingTypes::DataTemplatePtr dataTemplate;
                  if (totalParams > 0) {
                    dataTemplate = IEventingTypes::DataTemplate::create();
                  }

                  NameSet nameSet;
                  for (decltype(totalParams) loop = 0; loop < totalParams; ++loop)
                  {
                    String type = provider->aliasLookup(args[ZS_EVENTING_METHOD_TOTAL_PARAMS + (loop * 3)]);
                    String name = provider->aliasLookup(args[ZS_EVENTING_METHOD_TOTAL_PARAMS + (loop * 3) + 1]);

                    String upperName = name;
                    upperName.toUpper();

                    if (nameSet.end() != nameSet.find(upperName)) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Event has duplicate name \"") + name + "\" in line: " + line);
                    }
                    nameSet.insert(upperName);

                    auto dataType = IEventingTypes::DataType::create();
                    dataType->mValueName = name;

                    // check if there's a typedef
                    {
                      auto found = provider->mTypedefs.find(type);
                      if (found != provider->mTypedefs.end()) {
                        dataType->mType = (*found).second->mType;
                      }
                      else {
                        try {
                          dataType->mType = IEventingTypes::toPredefinedTypedef(type);
                        } catch (const InvalidArgument &) {
                          ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Event has invalid type \"") + type + "\" in line: " + line);
                        }
                      }
                    }

                    dataTemplate->mDataTypes.push_back(dataType);
                  }

                  String dataTemplateHash;

                  if (dataTemplate) {
                    dataTemplateHash = dataTemplate->hash();

                    {
                      auto found = provider->mDataTemplates.find(dataTemplateHash);
                      if (found == provider->mDataTemplates.end()) {
                        provider->mDataTemplates[dataTemplateHash] = dataTemplate; // remember new template
                      }
                      else {
                        dataTemplate = (*found).second; // replace with existing template
                      }
                    }
                  }

                  event->mDataTemplate = dataTemplate;

                  {
                    auto found = provider->mEvents.find(event->mName);
                    if (found != provider->mEvents.end()) {
                      {
                        auto existingEvent = (*found).second;
                        if (event->mName != existingEvent->mName) goto reject_duplicate;
                        if (event->mSeverity != existingEvent->mSeverity) goto reject_duplicate;
                        if (event->mLevel != existingEvent->mLevel) goto reject_duplicate;
                        if (event->mSubsystem != existingEvent->mSubsystem) goto reject_duplicate;
                        String existingHash;
                        if (existingEvent->mDataTemplate) {
                          existingHash = existingEvent->mDataTemplate->hash();
                        }
                        if (dataTemplateHash != existingHash) goto reject_duplicate;
                        if (event->mChannel->mName != existingEvent->mChannel->mName) goto reject_duplicate;
                        if (event->mTask->mName != existingEvent->mTask->mName) goto reject_duplicate;
                        if (event->mOpCode->mName != existingEvent->mOpCode->mName) goto reject_duplicate;

                        goto skip_duplicate;
                      }
                    skip_duplicate:
                      {
                        tool::output() << "[Info] Found duplicate event \"" << event->mName << "\" in file \"" << fileName << "\"\n";
                        continue;
                      }
                    reject_duplicate:
                      {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Event has duplicate symbol \"") + event->mName + "\" but not all properties match in line: " + line);
                      }
                    }
                  }

                  tool::output() << "[Info] Found event \"" << event->mName << "\" in file \"" << fileName << "\"\n";
                  provider->mEvents[event->mName] = event;
                  continue;
                }

                if (ZS_EVENTING_METHOD_PROVIDER == method) {
                  if (5 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in provider \"" + string(args.size()) + "\" in line: " + line);
                  }

                  prepareProvider(mConfig);

                  String uuidStr = Helper::decodeQuotes(provider->aliasLookup(args[0]), state.mLineCount);
                  String nameStr = provider->aliasLookup(args[1]);
                  String symbolNameStr = Helper::decodeQuotes(provider->aliasLookup(args[2]), state.mLineCount);
                  String descriptionStr = Helper::decodeQuotes(provider->aliasLookup(args[3]), state.mLineCount);
                  String resouceNameStr = Helper::decodeQuotes(provider->aliasLookup(args[4]), state.mLineCount);

                  try {
                    decltype(provider->mID) id = Numeric<decltype(provider->mID)>(uuidStr);
                    if (!(!(provider->mID))) {
                      if (id != provider->mID) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Provider ID has been redefined \"") + uuidStr + "\" in line: " + line);
                      }
                    }
                    provider->mID = id;
                  }
                  catch (const Numeric<decltype(provider->mID)>::ValueOutOfRange &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Provider ID is invalid \"") + uuidStr + "\" in line: " + line);
                  }

                  if (provider->mName.hasData()) {
                    if (nameStr != provider->mName) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Provider name has been redefined \"") + nameStr + "\" in line: " + line);
                    }
                  }
                  provider->mName = nameStr;

                  if (provider->mSymbolName.hasData()) {
                    if (symbolNameStr != provider->mSymbolName) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Provider symbol name has been redefined \"") + symbolNameStr + "\" in line: " + line);
                    }
                  }
                  provider->mSymbolName = symbolNameStr;

                  if (provider->mDescription.hasData()) {
                    if (descriptionStr != provider->mDescription) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Provider description has been redefined \"") + descriptionStr + "\" in line: " + line);
                    }
                  }
                  provider->mDescription = descriptionStr;

                  if (provider->mResourceName.hasData()) {
                    if (resouceNameStr != provider->mResourceName) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Provider resource name has been redefined \"") + resouceNameStr + "\" in line: " + line);
                    }
                  }
                  provider->mResourceName = resouceNameStr;
                  tool::output() << "[Info] Found provider \"" << provider->mName << "\" in file \"" << fileName << "\"\n";
                  continue;
                }

                if (ZS_EVENTING_METHOD_ALIAS == method) {
                  if (2 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in alias \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);

                  {
                    auto found = provider->mAliases.find(args[0]);
                    if (found != provider->mAliases.end()) {
                      auto existingAlias = (*found).second;
                      if (existingAlias != args[1]) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Alias has been redefined \"") + args[0] + "\" in line: " + line);
                      }
                      continue;
                    }
                  }

                  provider->mAliases[args[0]] = args[1];
                  tool::output() << "[Info] Found alias: " << args[0] << " -> " << args[1] << "\n";
                  continue;
                }
                if (ZS_EVENTING_METHOD_SOURCE == method) {
                  if (1 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in source \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);
                  mConfig.mSourceFiles.push_back(Helper::decodeQuotes(provider->aliasLookup(args[0]), state.mLineCount));
                  tool::output() << "[Info] Found source: " << args[0] << "\n";
                  continue;
                }
                if (ZS_EVENTING_METHOD_INCLUDE == method) {
                  if (1 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in source \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);
                  mConfig.mSourceFiles.push_front(Helper::decodeQuotes(provider->aliasLookup(args[0]), state.mLineCount));
                  tool::output() << "[Info] Found include: " << args[0] << "\n";
                  continue;
                }
                if (ZS_EVENTING_METHOD_CHANNEL == method) {
                  if (3 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in channel \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);

                  auto channel = IEventingTypes::Channel::create();
                  channel->mID = provider->aliasLookup(args[0]);
                  channel->mName = Helper::decodeQuotes(provider->aliasLookup(args[1]), state.mLineCount);
                  try {
                    channel->mType = IEventingTypes::toOperationalType(provider->aliasLookup(args[2]));
                  }
                  catch (const InvalidArgument &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid operational type: " + line);
                  }

                  {
                    auto found = provider->mChannels.find(channel->mID);
                    if (found != provider->mChannels.end()) {
                      auto existingChannel = (*found).second;
                      if ((existingChannel->mID != channel->mID) ||
                        (existingChannel->mName != channel->mName) ||
                        (existingChannel->mType != channel->mType)) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Channel has been redefined \"") + channel->mID + "\" in line: " + line);
                      }
                      channel = existingChannel;
                    }
                    else {
                      provider->mChannels[channel->mID] = channel;
                    }
                  }
                  tool::output() << "[Info] Found channel: " << channel->mID << "\n";
                  continue;
                }
                if (ZS_EVENTING_METHOD_TASK == method) {
                  if (1 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in task \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);

                  auto task = IEventingTypes::Task::create();
                  task->mName = provider->aliasLookup(args[0]);

                  {
                    auto found = provider->mTasks.find(task->mName);
                    if (found == provider->mTasks.end()) {
                      provider->mTasks[task->mName] = task;
                    }
                  }
                  tool::output() << "[Info] Found task: " << task->mName << "\n";
                  continue;
                }

                if (ZS_EVENTING_METHOD_KEYWORD == method) {
                  if (1 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in keyword \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);
                  
                  auto keyword = IEventingTypes::Keyword::create();
                  keyword->mName = provider->aliasLookup(args[0]);
                  
                  {
                    auto found = provider->mKeywords.find(keyword->mName);
                    if (found == provider->mKeywords.end()) {
                      provider->mKeywords[keyword->mName] = keyword;
                    }
                  }
                  tool::output() << "[Info] Found keyword: " << keyword->mName << "\n";
                  continue;
                }

                if (ZS_EVENTING_METHOD_OPCODE == method) {
                  if (1 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in opcode \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);

                  auto opCode = IEventingTypes::OpCode::create();
                  opCode->mName = provider->aliasLookup(args[0]);

                  {
                    auto found = provider->mOpCodes.find(opCode->mName);
                    if (found == provider->mOpCodes.end()) {
                      provider->mOpCodes[opCode->mName] = opCode;
                    }
                  }
                  tool::output() << "[Info] Found opcode: " << opCode->mName << "\n";
                  continue;
                }

                if (ZS_EVENTING_METHOD_TASK_OPCODE == method) {
                  if (2 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in task opcode \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);

                  auto opCode = IEventingTypes::OpCode::create();
                  String taskName = provider->aliasLookup(args[0]);
                  opCode->mName = provider->aliasLookup(args[1]);

                  IEventingTypes::TaskPtr task;
                  {
                    auto found = provider->mTasks.find(taskName);
                    if (found == provider->mTasks.end()) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("Task is missing \"") + taskName + "\" in line: " + line);
                    }
                    task = (*found).second;
                  }

                  opCode->mTask = task;

                  {
                    auto found = task->mOpCodes.find(opCode->mName);
                    if (found == task->mOpCodes.end()) {
                      task->mOpCodes[opCode->mName] = opCode;
                    }
                  }
                  tool::output() << "[Info] Found task opcode: " << task->mName << ", " << opCode->mName << "\n";
                  continue;
                }

                if ((ZS_EVENTING_METHOD_REGISTER == method) ||
                    (ZS_EVENTING_METHOD_UNREGISTER == method)) {
                  if (1 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in (un)register \"" + string(args.size()) + "\" in line: " + line);
                  }
                  String providerName = provider->aliasLookup(args[0]);
                  prepareProvider(mConfig);
                  if (provider->mName.hasData()) {
                    if (provider->mName != providerName) {
                      ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, String("REGISTER does not match provider name \"") + provider->mName + "\" in line: " + line);
                    }
                  } else {
                    provider->mName = providerName;
                  }
                  tool::output() << "[Info] Found (un)register \"" << provider->mName << "\" in file \"" << fileName << "\"\n";
                  continue;
                }

                if (ZS_EVENTING_METHOD_EXCLUSIVE == method) {
                  prepareProvider(mConfig);
                  String providerName = provider->aliasLookup(args[0]);
                  if ("x" == providerName) {
                    tool::output() << "[Info] Contents are no longer exclusive\n";
                    excluseMode = false;
                  } else if (provider->mName != providerName) {
                    tool::output() << "[Info] Contents are exclusive to provider \"" << providerName << "\" and current provider is \"" <<  provider->mName << "\"\n";
                    excluseMode = true;
                  }
                  continue;
                }

                if (ZS_EVENTING_METHOD_ASSIGN_VALUE == method) {
                  if (2 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in assign value \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);
                  String eventName = provider->aliasLookup(args[0]);
                  String eventValue = provider->aliasLookup(args[1]);
                  auto found = provider->mEvents.find(eventName);
                  if (found == provider->mEvents.end()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event name not found \"" + eventName + "\" in line: " + line);
                  }
                  auto event = (*found).second;
                  try {
                    auto newValue = Numeric<decltype(event->mValue)>(eventValue);
                    if (0 != event->mValue) {
                      if (newValue != event->mValue) {
                        ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event found \"" + eventName + "\" but value mismatches previous value in line: " + line);
                      }
                    }
                    event->mValue = newValue;
                  } catch (const Numeric<decltype(event->mValue)>::ValueOutOfRange &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Event value not valid in event \"" + eventName + "\" in line: " + line);
                  }
                  tool::output() << "[Info] Assign event \"" << event->mName << "\" value \"" << string(event->mValue) << "\" in file \"" << fileName << "\"\n";
                  continue;
                }
                if (ZS_EVENTING_METHOD_SUBSYSTEM_DEFAULT_LEVEL == method) {
                  if (2 != args.size()) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid number of arguments in assign value \"" + string(args.size()) + "\" in line: " + line);
                  }
                  prepareProvider(mConfig);
                  String subsystemName = provider->aliasLookup(args[0]);
                  String subsystemLevel = provider->aliasLookup(args[1]);
                  
                  auto subsystem = IEventingTypes::Subsystem::create();
                  subsystem->mName = subsystemName;
                  
                  try {
                    subsystem->mLevel = zsLib::Log::toLevel(subsystemLevel);
                  } catch (const InvalidArgument &) {
                    ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, currentLine, "Invalid level \"" + subsystemLevel + "\" in line: " + line);
                  }

                  auto found = provider->mSubsystems.find(subsystemName);
                  if (found != provider->mSubsystems.end()) {
                    auto hash1 = subsystem->hash();
                    auto hash2 = (*found).second->hash();
                    if (hash1 != hash2) {
                      tool::output() << "[Warning] Subsystem default level has changed \"" << subsystem->mName << "\" and new level is \"" << zsLib::Log::toString(subsystem->mLevel) << "\"\n";
                    }
                  }

                  provider->mSubsystems[subsystem->mName] = subsystem;
                  tool::output() << "[Info] Assign subsystem \"" << subsystem->mName << "\" default level \"" << Log::toString(subsystem->mLevel) << "\" in file \"" << fileName << "\"\n";
                  continue;
                }

                ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_METHOD_NOT_UNDERSTOOD, currentLine, "Method is not valid: " + method);
              }
            } catch (const InvalidContent &e) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Invalid content found: " + e.message());
            } catch (const InvalidContentWithLine &e) {
              ZS_THROW_CUSTOM_PROPERTIES_2(FailureWithLine, ZS_EVENTING_TOOL_INVALID_CONTENT, e.lineNumber(), "Invalid content found: " + e.message());
            }
          }
        }

        //---------------------------------------------------------------------
        void EventingCompiler::prepareIndex() throw (Failure)
        {
          if (!mConfig.mProvider) return;

          ProviderPtr &provider = mConfig.mProvider;

          IndexSet consumedOpCodeIndexes;

          try {
            IndexSet consumedIndexes;

            for (auto iter = provider->mChannels.begin(); iter != provider->mChannels.end(); ++iter)
            {
              auto channel = (*iter).second;
              if (0 == channel->mValue) continue;
              if ((channel->mValue < 1) && (channel->mValue > (sizeof(BYTE)<<8))) {
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Channel index is not valid: " + string(channel->mValue));
              }
              insert(consumedIndexes, channel->mValue);
            }
          } catch (const InvalidArgument &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Channel index is not valid: " + e.message());
          }

          try {
            IndexSet consumedIndexes;

            for (auto iter = provider->mTasks.begin(); iter != provider->mTasks.end(); ++iter)
            {
              auto task = (*iter).second;
              if (0 == task->mValue) continue;
              if ((task->mValue < 1) && (task->mValue > 239)) {
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task index is not valid: " + string(task->mValue));
              }
              insert(consumedIndexes, task->mValue);
            }

            size_t current = 1;
            for (auto iter = provider->mTasks.begin(); iter != provider->mTasks.end(); ++iter)
            {
              auto task = (*iter).second;
              if (0 != task->mValue) continue;
              
              while (!insert(consumedIndexes, current, false)) { ++current; }
              task->mValue = current;
            }
            if (current > 239) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task index is too large: " + string(current));
            }
          } catch (const InvalidArgument &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task index is not valid: " + e.message());
          }

          try {
            // make sure to consume every index from every task
            for (auto iter = provider->mTasks.begin(); iter != provider->mTasks.end(); ++iter)
            {
              auto task = (*iter).second;

              for (auto iterOp = task->mOpCodes.begin(); iterOp != task->mOpCodes.end(); ++iterOp)
              {
                auto opCode = (*iterOp).second;
                if (0 == opCode->mValue) continue;
                insert(consumedOpCodeIndexes, opCode->mValue, false);
              }
            }

            for (auto iter = provider->mOpCodes.begin(); iter != provider->mOpCodes.end(); ++iter)
            {
              auto opCode = (*iter).second;
              if (0 == opCode->mValue) continue;
              if ((opCode->mValue < 10) && (opCode->mValue > 239)) {
                try {
                  auto predefinedType = IEventingTypes::toPredefinedOpCode(opCode->mName);
                  (void)predefinedType; // unused
                  goto found_predefined_opcode;
                } catch (const InvalidArgument &) {
                }
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "OpCode index is not valid: " + string(opCode->mValue));
              }
            found_predefined_opcode: {}
              insert(consumedOpCodeIndexes, opCode->mValue);
            }

            size_t current = 10;
            for (auto iter = provider->mOpCodes.begin(); iter != provider->mOpCodes.end(); ++iter)
            {
              auto opCode = (*iter).second;
              if (0 != opCode->mValue) continue;
              if (opCode->mName == IEventingTypes::toString(IEventingTypes::PredefinedOpCode_Info)) continue;

              while (!insert(consumedOpCodeIndexes, current, false)) { ++current; }
              opCode->mValue = current;
            }
            if (current > 239) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task index is too large: " + string(current));
            }
          } catch (const InvalidArgument &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task index is not valid: " + e.message());
          }

          try {
            for (auto iter = provider->mTasks.begin(); iter != provider->mTasks.end(); ++iter)
            {
              auto task = (*iter).second;

              IndexSet opIndexes(consumedOpCodeIndexes);  // never use a global opcode value

              for (auto iterOp = task->mOpCodes.begin(); iterOp != task->mOpCodes.end(); ++iterOp)
              {
                auto opCode = (*iterOp).second;
                if (0 == opCode->mValue) continue;
                if ((opCode->mValue < 10) && (opCode->mValue > 239)) {
                  ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "OpCode index is not valid: " + string(opCode->mValue));
                }
                insert(opIndexes, opCode->mValue);
              }

              size_t current = 10;
              for (auto iterOp = task->mOpCodes.begin(); iterOp != task->mOpCodes.end(); ++iterOp)
              {
                auto opCode = (*iterOp).second;
                if (0 != opCode->mValue) continue;
                while (!insert(opIndexes, current, false)) { ++current; }
                opCode->mValue = current;
              }
              if (current > 239) {
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task OpCode index is too large: " + string(current));
              }
            }
          } catch (const InvalidArgument &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Task OpCode index is not valid: " + e.message());
          }
          
          try {
            Index64Set consumedIndexes;
            
            for (auto iter = provider->mKeywords.begin(); iter != provider->mKeywords.end(); ++iter)
            {
              auto keyword = (*iter).second;
              if (0 == keyword->mMask) continue;
              insert64(consumedIndexes, keyword->mMask);
            }

            uint64_t current = 1;
            for (auto iter = provider->mKeywords.begin(); iter != provider->mKeywords.end(); ++iter)
            {
              auto keyword = (*iter).second;
              if (0 != keyword->mMask) continue;
              
              bool foundKeywordBitmask {};

              do {
                while (!insert64(consumedIndexes, current, false))
                {
                  current = current << 1;
                  if (0x8000000000000000ULL == current) {
                    ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Keyword mask exceeds maximum bitmask: " + keyword->mName);
                  }
                }

                // ensure this bit is not already consumed by any existing keyword(s)
                foundKeywordBitmask = false;
                for (auto iterSet = consumedIndexes.begin(); iterSet != consumedIndexes.end(); ++iterSet) {
                  auto &mask = (*iterSet);
                  if (mask == current) continue;  // skip if it was the same one added
                  
                  if (0 != (mask & current)) {
                    foundKeywordBitmask = true;
                    break;
                  }
                }
              } while (foundKeywordBitmask);

              keyword->mMask = current;
            }
          } catch (const InvalidArgument &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Keyword mask is not valid: " + e.message());
          }

          try {
            IndexSet consumedIndexes;

            for (auto iter = provider->mEvents.begin(); iter != provider->mEvents.end(); ++iter)
            {
              auto event = (*iter).second;
              if (0 == event->mValue) continue;
              insert(consumedIndexes, event->mValue);
            }

            size_t current = 1000;
            for (auto iter = provider->mEvents.begin(); iter != provider->mEvents.end(); ++iter)
            {
              auto event = (*iter).second;
              if (0 != event->mValue) continue;

              while (!insert(consumedIndexes, current, false)) { ++current; }
              event->mValue = current;
            }
          } catch (const InvalidArgument &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Event index is not valid: " + e.message());
          }
        }

        //---------------------------------------------------------------------
        void EventingCompiler::validate() throw (Failure)
        {
          ProviderPtr &provider = mConfig.mProvider;
          if (!provider) return;

          if (provider->mUniqueHash.isEmpty()) {
            provider->mUniqueHash = provider->uniqueEventingHash();
          }
        }

        //---------------------------------------------------------------------
        DocumentPtr EventingCompiler::generateManifest(const String &resourcePostFix) const throw (Failure)
        {
          const ProviderPtr &provider = mConfig.mProvider;

          DocumentPtr doc = Document::create();

          {
            // <?xml version='1.0' encoding='utf-8' standalone='yes'?>
            auto decl = Declaration::create();
            decl->setAttribute("version", "1.0");
            decl->setAttribute("encoding", "utf-8");
            decl->setAttribute("standalone", "yes");
            doc->adoptAsLastChild(decl);
            addEOL(doc);
          }

          {
            CommentPtr comment = Comment::create();
            comment->setValue(" " ZS_EVENTING_GENERATED_BY " ");
            doc->adoptAsLastChild(comment);
            addEOL(doc);
          }

          ElementPtr instrumentationManifestEl = Element::create("instrumentationManifest");
          ElementPtr instrumentationEl = Element::create("instrumentation");
          ElementPtr outerEventsEl = Element::create("events");
          ElementPtr providerEl = Element::create("provider");
          ElementPtr channelsEl = Element::create("channels");
          ElementPtr tasksEl = Element::create("tasks");
          ElementPtr keywordsEl = Element::create("keywords");
          ElementPtr opCodesEl = Element::create("opcodes");
          ElementPtr dataTemplatesEl = Element::create("templates");
          ElementPtr innerEventsEl = Element::create("events");

          ElementPtr localizationEl = Element::create("localization");
          ElementPtr resourcesEl = Element::create("resources");
          ElementPtr stringTableEl = Element::create("stringTable");

          //<instrumentationManifest xmlns="http://schemas.microsoft.com/win/2004/08/events">
          {
            instrumentationManifestEl->setAttribute("xmlns", "http://schemas.microsoft.com/win/2004/08/events");
            addEOL(instrumentationManifestEl);
            doc->adoptAsLastChild(instrumentationManifestEl);
            addEOL(doc);
          }
          //<instrumentation
          //  xmlns:win = "http://manifests.microsoft.com/win/2004/08/windows/events"
          //  xmlns:xs = "http://www.w3.org/2001/XMLSchema"
          //  xmlns:xsi = "http://www.w3.org/2001/XMLSchema-instance">
          {
            instrumentationEl->setAttribute("xmlns:win", "http://manifests.microsoft.com/win/2004/08/windows/events");
            instrumentationEl->setAttribute("xmlns:xs", "http://www.w3.org/2001/XMLSchema");
            instrumentationEl->setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
            instrumentationManifestEl->adoptAsLastChild(instrumentationEl);
            addEOL(instrumentationManifestEl);
          }
          //<events xmlns="http://schemas.microsoft.com/win/2004/08/events">
          {
            outerEventsEl->setAttribute("xmlns", "http://schemas.microsoft.com/win/2004/08/events");
            addEOL(outerEventsEl);
            instrumentationEl->adoptAsLastChild(outerEventsEl);
            addEOL(instrumentationEl);
          }

          //<provider
          //  guid = "{6586be19-7cf9-44f1-996b-3751e3549ccd}"
          //  name = "zsLib"
          //  message = "$(string.zsLib.ProviderMessage)"
          //  symbol = "ZSLIB_PROVIDER_GUID"
          //  messageFileName = "zsLib_ETWTracing.dll"
          //  resourceFileName = "zsLib_ETWTracing.dll" >
          {
            providerEl->setAttribute("guid", "{"+string(provider->mID) + "}");
            providerEl->setAttribute("name", provider->mName);
            providerEl->setAttribute("message", "$(string.Provider)");
            providerEl->setAttribute("symbol", "PROVIDER_" + toSymbol(provider->mSymbolName));
            providerEl->setAttribute("messageFileName", provider->mResourceName + resourcePostFix);
            providerEl->setAttribute("resourceFileName", provider->mResourceName + resourcePostFix);
            addEOL(providerEl);
            outerEventsEl->adoptAsLastChild(providerEl);
            addEOL(outerEventsEl);

            stringTableEl->adoptAsLastChild(createStringEl("Provider", provider->mName));
            addEOL(stringTableEl);
          }

          //<channels>
          //  <channel chid="zs"
          //    name="zsLib/Debug"
          //    type="Debug"
          //    symbol="CHANNEL_ZSLIB_DEBUG"
          //    message="$(string.Channel.zsLibDebug)" / >
          //</channels>
          if (provider->mChannels.size() > 0)
          {
            addEOL(channelsEl);
            for (auto iter = provider->mChannels.begin(); iter != provider->mChannels.end(); ++iter)
            {
              auto channel = (*iter).second;
              ElementPtr channelEl = Element::create("channel");
              channelEl->setAttribute("chid", channel->mID);
              channelEl->setAttribute("name", channel->mName + "/" + IEventingTypes::toString(channel->mType));
              channelEl->setAttribute("type", IEventingTypes::toString(channel->mType));
              channelEl->setAttribute("symbol", "CHANNEL_" + toSymbol(channel->mName));
              if (0 != channel->mValue) {
                channelEl->setAttribute("value", string(channel->mValue));
              }
              channelEl->setAttribute("message", "$(string.Channel." + channel->mID + ")");
              channelsEl->adoptAsLastChild(channelEl);
              addEOL(channelsEl);

              stringTableEl->adoptAsLastChild(createStringEl("Channel." + channel->mID, channel->mName));
              addEOL(stringTableEl);
            }
            providerEl->adoptAsLastChild(channelsEl);
            addEOL(providerEl);
          }

          //<tasks>
          //  <task name="Exception" symbol="TASK_EXCEPTION" value="1" message="$(string.Task.Exception)">
          //  <opcodes>
          //    <opcode name="Exception" symbol="OPCODE_EXCEPTION_EXCEPTION" value = "11" message="$(string.Opcode.Exception.Exception)" / >
          //  </opcodes>
          //</task>
          if (provider->mTasks.size() > 0) {
            addEOL(tasksEl);
            for (auto iter = provider->mTasks.begin(); iter != provider->mTasks.end(); ++iter)
            {
              auto task = (*iter).second;
              ElementPtr taskEl = Element::create("task");
              taskEl->setAttribute("name", task->mName);
              taskEl->setAttribute("symbol", "TASK_" + toSymbol(task->mName));
              if (0 != task->mValue) {
                taskEl->setAttribute("value", string(task->mValue));
              }
              taskEl->setAttribute("message", "$(string.Task." + task->mName + ")");
              if (task->mOpCodes.size() > 0) {
                addEOL(taskEl);
                ElementPtr taskOpCodesEl = Element::create("opcodes");
                addEOL(taskOpCodesEl);
                for (auto iterOpCode = task->mOpCodes.begin(); iterOpCode != task->mOpCodes.end(); ++iterOpCode)
                {
                  auto opCode = (*iterOpCode).second;
                  ElementPtr opCodeEl = Element::create("opcode");
                  opCodeEl->setAttribute("name", opCode->mName);
                  opCodeEl->setAttribute("symbol", "TASK_" + toSymbol(task->mName) + "_OPCODE_" + toSymbol(opCode->mName));
                  if ((0 != opCode->mValue) ||
                      (opCode->mName == IEventingTypes::toString(IEventingTypes::PredefinedOpCode_Info))) {
                    opCodeEl->setAttribute("value", string(opCode->mValue));
                  }
                  opCodeEl->setAttribute("message", "$(string.Task." + task->mName + ".OpCode." + opCode->mName + ")");
                  taskOpCodesEl->adoptAsLastChild(opCodeEl);
                  addEOL(taskOpCodesEl);

                  stringTableEl->adoptAsLastChild(createStringEl("Task." + task->mName + ".OpCode." + opCode->mName, opCode->mName));
                  addEOL(stringTableEl);
                }
                taskEl->adoptAsLastChild(taskOpCodesEl);
                addEOL(taskEl);
              }
              tasksEl->adoptAsLastChild(taskEl);
              addEOL(tasksEl);

              stringTableEl->adoptAsLastChild(createStringEl("Task." + task->mName, task->mName));
              addEOL(stringTableEl);
            }
            providerEl->adoptAsLastChild(tasksEl);
            addEOL(providerEl);
          }
          
          //<keywords>
          //  <keyword name="Read" mask="0x1" symbol="KEYWORD_READ" message="$(string.Keyword.Read)" />
          //</keywords>
          if (provider->mKeywords.size() > 0) {
            addEOL(keywordsEl);
            for (auto iter = provider->mKeywords.begin(); iter != provider->mKeywords.end(); ++iter)
            {
              auto keyword = (*iter).second;
              ElementPtr keywordEl = Element::create("keyword");
              keywordEl->setAttribute("name", keyword->mName);
              keywordEl->setAttribute("symbol", "KEYWORD_" + toSymbol(keyword->mName));
              if (0 != keyword->mMask) {
                keywordEl->setAttribute("mask", String("0x") + Stringize<uint64_t>(keyword->mMask, 16).string());
              }
              keywordEl->setAttribute("message", "$(string.Keyword." + keyword->mName + ")");

              keywordsEl->adoptAsLastChild(keywordEl);
              addEOL(keywordsEl);

              stringTableEl->adoptAsLastChild(createStringEl("Keyword." + keyword->mName, keyword->mName));
              addEOL(stringTableEl);
            }
            providerEl->adoptAsLastChild(keywordsEl);
            addEOL(providerEl);
          }

          if (provider->mOpCodes.size() > 0) {
            addEOL(opCodesEl);
            for (auto iter = provider->mOpCodes.begin(); iter != provider->mOpCodes.end(); ++iter)
            {
              auto opCode = (*iter).second;

              switch (static_cast<IEventingTypes::PredefinedOpCodes>(opCode->mValue))
              {
                case IEventingTypes::PredefinedOpCode_Info:       goto skip_next;
                case IEventingTypes::PredefinedOpCode_Start:      goto skip_next;
                case IEventingTypes::PredefinedOpCode_Stop:       goto skip_next;
                case IEventingTypes::PredefinedOpCode_DC_Start:   goto skip_next;
                case IEventingTypes::PredefinedOpCode_DC_Stop:    goto skip_next;
                case IEventingTypes::PredefinedOpCode_Extension:  goto skip_next;
                case IEventingTypes::PredefinedOpCode_Reply:      goto skip_next;
                case IEventingTypes::PredefinedOpCode_Resume:     goto skip_next;
                case IEventingTypes::PredefinedOpCode_Suspend:    goto skip_next;
                case IEventingTypes::PredefinedOpCode_Send:       goto skip_next;
                case IEventingTypes::PredefinedOpCode_Receive:    goto skip_next;
                default:  break;
              }

              {
                ElementPtr opCodeEl = Element::create("opcode");
                opCodeEl->setAttribute("name", opCode->mName);
                opCodeEl->setAttribute("symbol", "OPCODE_" + toSymbol(opCode->mName));
                if (0 != opCode->mValue) {
                  opCodeEl->setAttribute("value", string(opCode->mValue));
                }
                opCodeEl->setAttribute("message", "$(string.OpCode." + opCode->mName + ")");
                opCodesEl->adoptAsLastChild(opCodeEl);
                addEOL(opCodesEl);

                stringTableEl->adoptAsLastChild(createStringEl("OpCode." + opCode->mName, opCode->mName));
                addEOL(stringTableEl);
              }

            skip_next: {}
            }
            providerEl->adoptAsLastChild(opCodesEl);
            addEOL(providerEl);
          }
          
          if (provider->mDataTemplates.size() > 0) {
            addEOL(dataTemplatesEl);
            for (auto iter = provider->mDataTemplates.begin(); iter != provider->mDataTemplates.end(); ++iter)
            {
              auto dataTemplate = (*iter).second;
              String hash = dataTemplate->hash();

              ElementPtr dataTemplateEl = Element::create("template");
              dataTemplateEl->setAttribute("tid", "T_" + hash);
              addEOL(dataTemplateEl);

              dataTemplateEl->adoptAsLastChild(createDataEl("AnsiString", "_subsystem"));
              addEOL(dataTemplateEl);
              dataTemplateEl->adoptAsLastChild(createDataEl("AnsiString", "_function"));
              addEOL(dataTemplateEl);
              dataTemplateEl->adoptAsLastChild(createDataEl("UInt32", "_line"));
              addEOL(dataTemplateEl);

              IEventingTypes::DataTypePtr lastDataType;
              for (auto iterDataTemplate = dataTemplate->mDataTypes.begin(); iterDataTemplate != dataTemplate->mDataTypes.end(); ++iterDataTemplate)
              {
                auto dataType = (*iterDataTemplate);

                if (("_function" == dataType->mValueName) ||
                    ("_line" == dataType->mValueName) ||
                    ("_subsystem" == dataType->mValueName)) {
                  ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Function parameter cannot be named: " + dataType->mValueName);
                }

                {
                  if (lastDataType) {
                    if (IEventingTypes::PredefinedTypedef_binary == lastDataType->mType) {
                      if (IEventingTypes::PredefinedTypedef_size != dataType->mType) {
                        ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Template binary missing size: T_" + hash);
                      }
                      dataTemplateEl->adoptAsLastChild(createDataEl("UInt32", dataType->mValueName));
                      addEOL(dataTemplateEl);

                      // add the binary data type with length
                      {
                        ElementPtr dataEl = createDataEl("Binary", lastDataType->mValueName);
                        dataEl->setAttribute("length", dataType->mValueName);
                        dataTemplateEl->adoptAsLastChild(dataEl);
                        addEOL(dataTemplateEl);
                      }
                      goto next;
                    }
                  }
                  switch (dataType->mType) {
                    case IEventingTypes::PredefinedTypedef_bool:        dataTemplateEl->adoptAsLastChild(createDataEl("Boolean", dataType->mValueName)); addEOL(dataTemplateEl); break;

                    case IEventingTypes::PredefinedTypedef_byte:
                    case IEventingTypes::PredefinedTypedef_uint8:
                    case IEventingTypes::PredefinedTypedef_uchar:       dataTemplateEl->adoptAsLastChild(createDataEl("UInt8", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_int8:
                    case IEventingTypes::PredefinedTypedef_sint8:
                    case IEventingTypes::PredefinedTypedef_char:
                    case IEventingTypes::PredefinedTypedef_schar:       dataTemplateEl->adoptAsLastChild(createDataEl("Int8", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_word:
                    case IEventingTypes::PredefinedTypedef_uint16:
                    case IEventingTypes::PredefinedTypedef_ushort:      dataTemplateEl->adoptAsLastChild(createDataEl("UInt16", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_int16:
                    case IEventingTypes::PredefinedTypedef_sint16:
                    case IEventingTypes::PredefinedTypedef_short:
                    case IEventingTypes::PredefinedTypedef_sshort:      dataTemplateEl->adoptAsLastChild(createDataEl("Int16", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_int:
                    case IEventingTypes::PredefinedTypedef_sint:        
                    case IEventingTypes::PredefinedTypedef_int64:
                    case IEventingTypes::PredefinedTypedef_sint64:
                    case IEventingTypes::PredefinedTypedef_long:
                    case IEventingTypes::PredefinedTypedef_longlong:
                    case IEventingTypes::PredefinedTypedef_slonglong:   
                    case IEventingTypes::PredefinedTypedef_slong:       dataTemplateEl->adoptAsLastChild(createDataEl("Int64", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_size:
                    case IEventingTypes::PredefinedTypedef_uint:
                    case IEventingTypes::PredefinedTypedef_qword:
                    case IEventingTypes::PredefinedTypedef_uint64:
                    case IEventingTypes::PredefinedTypedef_ulong:       
                    case IEventingTypes::PredefinedTypedef_ulonglong:   dataTemplateEl->adoptAsLastChild(createDataEl("UInt64", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_dword:
                    case IEventingTypes::PredefinedTypedef_uint32:      dataTemplateEl->adoptAsLastChild(createDataEl("UInt32", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_int32:
                    case IEventingTypes::PredefinedTypedef_sint32:      dataTemplateEl->adoptAsLastChild(createDataEl("Int32", dataType->mValueName)); addEOL(dataTemplateEl); break;


                    case IEventingTypes::PredefinedTypedef_float32:
                    case IEventingTypes::PredefinedTypedef_float:       dataTemplateEl->adoptAsLastChild(createDataEl("Float", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_float64:
                    case IEventingTypes::PredefinedTypedef_ldouble:
                    case IEventingTypes::PredefinedTypedef_double:      dataTemplateEl->adoptAsLastChild(createDataEl("Double", dataType->mValueName)); addEOL(dataTemplateEl); break;

                    case IEventingTypes::PredefinedTypedef_void:
                    case IEventingTypes::PredefinedTypedef_pointer:     dataTemplateEl->adoptAsLastChild(createDataEl("Pointer", dataType->mValueName)); addEOL(dataTemplateEl); break;

                    case IEventingTypes::PredefinedTypedef_binary:      goto next;

                    case IEventingTypes::PredefinedTypedef_string:      
                    case IEventingTypes::PredefinedTypedef_astring:     dataTemplateEl->adoptAsLastChild(createDataEl("AnsiString", dataType->mValueName)); addEOL(dataTemplateEl); break;
                    case IEventingTypes::PredefinedTypedef_wstring:     dataTemplateEl->adoptAsLastChild(createDataEl("UnicodeString", dataType->mValueName)); addEOL(dataTemplateEl); break;
                  }
                }

              next: {}
                lastDataType = dataType;
              }
              if (lastDataType) {
                if (IEventingTypes::PredefinedTypedef_binary == lastDataType->mType) {
                  ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Template binary missing size: T_" + hash);
                }
              }
              dataTemplatesEl->adoptAsLastChild(dataTemplateEl);
              addEOL(dataTemplatesEl);
            }
          }

          // template basic
          {
            ElementPtr dataTemplateEl = Element::create("template");
            dataTemplateEl->setAttribute("tid", "T_Basic");
            addEOL(dataTemplateEl);
            dataTemplateEl->adoptAsLastChild(createDataEl("AnsiString", "_subsystem"));
            addEOL(dataTemplateEl);
            dataTemplateEl->adoptAsLastChild(createDataEl("AnsiString", "_function"));
            addEOL(dataTemplateEl);
            dataTemplateEl->adoptAsLastChild(createDataEl("UInt32", "_line"));
            addEOL(dataTemplateEl);

            dataTemplatesEl->adoptAsLastChild(dataTemplateEl);
            addEOL(dataTemplatesEl);
          }
          providerEl->adoptAsLastChild(dataTemplatesEl);
          addEOL(providerEl);

          if (provider->mEvents.size() > 0) {
            /*
            <events>
              <event symbol="ZsExceptionEventFired" channel="zs" template="T_Exception" task="Exception" keywords="Read Write" opcode="Exception" value="1001" level="win:Error" message="$(string.Event.ZsExceptionEventFired)" />
              <event symbol="ZsMessageQueueCreate" channel="zs" template="T_BasicThis" task="MessageQueue" opcode="win:Start" value="1101" level="win:Informational" message="$(string.Event.ZsMessageQueueCreate)" />
            */
            addEOL(innerEventsEl);
            for (auto iter = provider->mEvents.begin(); iter != provider->mEvents.end(); ++iter)
            {
              auto event = (*iter).second;
              auto eventEl = Element::create("event");
              if (event->mName.hasData()) {
                eventEl->setAttribute("symbol", event->mName);
              }
              if (event->mChannel) {
                eventEl->setAttribute("channel", event->mChannel->mID);
              }
              if (event->mDataTemplate) {
                eventEl->setAttribute("template", "T_" + event->mDataTemplate->hash());
              } else {
                eventEl->setAttribute("template", "T_Basic");
              }
              if (event->mTask) {
                eventEl->setAttribute("task", event->mTask->mName);
              }
              if (event->mKeywords.size() > 0) {
                String keywordsStr;
                for (auto iterKeywords = event->mKeywords.begin(); iterKeywords != event->mKeywords.end(); ++iterKeywords) {
                  auto keyword = (*iterKeywords).second;
                  if (keywordsStr.hasData()) {
                    keywordsStr += keyword->mName + " ";
                  } else {
                    keywordsStr += keyword->mName;
                  }
                }
                eventEl->setAttribute("keywords", keywordsStr);
              }
              if (event->mOpCode) {
                if ((event->mOpCode->mValue < 10) || (event->mOpCode->mValue > 239)) {
                  eventEl->setAttribute("opcode", "win:" + event->mOpCode->mName);
                } else {
                  eventEl->setAttribute("opcode", event->mOpCode->mName);
                }
              }
              if (0 != event->mValue) {
                eventEl->setAttribute("value", string(event->mValue));
              }
              eventEl->setAttribute("level", String("win:") + IEventingTypes::toString(IEventingTypes::toPredefinedLevel(event->mSeverity, event->mLevel)));
              if (event->mName.hasData()) {
                eventEl->setAttribute("message", "$(string.Event." + event->mName + ")");
                stringTableEl->adoptAsLastChild(createStringEl("Event." + event->mName, event->mName));
                addEOL(stringTableEl);
              }
              innerEventsEl->adoptAsLastChild(eventEl);
              addEOL(innerEventsEl);
            }

            providerEl->adoptAsLastChild(innerEventsEl);
            addEOL(providerEl);
          }


          //<localization>
          //  <resources culture="en-US">
          //    <stringTable>
          //      <string id="zsLib.ProviderMessage" value="zsLib Provider" / >
          //      <string id = "level.Critical" value="Critical" / >
          //      <string id = "level.Error" value="Error" / >
          //      <string id = "level.Warning" value="Warning" / >
          //      <string id = "level.Informational" value="Informational" / >
          //      <string id = "level.Verbose" value="Verbose" / >
          {
            resourcesEl->setAttribute("culture", "en-US");
            resourcesEl->adoptAsLastChild(stringTableEl);
            addEOL(resourcesEl);
            addEOL(localizationEl);
            localizationEl->adoptAsLastChild(resourcesEl);
            addEOL(localizationEl);
            instrumentationManifestEl->adoptAsLastChild(localizationEl);
            addEOL(instrumentationManifestEl);

            stringTableEl->adoptAsLastChild(createStringEl(String("level.") + IEventingTypes::toString(IEventingTypes::PredefinedLevel_Critical), IEventingTypes::toString(IEventingTypes::PredefinedLevel_Critical)));
            addEOL(stringTableEl);
            stringTableEl->adoptAsLastChild(createStringEl(String("level.") + IEventingTypes::toString(IEventingTypes::PredefinedLevel_Error), IEventingTypes::toString(IEventingTypes::PredefinedLevel_Error)));
            addEOL(stringTableEl);
            stringTableEl->adoptAsLastChild(createStringEl(String("level.") + IEventingTypes::toString(IEventingTypes::PredefinedLevel_Warning), IEventingTypes::toString(IEventingTypes::PredefinedLevel_Warning)));
            addEOL(stringTableEl);
            stringTableEl->adoptAsLastChild(createStringEl(String("level.") + IEventingTypes::toString(IEventingTypes::PredefinedLevel_Informational), IEventingTypes::toString(IEventingTypes::PredefinedLevel_Informational)));
            addEOL(stringTableEl);
            stringTableEl->adoptAsLastChild(createStringEl(String("level.") + IEventingTypes::toString(IEventingTypes::PredefinedLevel_Verbose), IEventingTypes::toString(IEventingTypes::PredefinedLevel_Verbose)));
            addEOL(stringTableEl);
          }

          return doc;
        }

        //---------------------------------------------------------------------
        DocumentPtr EventingCompiler::generateWprp() const throw (Failure)
        {
          const ProviderPtr &provider = mConfig.mProvider;

          DocumentPtr doc = Document::create();

          {
            // <?xml version='1.0' encoding='utf-8' standalone='yes'?>
            auto decl = Declaration::create();
            decl->setAttribute("version", "1.0");
            decl->setAttribute("encoding", "utf-8");
            decl->setAttribute("standalone", "yes");
            doc->adoptAsLastChild(decl);
            addEOL(doc);
          }

          {
            CommentPtr comment = Comment::create();
            comment->setValue(" " ZS_EVENTING_GENERATED_BY " ");
            doc->adoptAsLastChild(comment);
            addEOL(doc);
          }

          ElementPtr windowsPerformanceRecorderEl = Element::create("WindowsPerformanceRecorder");
          ElementPtr profilesEl = Element::create("Profiles");

          {
            //<WindowsPerformanceRecorder
            // Author="Robin Raymond"
            // Comments="zsLib Custom Provider Profile"
            // Version="1.0">
            windowsPerformanceRecorderEl->setAttribute("Author", mConfig.mAuthor.hasData() ? mConfig.mAuthor : "Unknown");
            windowsPerformanceRecorderEl->setAttribute("Comments", provider->mName);
            windowsPerformanceRecorderEl->setAttribute("Version", "1.0");
            addEOL(windowsPerformanceRecorderEl);
          }

          {
            addEOL(profilesEl);
            //<EventCollector Id="EventCollector_zsLib_Verbose" Name="zsLib Event Collector" Private="false" ProcessPrivate="false" Secure="false" Realtime="false">
            //  <BufferSize Value="1024"/>
            //  <Buffers Value="40"/>
            //</EventCollector>
            {
              ElementPtr eventCollectorEl = Element::create("EventCollector");
              eventCollectorEl->setAttribute("Id", "EventCollector_" + provider->mSymbolName + "_Verbose");
              eventCollectorEl->setAttribute("Name", provider->mSymbolName + " Event Collector");
              eventCollectorEl->setAttribute("Private", "false");
              eventCollectorEl->setAttribute("ProcessPrivate", "false");
              eventCollectorEl->setAttribute("Secure", "false");
              eventCollectorEl->setAttribute("Realtime", "false");
              addEOL(eventCollectorEl);
              {
                ElementPtr bufferSizeEl = Element::create("BufferSize");
                bufferSizeEl->setAttribute("Value", "1024");
                eventCollectorEl->adoptAsLastChild(bufferSizeEl);
                addEOL(eventCollectorEl);
              }
              {
                ElementPtr bufferEl = Element::create("Buffers");
                bufferEl->setAttribute("Value", "1024");
                eventCollectorEl->adoptAsLastChild(bufferEl);
                addEOL(eventCollectorEl);
              }
              profilesEl->adoptAsLastChild(eventCollectorEl);
              addEOL(profilesEl);
            }

            //<EventCollector Id="EventCollector_zsLib_Light" Name="zsLib Event Collector" Private="false" ProcessPrivate="false" Secure="false" Realtime="false">
            //  <BufferSize Value="128"/>
            //  <Buffers Value="40"/>
            //</EventCollector>
            {
              ElementPtr eventCollectorEl = Element::create("EventCollector");
              eventCollectorEl->setAttribute("Id", "EventCollector_" + provider->mSymbolName + "_Light");
              eventCollectorEl->setAttribute("Name", provider->mSymbolName + " Event Collector");
              eventCollectorEl->setAttribute("Private", "false");
              eventCollectorEl->setAttribute("ProcessPrivate", "false");
              eventCollectorEl->setAttribute("Secure", "false");
              eventCollectorEl->setAttribute("Realtime", "false");
              addEOL(eventCollectorEl);
              {
                ElementPtr bufferSizeEl = Element::create("BufferSize");
                bufferSizeEl->setAttribute("Value", "1024");
                eventCollectorEl->adoptAsLastChild(bufferSizeEl);
                addEOL(eventCollectorEl);
              }
              {
                ElementPtr bufferEl = Element::create("Buffers");
                bufferEl->setAttribute("Value", "1024");
                eventCollectorEl->adoptAsLastChild(bufferEl);
                addEOL(eventCollectorEl);
              }
              profilesEl->adoptAsLastChild(eventCollectorEl);
              addEOL(profilesEl);
            }
          }

          {
            // <EventProvider Id="EventProvider_zsLib" Name="6586be19-7cf9-44f1-996b-3751e3549ccd" />
            ElementPtr eventProviderEl = Element::create("EventProvider");

            eventProviderEl->setAttribute("Id", "EventProvider_" + provider->mSymbolName);
            eventProviderEl->setAttribute("Name", string(provider->mID));

            profilesEl->adoptAsLastChild(eventProviderEl);
            addEOL(profilesEl);
          }

          // Profile - Verbose.Memory
          {
            //<Profiles>
            //  <Profile
            //    Description="GeneralProfileForLargeServers"
            //    DetailLevel="Verbose"
            //    Id="GeneralProfileForLargeServers.Verbose.Memory"
            //    LoggingMode="Memory"
            //    Name="GeneralProfileForLargeServers"
            //    >
            //    <Collectors>
            //      <!-- EventCollectorId must match the EventCollector ID specified above -->
            //      <EventCollectorId Value="EventCollector_zsLib_Verbose">
            //        <EventProviders>
            //          <EventProviderId Value="EventProvider_zsLib"/>
            //        </EventProviders>
            //      </EventCollectorId>
            //    </Collectors>
            ElementPtr profileEl = Element::create("Profile");
            addEOL(profileEl);

            profileEl->setAttribute("Description", provider->mDescription);
            profileEl->setAttribute("DetailLevel", "Verbose");
            profileEl->setAttribute("Id", provider->mSymbolName + ".Verbose.Memory");
            profileEl->setAttribute("LoggingMode", "Memory");
            profileEl->setAttribute("Name", provider->mSymbolName);

            ElementPtr collectorsEl = Element::create("Collectors");
            addEOL(collectorsEl);
            ElementPtr eventCollectorIdEl = Element::create("EventCollectorId");
            addEOL(eventCollectorIdEl);
            eventCollectorIdEl->setAttribute("Value", "EventCollector_" + provider->mSymbolName + "_Verbose");
            ElementPtr eventProvidersEl = Element::create("EventProviders");
            addEOL(eventProvidersEl);
            ElementPtr eventProviderIdEl = Element::create("EventProviderId");
            eventProviderIdEl->setAttribute("Value",  "EventProvider_" + provider->mSymbolName);
            eventProvidersEl->adoptAsLastChild(eventProviderIdEl);
            addEOL(eventProvidersEl);

            eventCollectorIdEl->adoptAsLastChild(eventProvidersEl);
            addEOL(eventCollectorIdEl);
            collectorsEl->adoptAsLastChild(eventCollectorIdEl);
            addEOL(collectorsEl);
            profileEl->adoptAsLastChild(collectorsEl);
            addEOL(profileEl);

            profilesEl->adoptAsLastChild(profileEl);
            addEOL(profilesEl);
          }

          //<Profile
          //  Id="zsLibProfile.Light.File"
          //  LoggingMode="File"
          //  Name="zsLibProfile"
          //  DetailLevel="Light"
          //  Description="zsLib Provider for Diagnostic trace">
          //  <Collectors>
          //    <!-- EventCollectorId must match the EventCollector ID specified above -->
          //    <EventCollectorId Value="EventCollector_zsLib_Light">
          //      <EventProviders>
          //        <EventProviderId Value="EventProvider_zsLib"/>
          //      </EventProviders>
          //    </EventCollectorId>
          //  </Collectors>
          //</Profile>
          {
            ElementPtr profileEl = Element::create("Profile");
            addEOL(profileEl);

            profileEl->setAttribute("Description", provider->mDescription);
            profileEl->setAttribute("DetailLevel", "Light");
            profileEl->setAttribute("Id", provider->mSymbolName + ".Light.Memory");
            profileEl->setAttribute("LoggingMode", "Memory");
            profileEl->setAttribute("Name", provider->mSymbolName);

            ElementPtr collectorsEl = Element::create("Collectors");
            addEOL(collectorsEl);
            ElementPtr eventCollectorIdEl = Element::create("EventCollectorId");
            addEOL(eventCollectorIdEl);
            eventCollectorIdEl->setAttribute("Value", "EventCollector_" + provider->mSymbolName + "_Light");
            ElementPtr eventProvidersEl = Element::create("EventProviders");
            addEOL(eventProvidersEl);
            ElementPtr eventProviderIdEl = Element::create("EventProviderId");
            eventProviderIdEl->setAttribute("Value", "EventProvider_" + provider->mSymbolName);
            eventProvidersEl->adoptAsLastChild(eventProviderIdEl);
            addEOL(eventProvidersEl);

            eventCollectorIdEl->adoptAsLastChild(eventProvidersEl);
            addEOL(eventCollectorIdEl);
            collectorsEl->adoptAsLastChild(eventCollectorIdEl);
            addEOL(collectorsEl);
            profileEl->adoptAsLastChild(collectorsEl);
            addEOL(profileEl);

            profilesEl->adoptAsLastChild(profileEl);
            addEOL(profilesEl);
          }

          {
            //<Profile
            //  Id="zsLibProfile.Verbose.Memory"
            //  LoggingMode="Memory"
            //  Name="zsLibProfile"
            //  DetailLevel="Verbose"
            //  Description="zsLib Provider for Diagnostic trace"
            //  Base="zsLibProfile.Verbose.File"/>
            ElementPtr profileEl = Element::create("Profile");

            profileEl->setAttribute("Description", provider->mDescription);
            profileEl->setAttribute("DetailLevel", "Verbose");
            profileEl->setAttribute("Id", provider->mSymbolName + ".Verbose.File");
            profileEl->setAttribute("LoggingMode", "File");
            profileEl->setAttribute("Name", provider->mSymbolName);
            profileEl->setAttribute("Base", provider->mSymbolName + ".Verbose.Memory");

            profilesEl->adoptAsLastChild(profileEl);
            addEOL(profilesEl);
          }

          {
            //<Profile
            //  Id="zsLibProfile.Light.Memory"
            //  LoggingMode="Memory"
            //  Name="zsLibProfile"
            //  DetailLevel="Light"
            //  Description="zsLib Provider for Diagnostic trace"
            //  Base="zsLibProfile.Light.File"/>
            ElementPtr profileEl = Element::create("Profile");

            profileEl->setAttribute("Description", provider->mDescription);
            profileEl->setAttribute("DetailLevel", "Light");
            profileEl->setAttribute("Id", provider->mSymbolName + ".Light.File");
            profileEl->setAttribute("LoggingMode", "File");
            profileEl->setAttribute("Name", provider->mSymbolName);
            profileEl->setAttribute("Base", provider->mSymbolName + ".Light.Memory");

            profilesEl->adoptAsLastChild(profileEl);
            addEOL(profilesEl);
          }

          windowsPerformanceRecorderEl->adoptAsLastChild(profilesEl);
          addEOL(windowsPerformanceRecorderEl);
          doc->adoptAsLastChild(windowsPerformanceRecorderEl);
          addEOL(doc);

          return doc;
        }

        //---------------------------------------------------------------------
        DocumentPtr EventingCompiler::generateJsonMan() const throw (Failure)
        {
          DocumentPtr doc = Document::create();
          ElementPtr providerEl = mConfig.mProvider->createElement();

          ElementPtr commentEl = UseEventingHelper::createElementWithTextAndJSONEncode("comment", ZS_EVENTING_GENERATED_BY);
          providerEl->adoptAsFirstChild(commentEl);
          doc->adoptAsLastChild(providerEl);
          return doc;
        }

        //---------------------------------------------------------------------
        static const char *getFunctions()
        {
          static const char *functions =
          "\n";

          return functions;
        }

        //---------------------------------------------------------------------
        SecureByteBlockPtr EventingCompiler::generateXPlatformEventsHeader(
                                                                           const String &outputNameXPlatform,
                                                                           const String &outputNameWindows
                                                                           ) const throw (Failure)
        {
          std::stringstream ss;

          const ProviderPtr &provider = mConfig.mProvider;
          if (!provider) return SecureByteBlockPtr();

          ss << "// " ZS_EVENTING_GENERATED_BY "\n\n";
          ss << "#pragma once\n\n";
          ss << "#include <zsLib/eventing/noop.h>\n";
          ss << "#include <zsLib/eventing/Log.h>\n";
          ss << "#include <stdint.h>\n\n";
          ss << "namespace zsLib {\n";
          ss << "  namespace eventing {\n";

          ss << getFunctions();
          
          String getEventingHandleFunction = "getEventHandle_" + provider->mName + "()";
          String getEventingHandleFunctionWithNamespace = "::zsLib::eventing::" + getEventingHandleFunction;

          ss <<
            "\n"
            "    inline zsLib::Log::ProviderHandle &" << getEventingHandleFunction << "\n"
            "    {\n"
            "      static zsLib::Log::ProviderHandle gHandle {};\n"
            "      return gHandle;\n"
            "    }\n\n";

          ss << "#define ZS_INTERNAL_REGISTER_EVENTING_" << provider->mName << "() \\\n";
          ss << "    { \\\n";
          ss << "      ZS_EVENTING_REGISTER_EVENT_WRITER(" << getEventingHandleFunctionWithNamespace << ", \"" << string(provider->mID) << "\", \"" << provider->mName << "\", \"" << provider->mUniqueHash << "\"); \\\n";
          for (auto iter = provider->mSubsystems.begin(); iter != provider->mSubsystems.end(); ++iter) {
            auto subsystem = (*iter).second;
            ss << "      ZS_EVENTING_REGISTER_SUBSYSTEM_DEFAULT_LEVEL(" << subsystem->mName << ", " << zsLib::Log::toString(subsystem->mLevel) << "); \\\n";
          }
          ss << "    }\n";
          ss << "\n";

          ss << "#define ZS_INTERNAL_UNREGISTER_EVENTING_" << provider->mName << "() ZS_EVENTING_UNREGISTER_EVENT_WRITER(" << getEventingHandleFunctionWithNamespace << ")\n\n";

          for (auto iter = provider->mEvents.begin(); iter != provider->mEvents.end(); ++iter)
          {
            auto event = (*iter).second;

//            inline const USE_EVENT_DESCRIPTOR *getEventDescriptor_ExceptionEvent()
//            {
//              static const USE_EVENT_DESCRIPTOR description {0,0,0,0,0,0,0x8000000000000000};
//              return &description;
//            }
            
//            typedef struct _EVENT_DESCRIPTOR {
//              USHORT    Id;
//              UCHAR     Version;
//              UCHAR     Channel;
//              UCHAR     Level;
//              UCHAR     Opcode;
//              USHORT    Task;
//              ULONGLONG Keyword;
//            }

            String keywordValue = "(0x";
            if (event->mKeywords.size() > 0) {
              uint64_t mask = 0;
              for (auto iterKeywords = event->mKeywords.begin(); iterKeywords != event->mKeywords.end(); ++iterKeywords)
              {
                auto keyword = (*iterKeywords).second;
                mask = mask | keyword->mMask;
              }
              keywordValue += Stringize<decltype(mask)>(mask, 16).string();
            }
            else {
              keywordValue += "8000000000000000";
            }

            keywordValue += "ULL)";
            
            {
              ss << "\n";
              ss << "    inline const USE_EVENT_DESCRIPTOR *getEventDescriptor_" << event->mName << "()\n";
              ss << "    {\n";
              ss << "      static const USE_EVENT_DESCRIPTOR description {";
              
              ss << string(event->mValue) << ", ";
              ss << "0, "; // version not supported
              if (event->mChannel) {
                ss << string(event->mChannel->mValue) << ", ";
              } else {
                ss << "0, ";
              }
              ss <<  string(static_cast<std::underlying_type<IEventingTypes::PredefinedLevels>::type>(IEventingTypes::toPredefinedLevel(event->mSeverity, event->mLevel))) << ", ";
              
              if (event->mOpCode) {
                ss << string(event->mOpCode->mValue) << ", ";
              } else {
                ss << "0, ";
              }
              
              if (event->mTask) {
                ss << string(event->mTask->mValue) << ", ";
              } else {
                ss << "0, ";
              }
              
              ss << keywordValue;
              ss << "};\n";
              ss << "      return &description;\n";
              ss << "    }\n";
            }
            
//            enum EventParameterTypes
//            {
//              EventParameterType_Boolean = 1,
//              EventParameterType_UnsignedInteger = 2,
//              EventParameterType_SignedInteger = 2 | 4,
//              EventParameterType_FloatingPoint = 8,
//              EventParameterType_Pointer = 16,
//              EventParameterType_Binary = 16 | 32,
//              EventParameterType_String = 16 | 64,
//            };
            

            {
              ss << "\n";
              ss << "    inline const USE_EVENT_PARAMETER_DESCRIPTOR *getEventParameterDescriptor_" << event->mName << "()\n";
              ss << "    {\n";
              ss << "      static const USE_EVENT_PARAMETER_DESCRIPTOR descriptions [] =\n";
              ss << "      {\n";
              ss << "        {EventParameterType_AString},\n";
              ss << "        {EventParameterType_AString},\n";
              ss << "        {EventParameterType_UnsignedInteger}";
              
              if (event->mDataTemplate) {
                bool nextMustBeSize = false;
                for (auto iterDataType = event->mDataTemplate->mDataTypes.begin(); iterDataType != event->mDataTemplate->mDataTypes.end(); ++iterDataType) {
                  auto dataType = (*iterDataType);
                  if (nextMustBeSize) {
                    nextMustBeSize = false;
                    continue;
                  }

                  String sizeTypeStr;
                  String typeStr;

                  switch (IEventingTypes::getBaseType(dataType->mType))
                  {
                    case IEventingTypes::BaseType_Boolean:  typeStr = "Boolean"; break;
                    case IEventingTypes::BaseType_Integer:  {
                      if (IEventingTypes::isSigned(dataType->mType)) {
                        typeStr = "SignedInteger";
                      } else {
                        typeStr = "UnsignedInteger";
                      }
                      break;
                    }
                    case IEventingTypes::BaseType_Float:    typeStr = "FloatingPoint"; break;
                    case IEventingTypes::BaseType_Pointer:  typeStr = "Pointer"; break;
                    case IEventingTypes::BaseType_Binary:   typeStr = "Binary"; sizeTypeStr = "UnsignedInteger"; nextMustBeSize = true; break;
                    case IEventingTypes::BaseType_String:   {
                      if (IEventingTypes::isAString(dataType->mType)) {
                        typeStr = "AString";
                      } else {
                        typeStr = "WString";
                      }
                      break;
                    }
                  }

                  ss << ",\n";
                  if (sizeTypeStr.hasData()) {
                    ss << "        {EventParameterType_" << sizeTypeStr << "},\n";
                  }
                  ss << "        {EventParameterType_" << typeStr << "}";
                }
              }
              
              ss << "\n";
              ss << "      };\n";
              ss << "      return &(descriptions[0]);\n";
              ss << "    }\n";
            }

            {
              ss << "\n";
              ss << "#define ZS_INTERNAL_EVENTING_EVENT_" << event->mName << "(xSubsystem";

              if (!event->mDataTemplate) goto declaration_done;
              if (event->mDataTemplate->mDataTypes.size() < 1) goto declaration_done;

              for (size_t loop = 1; loop <= event->mDataTemplate->mDataTypes.size(); ++loop)
              {
                ss << ", xValue" << string(loop);
              }
            }

          declaration_done:
            {
              ss << ") \\\n";
            }

            {
              String getCurrentSubsystemStr;
              if ("x" == event->mSubsystem) {
                ss << "  if (ZS_EVENTING_IS_LOGGING(" << getEventingHandleFunctionWithNamespace << ", " << keywordValue << ", " << Log::toString(event->mLevel) << ")) { \\\n";
                getCurrentSubsystemStr = "(ZS_GET_SUBSYSTEM())";
              } else {
                ss << "  if (ZS_EVENTING_IS_SUBSYSTEM_LOGGING(" << getEventingHandleFunctionWithNamespace << ", " << keywordValue << ", xSubsystem, " << Log::toString(event->mLevel) << ")) { \\\n";
                getCurrentSubsystemStr = "(xSubsystem)";
              }
              

              size_t totalDataTypes = 0;
              size_t totalPointerTypes = 0;
              size_t totalStringTypes = 0;

              if (event->mDataTemplate) {
                for (auto iterDataType = event->mDataTemplate->mDataTypes.begin(); iterDataType != event->mDataTemplate->mDataTypes.end(); ++iterDataType) {
                  auto dataType = (*iterDataType);
                  switch (IEventingTypes::getBaseType(dataType->mType))
                  {
                    case IEventingTypes::BaseType_Boolean:
                    case IEventingTypes::BaseType_Integer:
                    case IEventingTypes::BaseType_Float:
                    case IEventingTypes::BaseType_Pointer:  ++totalDataTypes; break;
                    case IEventingTypes::BaseType_Binary:   ++totalPointerTypes; break;
                    case IEventingTypes::BaseType_String:   ++totalStringTypes; break;
                  }
                }
              }
              
              size_t totalTypes = totalDataTypes + totalPointerTypes + totalStringTypes;
              
#define ZS_EVENTING_TOTAL_BUILT_IN_DATA_EVENT_TYPES (3)
              
              ss << "    ::zsLib::eventing::USE_EVENT_DATA_DESCRIPTOR xxDescriptors[" << string(ZS_EVENTING_TOTAL_BUILT_IN_DATA_EVENT_TYPES+totalTypes) << "]; \\\n";
              ss << "    uint32_t xxLineNumber = __LINE__; \\\n";
              ss << "    \\\n";
              ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_ASTR(&(xxDescriptors[0]), " << getCurrentSubsystemStr << ".getName()); \\\n";
              ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_ASTR(&(xxDescriptors[1]), __func__); \\\n";
              ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_VALUE(&(xxDescriptors[2]), &xxLineNumber, sizeof(xxLineNumber)); \\\n";
              ss << "    \\\n";
              
              size_t current = ZS_EVENTING_TOTAL_BUILT_IN_DATA_EVENT_TYPES;

              bool nextMustBeSize = false;
              size_t loop = 1;
              if (event->mDataTemplate) {
                for (auto iterDataType = event->mDataTemplate->mDataTypes.begin(); iterDataType != event->mDataTemplate->mDataTypes.end(); ++iterDataType, ++loop) {
                  auto dataType = (*iterDataType);

                  String originalValueStr = "(xValue" + string(loop) + ")";
                  String newValueStr = "xxVal" + string(current);

                  bool isDataType = true;
                  switch (dataType->mType)
                  {
                    case IEventingTypes::PredefinedTypedef_bool: {
                      ss << "    ::zsLib::eventing::USE_EVENT_DATA_BOOL_TYPE " << newValueStr << " {" << originalValueStr << " ? 1 : 0}; \\\n";
                      break;
                    }

                    case IEventingTypes::PredefinedTypedef_uchar:
                    case IEventingTypes::PredefinedTypedef_ushort:
                    case IEventingTypes::PredefinedTypedef_uint:
                    case IEventingTypes::PredefinedTypedef_ulong:
                    case IEventingTypes::PredefinedTypedef_ulonglong:
                    case IEventingTypes::PredefinedTypedef_uint8:
                    case IEventingTypes::PredefinedTypedef_uint16:
                    case IEventingTypes::PredefinedTypedef_uint32:
                    case IEventingTypes::PredefinedTypedef_uint64:
                    case IEventingTypes::PredefinedTypedef_byte:
                    case IEventingTypes::PredefinedTypedef_word:
                    case IEventingTypes::PredefinedTypedef_dword:
                    case IEventingTypes::PredefinedTypedef_qword: {
                      String typeStr = String("uint") + string(IEventingTypes::getMaxBytes(dataType->mType) * 8) + "_t";
                      ss << "    " << typeStr << " " << newValueStr << "{" << originalValueStr << "}; \\\n";
                      break;
                    }

                    case IEventingTypes::PredefinedTypedef_char:
                    case IEventingTypes::PredefinedTypedef_schar:
                    case IEventingTypes::PredefinedTypedef_short:
                    case IEventingTypes::PredefinedTypedef_sshort:
                    case IEventingTypes::PredefinedTypedef_int:
                    case IEventingTypes::PredefinedTypedef_sint:
                    case IEventingTypes::PredefinedTypedef_long:
                    case IEventingTypes::PredefinedTypedef_slong:
                    case IEventingTypes::PredefinedTypedef_longlong:
                    case IEventingTypes::PredefinedTypedef_slonglong:
                    case IEventingTypes::PredefinedTypedef_int8:
                    case IEventingTypes::PredefinedTypedef_sint8:
                    case IEventingTypes::PredefinedTypedef_int16:
                    case IEventingTypes::PredefinedTypedef_sint16:
                    case IEventingTypes::PredefinedTypedef_int32:
                    case IEventingTypes::PredefinedTypedef_sint32:
                    case IEventingTypes::PredefinedTypedef_int64:
                    case IEventingTypes::PredefinedTypedef_sint64: {
                      String typeStr = String("int") + string(IEventingTypes::getMaxBytes(dataType->mType) * 8) + "_t";
                      ss << "    " << typeStr << " " << newValueStr << "{" << originalValueStr << "}; \\\n";
                      break;
                    }

                    case IEventingTypes::PredefinedTypedef_float:
                    case IEventingTypes::PredefinedTypedef_float32: {
                      ss << "    float " << newValueStr << "{" << originalValueStr << "}; \\\n";
                      break;
                    }
                    case IEventingTypes::PredefinedTypedef_double:
                    case IEventingTypes::PredefinedTypedef_float64: {
                      ss << "    double " << newValueStr << "{" << originalValueStr << "}; \\\n";
                      break;
                    }
                    case IEventingTypes::PredefinedTypedef_ldouble: {
                      ss << "    long double " << newValueStr << "{" << originalValueStr << "}; \\\n";
                      break;
                    }

                    case IEventingTypes::PredefinedTypedef_void:
                    case IEventingTypes::PredefinedTypedef_pointer: {
                      ss << "    uintptr_t " << newValueStr << " = reinterpret_cast<uintptr_t>(" << originalValueStr << "); \\\n";
                      break;
                    }

                    case IEventingTypes::PredefinedTypedef_binary: {
                      isDataType = false;
                      
                      String newValueStrPlus1 = "xxVal" + string(current+1);
                      String oldValueStrPlus1 = "(xValue" + string(loop+1) + ")";
                      
                      ss << "    auto " << newValueStr << " = " << originalValueStr << "; \\\n";
                      ss << "    uint32_t " << newValueStrPlus1 << " {static_cast<uint32_t>" << oldValueStrPlus1 << "}; \\\n";
                      ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_VALUE(&(xxDescriptors[" << current << "]), &(" << newValueStrPlus1 << "), sizeof(" << newValueStrPlus1 << ")); \\\n";
                      ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_BUFFER(&(xxDescriptors[" << string(current+1) << "]), " << newValueStr << ", " << newValueStrPlus1 << "); \\\n";

                      if (loop + 1 > event->mDataTemplate->mDataTypes.size()) {
                        ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, String("Binary data missing size"));
                      }
                      nextMustBeSize = true;
                      goto next_loop;
                    }
                    case IEventingTypes::PredefinedTypedef_size: {
                      isDataType = false;
                      nextMustBeSize = false;
                      break;
                    }

                    case IEventingTypes::PredefinedTypedef_string:
                    case IEventingTypes::PredefinedTypedef_astring: {
                      isDataType = false;
                      ss << "    auto " << newValueStr << " = " << originalValueStr << "; \\\n";
                      ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_ASTR(&(xxDescriptors[" << current << "]), " << newValueStr << "); \\\n";
                      break;
                    }
                    case IEventingTypes::PredefinedTypedef_wstring: {
                      isDataType = false;
                      ss << "    auto " << newValueStr << " = " << originalValueStr << "; \\\n";
                      ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_WSTR(&(xxDescriptors[" << current << "]), " << newValueStr << "); \\\n";
                      break;
                    }
                  }

                  {
                    if (isDataType) {
                      ss << "    ZS_EVENTING_EVENT_DATA_DESCRIPTOR_FILL_VALUE(&(xxDescriptors[" << current << "]), &(" << newValueStr << "), sizeof(" << newValueStr << ")); \\\n";
                    }
                    if (nextMustBeSize) {
                      ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, String("Binary data missing size"));
                    }
                  }

                next_loop:
                  {
                    ++current;
                  }
                }
              }

              ss << "    ZS_EVENTING_WRITE_EVENT(" << getEventingHandleFunctionWithNamespace << ", " << Log::toString(event->mSeverity) << ", " << Log::toString(event->mLevel) << ", ::zsLib::eventing::getEventDescriptor_" << event->mName << "(), ::zsLib::eventing::getEventParameterDescriptor_" << event->mName << "(), &(xxDescriptors[0]), " << string(ZS_EVENTING_TOTAL_BUILT_IN_DATA_EVENT_TYPES+totalTypes) << "); \\\n";
              ss << "  }\n";
            }
          }

          ss << "\n";
          ss << "  } // namespace eventing\n";
          ss << "} // namespace zsLib\n\n";

          return UseEventingHelper::convertToBuffer(ss.str());
        }

        //---------------------------------------------------------------------
        SecureByteBlockPtr EventingCompiler::generateWindowsEventsHeader(
                                                                         const String &outputNameXPlatform,
                                                                         const String &outputNameWindows,
                                                                         const String &outputNameWindowsETW
                                                                         ) const throw (Failure)
        {
          std::stringstream ss;

          const ProviderPtr &provider = mConfig.mProvider;
          if (!provider) return SecureByteBlockPtr();

          ss << "/*\n";
          ss << " " << ZS_EVENTING_GENERATED_BY << "\n";
          ss <<
            " - Stand-alone compiler header for generating ETW events on Windows and NO-OP events cross-platform.\n"
            "*/\n"
            "\n"
            "#pragma once\n"
            "\n"
            "#ifdef ZS_EVENTING_NOOP\n"
            "\n"
            "#include <zsLib/eventing/noop.h>\n"
            "\n"
            "#else /* ZS_EVENTING_NOOP */\n"
            "\n"
            "#ifndef ZS_EVENTING_IS_SUBSYSTEM_LOGGING\n"
            "#define ZS_EVENTING_IS_SUBSYSTEM_LOGGING(xSubsystem, xLevel)\n"
            "#endif /* ZS_EVENTING_IS_SUBSYSTEM_LOGGING */\n"
            "\n"
            "#ifndef ZS_EVENTING_IS_LOGGING\n"
            "#define ZS_EVENTING_IS_LOGGING(xLevel)\n"
            "#endif /* ZS_EVENTING_IS_LOGGING */\n"
            "\n"
            "#ifndef ZS_EVENTING_GET_SUBSYSTEM_NAME\n"
            "#define ZS_EVENTING_GET_SUBSYSTEM_NAME ((const char *)NULL)\n"
            "#endif /* ZS_EVENTING_GET_SUBSYSTEM_NAME */\n"
            "\n"
            "#ifndef ZS_EVENTING_EXCLUSIVE\n"
            "#define ZS_EVENTING_EXCLUSIVE(xProviderName)\n"
            "#define ZS_EVENTING_PROVIDER(xUUID, xName, xSymbolName, xDescription, xResourceName)\n"
            "#define ZS_EVENTING_ALIAS(xAliasInput, xAliasOuput)\n"
            "#define ZS_EVENTING_INCLUDE(xSourceStr)\n"
            "#define ZS_EVENTING_SOURCE(xSourceStr)\n"
            "#define ZS_EVENTING_CHANNEL(xID, xNameStr, xOperationalType)\n"
            "#define ZS_EVENTING_TASK(xName)\n"
            "#define ZS_EVENTING_KEYWORD(xName)\n"
            "#define ZS_EVENTING_OPCODE(xName)\n"
            "#define ZS_EVENTING_TASK_OPCODE(xTaskName, xOpCodeName)\n"
            "#define ZS_EVENTING_ASSIGN_VALUE(xSymbol, xValue)\n"
            "#define ZS_EVENTING_SUBSYSTEM_DEFAULT_LEVEL(xSubsystemName, xLevel)\n"
            "#endif /* ZS_EVENTING_EXCLUSIVE */\n"
            "\n"
            "#ifndef _WIN32\n"
            "\n"
            "#ifndef ZS_EVENTING_REGISTER\n"
            "#define ZS_EVENTING_REGISTER(xProviderName)\n"
            "#define ZS_EVENTING_UNREGISTER(xProviderName)\n"
            "\n";

          for (int index = 0; index <= 38; ++index)
          {
            ss << "#define ZS_EVENTING_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
            for (int paramIndex = 1; paramIndex <= index; ++paramIndex)
            {
              ss << ", xType" << string(paramIndex) << ", xName" << string(paramIndex) << ", xValue" << string(paramIndex);
            }
            ss << ")\n";
          }

          ss << "\n";

          for (int index = 0; index <= 60; ++index)
          {
            ss << "#define ZS_EVENTING_COMPACT_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
            for (int paramIndex = 1; paramIndex <= index; ++paramIndex)
            {
              ss << ", xType" << string(paramIndex) << "AndName" << string(paramIndex) << ", xValue" << string(paramIndex);
            }
            ss << ")\n";
          }

          ss <<
            "#endif /* ZS_EVENTING_REGISTER */\n"
            "\n"
            "#else /* ndef _WIN32 */\n"
            "\n";

          ss << "#include \"" << Helper::fileNameAfterPath(outputNameWindowsETW) << "\"\n";

          ss <<
            "#include <stdint.h>\n"
            "\n"
            "#ifndef ZS_EVENTING_REGISTER\n"
            "#define ZS_EVENTING_REGISTER(xProviderName)                                     ZS_INTERNAL_REGISTER_EVENTING_##xProviderName()\n"
            "#define ZS_EVENTING_UNREGISTER(xProviderName)                                   ZS_INTERNAL_UNREGISTER_EVENTING_##xProviderName()\n"
            "\n";

          for (int index = 0; index <= 38; ++index)
          {
            ss << "#define ZS_EVENTING_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
            for (int paramIndex = 1; paramIndex <= index; ++paramIndex)
            {
              ss << ", xType" << string(paramIndex) << ", xName" << string(paramIndex) << ", xValue" << string(paramIndex);
            }
            ss << ") \\\n";
            ss << "                                                                                ZS_INTERNAL_EVENTING_EVENT_##xSymbol(xSubsystem";
            for (int paramIndex = 1; paramIndex <= index; ++paramIndex)
            {
              ss << ", xValue" << string(paramIndex);
            }
            ss << ")\n\n";
          }

          ss << "\n";

          for (int index = 0; index <= 38; ++index)
          {
            ss << "#define ZS_EVENTING_COMPACT_" << string(index) << "(xSubsystem, xSeverity, xLevel, xSymbol, xChannelID, xTaskID, xOpCode";
            for (int paramIndex = 1; paramIndex <= index; ++paramIndex)
            {
              ss << ", xType" << string(paramIndex) << "AndName" << string(paramIndex) << ", xValue" << string(paramIndex);
            }
            ss << ") \\\n";
            ss << "                                                                                ZS_INTERNAL_EVENTING_EVENT_##xSymbol(xSubsystem";
            for (int paramIndex = 1; paramIndex <= index; ++paramIndex)
            {
              ss << ", xValue" << string(paramIndex);
            }
            ss << ")\n\n";
          }

          ss << "#endif /* ZS_EVENTING_REGISTER */\n\n";

          ss <<
            "#ifndef ZS_EVENTING_IS_LOGGING\n"
            "#define ZS_EVENTING_IS_LOGGING(xLevel)\n"
            "#define ZS_EVENTING_IS_SUBSYSTEM_LOGGING(xSubsystem, xLevel)\n"
            "#define ZS_EVENTING_GET_CURRENT_SUBSYSTEM_NAME() ((const char *)NULL)\n"
            "#define ZS_EVENTING_GET_SUBSYSTEM_NAME(xSubsystem) #xSubsystem\n"
            "#endif /*ZS_EVENTING_IS_LOGGING */\n\n";

          ss << "#define ZS_INTERNAL_REGISTER_EVENTING_" << provider->mName << "() EventRegister" << provider->mName << "()\n";
          ss << "#define ZS_INTERNAL_UNREGISTER_EVENTING_" << provider->mName << "() EventUnregister" << provider->mName << "()\n\n";

          for (auto iter = provider->mEvents.begin(); iter != provider->mEvents.end(); ++iter)
          {
            auto event = (*iter).second;

            {
              ss << "#define ZS_INTERNAL_EVENTING_EVENT_" << event->mName << "(xSubsystem";

              if (!event->mDataTemplate) goto declaration_done;
              if (event->mDataTemplate->mDataTypes.size() < 1) goto declaration_done;

              for (size_t loop = 1; loop <= event->mDataTemplate->mDataTypes.size(); ++loop)
              {
                ss << ", xValue" << string(loop);
              }
            }

          declaration_done:
            {
              ss << ") ";
            }

            {
              String getCurrentSubsystemStr;
              if ("x" == event->mSubsystem) {
                ss << "ZS_EVENTING_IS_LOGGING(" << Log::toString(event->mLevel) << ") { ";
                getCurrentSubsystemStr = "ZS_EVENTING_GET_CURRENT_SUBSYSTEM_NAME()";
              } else {
                ss << "ZS_EVENTING_IS_SUBSYSTEM_LOGGING(xSubsystem, " << Log::toString(event->mLevel) << ") { ";
                getCurrentSubsystemStr = "ZS_EVENTING_GET_SUBSYSTEM_NAME(xSubsystem)";
              }

              ss << "EventWrite" << event->mName << "(" << getCurrentSubsystemStr << ", __func__, __LINE__";

              size_t index = 1;
              bool nextMustBeSize = false;
              if (event->mDataTemplate) {
                for (auto iterDataType = event->mDataTemplate->mDataTypes.begin(); iterDataType != event->mDataTemplate->mDataTypes.end(); ++iterDataType, ++index) {
                  auto dataType = (*iterDataType);
                  switch (IEventingTypes::getBaseType(dataType->mType))
                  {
                    case IEventingTypes::BaseType_Boolean:  goto output_next;
                    case IEventingTypes::BaseType_Integer: {
                      if (IEventingTypes::PredefinedTypedef_size == dataType->mType) {
                        nextMustBeSize = false;
                        goto skip_next;
                      }
                      goto output_next;
                    }
                    case IEventingTypes::BaseType_Float:    goto output_next;
                    case IEventingTypes::BaseType_Pointer:  goto output_next;
                    case IEventingTypes::BaseType_Binary: {
                      ss << ", static_cast<size_t>(xValue" << (index+1) << ")";
                      ss << ", reinterpret_cast<const BYTE *>(xValue" << index << ")";
                      nextMustBeSize = true;
                      goto skip_next;
                    }
                    case IEventingTypes::BaseType_String:   goto output_next;
                  }

                output_next:
                  {
                    if (nextMustBeSize) {
                      ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, String("Binary data missing size"));
                    }

                    switch (dataType->mType)
                    {
                      case IEventingTypes::PredefinedTypedef_bool: {
                        ss << ", (bool)(xValue" << index << ")";
                        break;
                      }

                      case IEventingTypes::PredefinedTypedef_uchar:
                      case IEventingTypes::PredefinedTypedef_ushort:
                      case IEventingTypes::PredefinedTypedef_uint:
                      case IEventingTypes::PredefinedTypedef_ulong:
                      case IEventingTypes::PredefinedTypedef_ulonglong:
                      case IEventingTypes::PredefinedTypedef_uint8:
                      case IEventingTypes::PredefinedTypedef_uint16:
                      case IEventingTypes::PredefinedTypedef_uint32:
                      case IEventingTypes::PredefinedTypedef_uint64:
                      case IEventingTypes::PredefinedTypedef_byte:
                      case IEventingTypes::PredefinedTypedef_word:
                      case IEventingTypes::PredefinedTypedef_dword:
                      case IEventingTypes::PredefinedTypedef_qword:
                      case IEventingTypes::PredefinedTypedef_size:
                      {
                        String typeStr = String("uint") + string(IEventingTypes::getMaxBytes(dataType->mType) * 8) + "_t";
                        ss << ", static_cast<" << typeStr << ">(xValue" << string(index) << ")";
                        break;
                      }

                      case IEventingTypes::PredefinedTypedef_char:
                      case IEventingTypes::PredefinedTypedef_schar:
                      case IEventingTypes::PredefinedTypedef_short:
                      case IEventingTypes::PredefinedTypedef_sshort:
                      case IEventingTypes::PredefinedTypedef_int:
                      case IEventingTypes::PredefinedTypedef_sint:
                      case IEventingTypes::PredefinedTypedef_long:
                      case IEventingTypes::PredefinedTypedef_slong:
                      case IEventingTypes::PredefinedTypedef_longlong:
                      case IEventingTypes::PredefinedTypedef_slonglong:
                      case IEventingTypes::PredefinedTypedef_int8:
                      case IEventingTypes::PredefinedTypedef_sint8:
                      case IEventingTypes::PredefinedTypedef_int16:
                      case IEventingTypes::PredefinedTypedef_sint16:
                      case IEventingTypes::PredefinedTypedef_int32:
                      case IEventingTypes::PredefinedTypedef_sint32:
                      case IEventingTypes::PredefinedTypedef_int64:
                      case IEventingTypes::PredefinedTypedef_sint64: {
                        String typeStr = String("int") + string(IEventingTypes::getMaxBytes(dataType->mType) * 8) + "_t";
                        ss << ", static_cast<" << typeStr << ">(xValue" << string(index) << ")";
                        break;
                      }

                      case IEventingTypes::PredefinedTypedef_float:
                      case IEventingTypes::PredefinedTypedef_double:
                      case IEventingTypes::PredefinedTypedef_ldouble:
                      case IEventingTypes::PredefinedTypedef_float32:
                      case IEventingTypes::PredefinedTypedef_float64: {
                        String typeStr = (IEventingTypes::getMaxBytes(dataType->mType) <= 4 ? "float" : "double");
                        ss << ", static_cast<" << typeStr << ">(xValue" << string(index) << ")";
                        break;
                      }

                      case IEventingTypes::PredefinedTypedef_void:
                      case IEventingTypes::PredefinedTypedef_pointer: {
                        ss << ", reinterpret_cast<const void *>(xValue" << string(index) << ")";
                        break;
                      }

                      case IEventingTypes::PredefinedTypedef_binary: break;

                      case IEventingTypes::PredefinedTypedef_string: {
                        ss << ", (xValue" << string(index) << ")";
                        break;
                      }
                      case IEventingTypes::PredefinedTypedef_astring: {
                        ss << ", (xValue" << string(index) << ")";
                        break;
                      }
                      case IEventingTypes::PredefinedTypedef_wstring: {
                        ss << ", (xValue" << string(index) << ")";
                        break;
                      }
                    }
                    continue;
                  }
                skip_next:
                  {
                  }
                }
              }
              if (nextMustBeSize) {
                ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, String("Binary data missing size"));
              }

              ss << "); ";

              ss << "}\n";
            }
          }

          ss <<
            "\n"
            "#endif /* ndef _WIN32 */\n"
            "\n"
            "#endif /* ZS_EVENTING_NOOP */\n";

          return UseEventingHelper::convertToBuffer(ss.str());
        }

        //---------------------------------------------------------------------
        void EventingCompiler::writeXML(const String &outputName, const DocumentPtr &doc) const throw (Failure)
        {
          if (!doc) return;
          try {
            auto output = UseEventingHelper::writeXML(*doc);
            UseEventingHelper::saveFile(outputName, *output);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save XML file \"" + outputName + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
        }

        //---------------------------------------------------------------------
        void EventingCompiler::writeJSON(const String &outputName, const DocumentPtr &doc) const throw (Failure)
        {
          if (!doc) return;
          try {
            auto output = UseEventingHelper::writeJSON(*doc);
            UseEventingHelper::saveFile(outputName, *output);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save JSON file \"" + outputName + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
        }

        //---------------------------------------------------------------------
        void EventingCompiler::writeBinary(const String &outputName, const SecureByteBlockPtr &buffer) const throw (Failure)
        {
          if ((!buffer) ||
              (0 == buffer->SizeInBytes())) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save file \"" + outputName + "\": file is empty");
          }
          try {
            UseEventingHelper::saveFile(outputName, *buffer);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to save file \"" + outputName + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
        }

      } // namespace internal


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICompilerTypes
      #pragma mark

      //-----------------------------------------------------------------------
      ICompilerTypes::Modes ICompilerTypes::toMode(const char *value) throw (InvalidArgument)
      {
        String str(value);
        for (ICompilerTypes::Modes index = ICompilerTypes::Mode_First; index <= ICompilerTypes::Mode_Last; index = static_cast<ICompilerTypes::Modes>(static_cast<std::underlying_type<ICompilerTypes::Modes>::type>(index) + 1)) {
          if (0 == str.compareNoCase(ICompilerTypes::toString(index))) return index;
        }
        
        ZS_THROW_INVALID_ARGUMENT(String("mode is not understood:") + value);
        return ICompilerTypes::Mode_First;
      }

      //-----------------------------------------------------------------------
      const char *ICompilerTypes::toString(Modes value)
      {
        switch (value)
        {
          case Mode_Eventing:         return "eventing";
          case Mode_IDL:              return "idl";
        }
        return "unknown";
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICompiler
      #pragma mark

      //-----------------------------------------------------------------------
      void ICompiler::installTarget(IIDLCompilerTargetPtr target)
      {
        tool::internal::installIDLTarget(target);
      }

      //-----------------------------------------------------------------------
      ICompilerPtr ICompiler::create(const Config &config)
      {
        switch (config.mMode) {
          case ICompilerTypes::Mode_Eventing:   break;
          case ICompilerTypes::Mode_IDL:        return internal::IDLCompiler::create(config);
        }
        return internal::EventingCompiler::create(config);
      }

    } // namespace tool
  } // namespace eventing
} // namespace zsLib
