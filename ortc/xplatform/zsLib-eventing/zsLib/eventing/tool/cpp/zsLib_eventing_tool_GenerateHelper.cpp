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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateHelper.h>
//#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Helper.h>
//
#include <zsLib/eventing/tool/OutputStream.h>
//
#include <zsLib/eventing/IHelper.h>
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
        #pragma mark GenerateHelper
        #pragma mark


        //---------------------------------------------------------------------
        String GenerateHelper::getDashedComment(const String &indent)
        {
          std::stringstream ss;
          ss << indent << "//";
          size_t length = 2 + indent.length();
          if (length < 80) {
            for (size_t index = 0; index < (80 - length); ++index) {
              ss << "-";
            }
          }
          ss << "\n";
          return ss.str();
        }

        //---------------------------------------------------------------------
        String GenerateHelper::getDocumentation(
                                                const String &linePrefix,
                                                ContextPtr context,
                                                size_t maxLineLength
                                                )
        {
          if (!context) return String();
          if (!context->mDocumentation) return String();

          std::stringstream ss;

          ElementPtr childEl = context->mDocumentation->getFirstChildElement();
          while (childEl)
          {
            auto xmlStr = UseHelper::toString(childEl, false);

            if (xmlStr.hasData()) {
              if (String::npos != xmlStr.find("<code>")) {
                UseHelper::SplitMap split;
                UseHelper::split(xmlStr, split, "\n");
                for (auto iter = split.begin(); iter != split.end(); ++iter)
                {
                  ss << linePrefix << (*iter).second << "\n";
                }
                return ss.str();
              }

              auto startElPos = xmlStr.find('>');
              auto endElPos = xmlStr.rfind('<');
              if ((String::npos != startElPos) &&
                  (String::npos != endElPos) &&
                  (endElPos > startElPos)) {
                String openElStr = xmlStr.substr(0, startElPos + 1);
                String endElStr = xmlStr.substr(endElPos);

                xmlStr = xmlStr.substr(startElPos + 1, xmlStr.length() - openElStr.length() - endElStr.length());
                xmlStr.replaceAll("\t", " ");
                xmlStr.replaceAll("\n", " ");
                xmlStr.replaceAll("\r", " ");
                xmlStr.replaceAll("\v", " ");

                ss << linePrefix << openElStr << "\n";

                UseHelper::SplitMap split;
                UseHelper::split(xmlStr, split, " ");
                UseHelper::splitTrim(split);
                UseHelper::splitPruneEmpty(split);

                size_t totalWordsOutput = 0;
                String currentStr;

                for (auto iter = split.begin(); iter != split.end(); ++iter)
                {
                  auto wordStr = (*iter).second;

                  String sep(totalWordsOutput > 0 ? String(" ") : String());

                  if (linePrefix.length() + currentStr.length() + sep.length() + wordStr.length() > maxLineLength) {
                    if (0 == totalWordsOutput) {
                      ss << linePrefix << wordStr << "\n";
                      continue;
                    }
                    ss << linePrefix << currentStr << "\n";
                    totalWordsOutput = 0;
                    currentStr = String();
                    sep = String();
                  }

                  currentStr += sep;
                  currentStr += wordStr;
                  ++totalWordsOutput;
                }
                if (totalWordsOutput > 0) {
                  ss << linePrefix << currentStr << "\n";
                }

                ss << linePrefix << endElStr << "\n";
              } else {
                ss << linePrefix << xmlStr << "\n";
              }
            }
            childEl = childEl->getNextSiblingElement();
          }

          return ss.str();
        }

        //---------------------------------------------------------------------
        void GenerateHelper::insertFirst(
                                         std::stringstream &ss,
                                         bool &first
                                         )
        {
          if (!first) return;
          first = false;
          ss << "\n";
        }

        //---------------------------------------------------------------------
        void GenerateHelper::insertLast(
                                        std::stringstream &ss,
                                        bool &first
                                        )
        {
          if (first) return;
          ss << "\n";
        }

        //---------------------------------------------------------------------
        bool GenerateHelper::isBuiltInType(TypePtr type)
        {
          if (!type) return false;

          type = type->getOriginalType();
          if (!type) return false;

          {
            auto basicType = type->toBasicType();
            if (basicType) return true;
          }

          {
            auto structObj = type->toStruct();
            if (structObj) {
              if (!structObj->hasModifier(Modifier_Special)) return false;
              String specialName = structObj->getPathName();
              if ("::zs::Any" == specialName) return true;
              if ("::zs::Promise" == specialName) return true;
              if ("::zs::PromiseWith" == specialName) return true;
              if ("::zs::PromiseRejectionReason" == specialName) return true;
              if ("::zs::exceptions::Exception" == specialName) return true;
              if ("::zs::exceptions::InvalidArgument" == specialName) return true;
              if ("::zs::exceptions::BadState" == specialName) return true;
              if ("::zs::exceptions::NotImplemented" == specialName) return true;
              if ("::zs::exceptions::NotSupported" == specialName) return true;
              if ("::zs::exceptions::UnexpectedError" == specialName) return true;
              if ("::zs::Time" == specialName) return true;
              if ("::zs::Milliseconds" == specialName) return true;
              if ("::zs::Microseconds" == specialName) return true;
              if ("::zs::Nanoseconds" == specialName) return true;
              if ("::zs::Seconds" == specialName) return true;
              if ("::zs::Minutes" == specialName) return true;
              if ("::zs::Hours" == specialName) return true;
              if ("::std::set" == specialName) return true;
              if ("::std::list" == specialName) return true;
              if ("::std::map" == specialName) return true;
              return false;
            }
          }

          {
            auto templatedStruct = type->toTemplatedStructType();
            if (templatedStruct) {
              auto parent = templatedStruct->getParent();
              if (parent) return isBuiltInType(parent->toStruct());
            }
          }
          return false;
        }

        //-------------------------------------------------------------------
        bool GenerateHelper::hasOnlyStaticMethods(StructPtr structObj)
        {
          if (!structObj) return true;

          for (auto iter = structObj->mIsARelationships.begin(); iter != structObj->mIsARelationships.end(); ++iter) {
            auto relatedObj = (*iter).second;
            if (!relatedObj) continue;

            auto relatedStruct = relatedObj->toStruct();
            if (!relatedStruct) continue;

            bool only = hasOnlyStaticMethods(relatedStruct);
            if (!only) return false;
          }

          for (auto iter = structObj->mMethods.begin(); iter != structObj->mMethods.end(); ++iter) {
            auto method = (*iter);
            if (method->hasModifier(Modifier_Static)) continue;
            return false;
          }
          for (auto iter = structObj->mProperties.begin(); iter != structObj->mProperties.end(); ++iter) {
            auto method = (*iter);
            if (method->hasModifier(Modifier_Static)) continue;
            return false;
          }
          return true;
        }

        //-------------------------------------------------------------------
        bool GenerateHelper::needsDefaultConstructor(StructPtr structObj)
        {
          if (!structObj) return false;

          if (structObj->hasModifier(Modifier_Static)) return false;

          for (auto iter = structObj->mMethods.begin(); iter != structObj->mMethods.end(); ++iter)
          {
            auto method = (*iter);
            if (method->hasModifier(Modifier_Method_Ctor)) return false;
          }

          return true;
        }

        //-------------------------------------------------------------------
        bool GenerateHelper::needsDefaultConstructor(TemplatedStructTypePtr templateObj)
        {
          if (!templateObj) return false;

          auto parent = templateObj->getParent();
          if (!parent) return false;
          return needsDefaultConstructor(parent->toStruct());
        }

        //-------------------------------------------------------------------
        String GenerateHelper::getBasicTypeString(BasicTypePtr type)
        {
          if (!type) return String();
          switch (type->mBaseType)
          {
          case PredefinedTypedef_void:        return "void";
          case PredefinedTypedef_bool:        return "bool";
          case PredefinedTypedef_uchar:       return "unsigned char";
          case PredefinedTypedef_char:        return "char";
          case PredefinedTypedef_schar:       return "signed char";
          case PredefinedTypedef_ushort:      return "unsigned short";
          case PredefinedTypedef_short:       return "short";
          case PredefinedTypedef_sshort:      return "signed short";
          case PredefinedTypedef_uint:        return "unsigned int";
          case PredefinedTypedef_int:         return "int";
          case PredefinedTypedef_sint:        return "signed int";
          case PredefinedTypedef_ulong:       return "unsigned long";
          case PredefinedTypedef_long:        return "long";
          case PredefinedTypedef_slong:       return "signed long";
          case PredefinedTypedef_ulonglong:   return "unsigned long long";
          case PredefinedTypedef_longlong:    return "long long";
          case PredefinedTypedef_slonglong:   return "signed long long";
          case PredefinedTypedef_uint8:       return "uint8_t";
          case PredefinedTypedef_int8:        return "int8_t";
          case PredefinedTypedef_sint8:       return "int8_t";
          case PredefinedTypedef_uint16:      return "uint16_t";
          case PredefinedTypedef_int16:       return "int16_t";
          case PredefinedTypedef_sint16:      return "int16_t";
          case PredefinedTypedef_uint32:      return "uint32_t";
          case PredefinedTypedef_int32:       return "int32_t";
          case PredefinedTypedef_sint32:      return "int32_t";
          case PredefinedTypedef_uint64:      return "uint64_t";
          case PredefinedTypedef_int64:       return "int64_t";
          case PredefinedTypedef_sint64:      return "int64_t";

          case PredefinedTypedef_byte:        return "uint8_t";
          case PredefinedTypedef_word:        return "uint16_t";
          case PredefinedTypedef_dword:       return "uint32_t";
          case PredefinedTypedef_qword:       return "uint64_t";

          case PredefinedTypedef_float:       return "float";
          case PredefinedTypedef_double:      return "double";
          case PredefinedTypedef_ldouble:     return "long double";
          case PredefinedTypedef_float32:     return "float";
          case PredefinedTypedef_float64:     return "double";

          case PredefinedTypedef_pointer:     return "uint64_t";

          case PredefinedTypedef_binary:      return "SecureByteBlockPtr";
          case PredefinedTypedef_size:        return "uint64_t";

          case PredefinedTypedef_string:      return "String";
          case PredefinedTypedef_astring:     return "String";
          case PredefinedTypedef_wstring:     return "::std::wstring";
          }
          return String();
        }

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
