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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateTypesHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateHelper.h>
//#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Helper.h>
//
#include <zsLib/eventing/tool/OutputStream.h>
//
//#include <zsLib/eventing/IHelper.h>
//#include <zsLib/eventing/IHasher.h>
//#include <zsLib/eventing/IEventingTypes.h>
//
//#include <zsLib/Exception.h>
//#include <zsLib/Numeric.h>
//
#include <sstream>
//#include <list>
//#include <set>
//#include <cctype>

namespace zsLib { namespace eventing { namespace tool { ZS_DECLARE_SUBSYSTEM(zsLib_eventing_tool) } } }

namespace zsLib
{
  namespace eventing
  {
    ZS_DECLARE_TYPEDEF_PTR(IIDLTypes::Project, Project);

    namespace tool
    {
      ZS_DECLARE_TYPEDEF_PTR(eventing::IHelper, UseHelper);
      ZS_DECLARE_TYPEDEF_PTR(eventing::IHasher, UseHasher);
      typedef std::set<String> HashSet;

      namespace internal
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark GenerateTypesHeader
        #pragma mark

        //-------------------------------------------------------------------
        void GenerateTypesHeader::processTypesNamespace(
                                                        std::stringstream &ss,
                                                        const String &inIndentStr,
                                                        NamespacePtr namespaceObj,
                                                        bool outputEnums
                                                        )
        {
          if (!namespaceObj) return;
          if (namespaceObj->hasModifier(Modifier_Special)) return;

          int parentCount = 0;
          String initialIndentStr;
          String indentStr(inIndentStr);

          {
            auto parent = namespaceObj->getParent();
            while (parent)
            {
              auto checkObj = parent->toNamespace();
              if (!checkObj) break;
              ++parentCount;
              indentStr += "  ";
              parent = parent->getParent();
            }
          }

          initialIndentStr = indentStr;

          if (namespaceObj->mName.hasData()) {
            ss << indentStr << "namespace " << namespaceObj->mName << " {\n";
          }

          indentStr += "  ";

          bool firstNamespace {true};
          for (auto iter = namespaceObj->mNamespaces.begin(); iter != namespaceObj->mNamespaces.end(); ++iter)
          {
            auto subNamespaceObj = (*iter).second;
            if (subNamespaceObj->hasModifier(Modifier_Special)) continue;

            if (!firstNamespace) {
              ss << "\n";
            }
            firstNamespace = false;
            processTypesNamespace(ss, inIndentStr, subNamespaceObj, outputEnums);
          }

          bool firstEnum {true};
          if (outputEnums) {
            for (auto iter = namespaceObj->mEnums.begin(); iter != namespaceObj->mEnums.end(); ++iter)
            {
              auto enumObj = (*iter).second;
              GenerateHelper::insertFirst(ss, firstEnum);
              ss << indentStr << "enum " << enumObj->mName << " {\n";
              for (auto iterValue = enumObj->mValues.begin(); iterValue != enumObj->mValues.end(); ++iterValue)
              {
                auto valueObj = (*iterValue);
                ss << indentStr << "  " << enumObj->mName << "_" << valueObj->mName;
                if (valueObj->mValue.hasData()) {
                  ss << " = " << valueObj->mValue;
                }
                ss << ",\n";
              }
              ss << indentStr << "};\n";
            }
            GenerateHelper::insertLast(ss, firstEnum);
          }

          bool firstStruct {firstEnum};
          for (auto iter = namespaceObj->mStructs.begin(); iter != namespaceObj->mStructs.end(); ++iter)
          {
            auto structObj = (*iter).second;
            if (GenerateHelper::isBuiltInType(structObj)) continue;
            if (structObj->mGenerics.size() > 0) continue;
            GenerateHelper::insertFirst(ss, firstStruct);
            ss << indentStr << "ZS_DECLARE_STRUCT_PTR(" << structObj->mName << ");\n";
          }
          if (namespaceObj->mStructs.size() > 0) {
            GenerateHelper::insertLast(ss, firstStruct);
          }

          indentStr = initialIndentStr;

          if (namespaceObj->mName.hasData()) {
            ss << indentStr << "} // namespace " << namespaceObj->mName << "\n";
          }
        }
      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
