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

#include <zsLib/eventing/tool/types.h>
#include <zsLib/eventing/tool/ICompiler.h>

#include <zsLib/eventing/IEventingTypes.h>
#include <zsLib/eventing/IRemoteEventing.h>

#include <zsLib/IPAddress.h>

namespace zsLib
{
  namespace eventing
  {
    namespace tool
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICommandLineTypes
      #pragma mark

      interaction ICommandLineTypes
      {
        ZS_DECLARE_TYPEDEF_PTR(std::list<String>, StringList);
        ZS_DECLARE_CUSTOM_EXCEPTION(NoopException);

        enum Flags
        {
          Flag_First,

          Flag_None = Flag_First,

          Flag_Quiet,
          Flag_Config,
          Flag_Question,
          Flag_Help,
          Flag_HelpAlt,
          Flag_Source,
          Flag_OutputName,
          Flag_Author,
          Flag_IDL,
          Flag_Monitor,
          Flag_MonitorPort,
          Flag_MonitorIP,
          Flag_MonitorTimeout,
          Flag_MonitorJMAN,
          Flag_MonitorJSON,
          Flag_MonitorProvider,
          Flag_MonitorSecret,

          Flag_Last = Flag_MonitorSecret,
        };

        static Flags toFlag(const char *str);
        static const char *toString(Flags flag);
        
        struct MonitorInfo
        {
          bool mMonitor {};
          bool mQuietMode {};
          IPAddress mIPAddress;
          WORD mPort {IRemoteEventingTypes::Port_Default};
          Seconds mTimeout {};
          StringList mJMANFiles;
          bool mOutputJSON {};
          String mSecret;
          StringList mSubscribeProviders;
        };
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ICommandLine
      #pragma mark

      interaction ICommandLine : public ICommandLineTypes
      {
        static void outputHeader();
        static void outputHelp();

        static StringList toList(
                                 int inArgc,
                                 const char * const inArgv[]
                                 );
        static StringList toList(
                                 int inArgc,
                                 const wchar_t * const inArgv[]
                                 );

        static int performDefaultHandling(const StringList &arguments);

        static void prepare(
                            StringList arguments,
                            MonitorInfo &outMonitor,
                            ICompilerTypes::Config &outConfig,
                            bool &outDidOutputHelp
                            ) throw (InvalidArgument);

        static void validate(
                             MonitorInfo &monitor,
                             ICompilerTypes::Config &config,
                             bool didOutputHelp
                             ) throw (InvalidArgument, NoopException);
        static void process(
                            MonitorInfo &monitor,
                            ICompilerTypes::Config &config
                            ) throw (Failure);
        
        static void interrupt();
      };

    } // namespace tool
  } // namespace eventing
} // namespace zsLib
