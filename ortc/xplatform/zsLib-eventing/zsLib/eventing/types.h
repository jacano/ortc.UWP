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

#include <zsLib/types.h>
#include <zsLib/String.h>
#include <zsLib/Exception.h>
#include <zsLib/XML.h>
#include <zsLib/Proxy.h>

#include <cryptopp/allocatorwithnul.h>

#include <list>

namespace zsLib
{
  namespace eventing
  {
    typedef CryptoPP::SecBlock<byte, CryptoPP::AllocatorWithNul<byte> > SecureByteBlockWithNulAllocator;
    ZS_DECLARE_TYPEDEF_PTR(SecureByteBlockWithNulAllocator, SecureByteBlock);

    ZS_DECLARE_TYPEDEF_PTR(std::list<String>, StringList);

    ZS_DECLARE_CUSTOM_EXCEPTION(InvalidContent);
    ZS_DECLARE_CUSTOM_EXCEPTION_WITH_PROPERTIES_1(InvalidContentWithLine, ULONG, lineNumber);
    ZS_DECLARE_CUSTOM_EXCEPTION_WITH_PROPERTIES_1(Failure, int, result);
    ZS_DECLARE_CUSTOM_EXCEPTION_WITH_PROPERTIES_2(FailureWithLine, int, result, ULONG, lineNumber);
    ZS_DECLARE_CUSTOM_EXCEPTION_WITH_PROPERTIES_1(StdError, int, result);

    typedef zsLib::Exceptions::InvalidArgument InvalidArgument;


    using namespace zsLib::XML;

    ZS_DECLARE_INTERACTION_PTR(IHelper);
    ZS_DECLARE_INTERACTION_PTR(IHasher);
    ZS_DECLARE_INTERACTION_PTR(IHasherAlgorithm);
    ZS_DECLARE_INTERACTION_PTR(IRemoteEventing);

    ZS_DECLARE_INTERACTION_PROXY(IRemoteEventingDelegate);
  }
}
