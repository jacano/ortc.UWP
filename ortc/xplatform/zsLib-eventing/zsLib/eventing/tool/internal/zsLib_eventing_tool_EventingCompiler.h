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

#include <zsLib/eventing/tool/ICompiler.h>

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
        #pragma mark EventingCompiler
        #pragma mark

        class EventingCompiler : public ICompiler
        {
          struct make_private {};

        public:
          //-------------------------------------------------------------------
          EventingCompiler(
                           const make_private &,
                           const Config &config
                           );
          ~EventingCompiler();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark EventingCompiler => ICompiler
          #pragma mark

          static EventingCompilerPtr create(const Config &config);

          virtual void process() throw (Failure, FailureWithLine);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark EventingCompiler => (internal)
          #pragma mark

          void outputMacros();
          void read() throw (Failure, FailureWithLine);
          void prepareIndex() throw (Failure);
          void validate() throw (Failure);
          DocumentPtr generateManifest(const String &resourcePostFix) const throw (Failure);
          DocumentPtr generateWprp() const throw (Failure);
          DocumentPtr generateJsonMan() const throw (Failure);
          SecureByteBlockPtr generateXPlatformEventsHeader(
                                                           const String &outputNameXPlatform,
                                                           const String &outputNameWindows
                                                           ) const throw (Failure);
          SecureByteBlockPtr generateWindowsEventsHeader(
                                                         const String &outputNameXPlatform,
                                                         const String &outputNameWindows,
                                                         const String &outputNameWindowsETW
                                                         ) const throw (Failure);

          void writeXML(const String &outputName, const DocumentPtr &doc) const throw (Failure);
          void writeJSON(const String &outputName, const DocumentPtr &doc) const throw (Failure);
          void writeBinary(const String &outputName, const SecureByteBlockPtr &buffer) const throw (Failure);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark EventingCompiler => (data)
          #pragma mark

          EventingCompilerWeakPtr mThisWeak;

          Config mConfig;
        };

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
