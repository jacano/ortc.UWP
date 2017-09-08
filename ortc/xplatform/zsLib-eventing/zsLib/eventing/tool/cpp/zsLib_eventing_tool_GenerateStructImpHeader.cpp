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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructImplHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateTypesHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateHelper.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Helper.h>

#include <zsLib/eventing/tool/OutputStream.h>

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

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark GenerateStructImplHeader
        #pragma mark

        //-------------------------------------------------------------------
        GenerateStructImplHeader::GenerateStructImplHeader() : IDLCompiler(Noop{})
        {
        }

        //-------------------------------------------------------------------
        GenerateStructImplHeaderPtr GenerateStructImplHeader::create()
        {
          return make_shared<GenerateStructImplHeader>();
        }

        //---------------------------------------------------------------------
        SecureByteBlockPtr GenerateStructImplHeader::generateTypesHeader(ProjectPtr project) throw (Failure)
        {
          std::stringstream ss;

          if (!project) return SecureByteBlockPtr();
          if (!project->mGlobal) return SecureByteBlockPtr();

          ss << "// " ZS_EVENTING_GENERATED_BY "\n\n";
          ss << "#pragma once\n\n";
          ss << "#include \"generated/types.h\"\n\n";
          ss << "namespace wrapper {\n";
          ss << "  namespace impl {\n\n";

          GenerateTypesHeader::processTypesNamespace(ss, String("  "), project->mGlobal, false);

          ss << "  } // namespace impl\n";
          ss << "} // namespace wrapper\n\n";

          return UseHelper::convertToBuffer(ss.str());
        }

        //-------------------------------------------------------------------
        String GenerateStructImplHeader::getStructFileName(StructPtr structObj)
        {
          return String("impl_") + GenerateStructHeader::getStructFileName(structObj);
        }

        //-------------------------------------------------------------------
        String GenerateStructImplHeader::getStructInitName(StructPtr structObj)
        {
          return GenerateStructHeader::getStructInitName(structObj);
        }

        //---------------------------------------------------------------------
        String GenerateStructImplHeader::makeOptional(bool isOptional, const String &value)
        {
          return GenerateStructHeader::makeOptional(isOptional, value);
        }

        //---------------------------------------------------------------------
        String GenerateStructImplHeader::getWrapperTypeString(bool isOptional, TypePtr type)
        {
          return GenerateStructHeader::getWrapperTypeString(isOptional, type);
        }

        //---------------------------------------------------------------------
        void GenerateStructImplHeader::outputMethods(
                                                     StructPtr derivedStructObj,
                                                     StructPtr structObj,
                                                     String indentStr,
                                                     std::stringstream &ss,
                                                     bool needsDefaultConstructor,
                                                     bool &foundEventHandler
                                                     )
        {
          if (!structObj) return;

          // output relationships
          {
            for (auto iterRelations = structObj->mIsARelationships.begin(); iterRelations != structObj->mIsARelationships.end(); ++iterRelations)
            {
              auto relatedObj = (*iterRelations).second;
              outputMethods(derivedStructObj, relatedObj->toStruct(), indentStr, ss, needsDefaultConstructor, foundEventHandler);
            }
          }

          bool foundCtor = false;
          bool firstMethod = true;
          for (auto iterMethods = structObj->mMethods.begin(); iterMethods != structObj->mMethods.end(); ++iterMethods)
          {
            auto methodObj = (*iterMethods);

            if (methodObj->hasModifier(Modifier_Method_Delete)) continue;
            if (methodObj->hasModifier(Modifier_Static)) continue;

            bool isCtor = methodObj->hasModifier(Modifier_Method_Ctor);
            
            if (derivedStructObj != structObj) {
              if (isCtor) continue;
            }

            if (methodObj->hasModifier(Modifier_Method_EventHandler)) {
              foundEventHandler = true;
              continue;
            }

            if (firstMethod) {
              ss << "\n";
              ss << indentStr << "// methods " << structObj->getMappingName() << "\n";
            }
            firstMethod = false;

            ss << indentStr;
            if (!isCtor) {
              ss << "virtual ";
              ss << getWrapperTypeString(methodObj->hasModifier(Modifier_Optional), methodObj->mResult);
              ss << " " << methodObj->mName;
            } else {
              foundCtor = true;
              ss << "virtual void wrapper_init_" << getStructInitName(structObj);
            }

            ss << "(";

            if (methodObj->mArguments.size() > 1) ss << "\n" << indentStr << "  ";

            bool firstArgument = true;
            for (auto iterParams = methodObj->mArguments.begin(); iterParams != methodObj->mArguments.end(); ++iterParams)
            {
              auto argument = (*iterParams);
              if (!firstArgument) {
                ss << ",\n";
                ss << indentStr << "  ";
              }
              firstArgument = false;

              String typeStr = getWrapperTypeString(argument->hasModifier(Modifier_Optional), argument->mType);
              ss << typeStr << " " << argument->mName;
            }
            if (methodObj->mArguments.size() > 1) ss << "\n" << indentStr << "  ";
            ss << ") override;\n";
          }

          if ((derivedStructObj == structObj) && (needsDefaultConstructor)) {
            ss << indentStr << "virtual void wrapper_init_" << getStructInitName(structObj) << "() override;\n";
          }

          bool isDictionary = structObj->hasModifier(Modifier_Struct_Dictionary);
          bool firstProperty = true;
          for (auto iterProperties = structObj->mProperties.begin(); iterProperties != structObj->mProperties.end(); ++iterProperties)
          {
            auto propertyObj = (*iterProperties);

            if (propertyObj->hasModifier(Modifier_Static)) continue;

            bool hasGetter = propertyObj->hasModifier(Modifier_Property_Getter);
            bool hasSetter = propertyObj->hasModifier(Modifier_Property_Setter);

            String typeStr = getWrapperTypeString(propertyObj->hasModifier(Modifier_Optional), propertyObj->mType);

            if (!isDictionary) {
              if (!((hasGetter) || (hasSetter))) {
                hasGetter = hasSetter = true;
              }
            }

            if ((!hasGetter) && (!hasSetter)) continue;

            if (firstProperty) {
              ss << "\n";
              ss << indentStr << "// properties " << structObj->getMappingName() << "\n";
            }
            firstProperty = false;

            if (hasGetter) {
              ss << indentStr << "virtual " << typeStr << " get_" << propertyObj->mName << "() override;\n";
            }
            if (hasSetter) {
              ss << indentStr << "virtual void set_" << propertyObj->mName << "(" << typeStr << " value) override;\n";
            }
          }
        }

        //---------------------------------------------------------------------
        void GenerateStructImplHeader::generateStructHeaderImpl(
                                                                StructPtr structObj,
                                                                String indentStr,
                                                                StringSet &includedHeaders,
                                                                std::stringstream &ss
                                                                )
        {
          if (!structObj) return;
          if (GenerateHelper::isBuiltInType(structObj)) return;
          if (structObj->mGenerics.size() > 0) return;

          ss << "\n";
          ss << indentStr << "struct " << structObj->mName << " : public wrapper" << structObj->getPathName() << "\n";
          ss << indentStr << "{\n";

          String currentIdentStr = indentStr;
          indentStr += "  ";

          if (!structObj->hasModifier(Modifier_Static)) {
            ss << indentStr << structObj->mName << "WeakPtr thisWeak_;\n\n";
            ss << indentStr << structObj->mName << "();\n";
          }
          ss << indentStr << "virtual ~" << structObj->mName << "();\n";

          for (auto iterStructs = structObj->mStructs.begin(); iterStructs != structObj->mStructs.end(); ++iterStructs)
          {
            auto subStructObj = (*iterStructs).second;
            generateStructHeaderImpl(subStructObj, indentStr, includedHeaders, ss);
          }

          bool foundEventHandler = false;
          outputMethods(structObj, structObj, indentStr, ss, GenerateHelper::needsDefaultConstructor(structObj), foundEventHandler);

          if (foundEventHandler) {
            ss << "\n";
            ss << indentStr << "virtual void wrapper_onObserverCountChanged(size_t count) override;\n";
          }

          indentStr = currentIdentStr;
          ss << indentStr << "};\n";
        }

        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        #pragma mark
        #pragma mark GenerateStructImplHeader::IIDLCompilerTarget
        #pragma mark

        //-------------------------------------------------------------------
        String GenerateStructImplHeader::targetKeyword()
        {
          return String("implheader");
        }

        //-------------------------------------------------------------------
        String GenerateStructImplHeader::targetKeywordHelp()
        {
          return String("C++ implementation wrapper header template");
        }

        //-------------------------------------------------------------------
        void GenerateStructImplHeader::targetOutput(
                                                    const String &pathStr,
                                                    const ICompilerTypes::Config &config
                                                    ) throw (Failure)
        {
          typedef std::stack<NamespacePtr> NamespaceStack;
          typedef std::stack<String> StringList;

          const ProjectPtr &project = config.mProject;
          if (!project) return;
          if (!project->mGlobal) return;

          writeBinary(UseHelper::fixRelativeFilePath(pathStr, String("types.h")), generateTypesHeader(project));

          NamespaceStack namespaceStack;

          namespaceStack.push(project->mGlobal);

          while (namespaceStack.size() > 0)
          {
            auto namespaceObj = namespaceStack.top();
            namespaceStack.pop();
            if (!namespaceObj) continue;
            if (namespaceObj->hasModifier(Modifier_Special)) continue;

            for (auto iter = namespaceObj->mNamespaces.begin(); iter != namespaceObj->mNamespaces.end(); ++iter)
            {
              auto subNamespaceObj = (*iter).second;
              namespaceStack.push(subNamespaceObj);
            }

            for (auto iter = namespaceObj->mStructs.begin(); iter != namespaceObj->mStructs.end(); ++iter)
            {
              auto structObj = (*iter).second;
              if (GenerateHelper::isBuiltInType(structObj)) continue;
              if (structObj->mGenerics.size() > 0) continue;

              String filename = GenerateStructImplHeader::getStructFileName(structObj);

              String outputname = UseHelper::fixRelativeFilePath(pathStr, filename);

              std::stringstream ss;
              std::stringstream includeSS;
              std::stringstream structSS;
              StringList endStrings;

              ss << "// " ZS_EVENTING_GENERATED_BY "\n\n";
              ss << "#pragma once\n\n";
              ss << "#include \"types.h\"\n";

              includeSS << "#include \"generated/" << GenerateStructHeader::getStructFileName(structObj) << "\"\n\n";

              structSS << "namespace wrapper {\n";
              structSS << "  namespace impl {\n";

              NamespaceStack parentStack;
              auto parent = structObj->getParent();
              while (parent) {
                auto parentNamespace = parent->toNamespace();
                if (parentNamespace) {
                  parentStack.push(parentNamespace);
                }
                parent = parent->getParent();
              }

              String indentStr = "    ";

              while (parentStack.size() > 0)
              {
                auto parentNamespace = parentStack.top();
                parentStack.pop();

                if (parentNamespace->mName.hasData()) {
                  structSS << indentStr << "namespace " << parentNamespace->mName << " {\n";
                  {
                    std::stringstream endSS;
                    endSS << indentStr << "} // " << parentNamespace->mName << "\n";
                    endStrings.push(endSS.str());
                  }

                  indentStr += "  ";
                }
              }

              {
                GenerateStructImplHeader::StringSet processedHeaders;
                GenerateStructImplHeader::generateStructHeaderImpl(structObj, indentStr, processedHeaders, structSS);
              }

              ss << includeSS.str();
              ss << "\n";
              ss << structSS.str();
              ss << "\n";
              while (endStrings.size() > 0) {
                ss << endStrings.top();
                endStrings.pop();
              }
              ss << "  } // namespace impl\n";
              ss << "} // namespace wrapper\n\n";

              writeBinary(outputname, UseHelper::convertToBuffer(ss.str()));
            }
          }
        }

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
