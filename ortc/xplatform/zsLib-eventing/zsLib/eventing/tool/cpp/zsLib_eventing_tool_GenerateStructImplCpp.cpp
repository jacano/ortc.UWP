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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructImplCpp.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateHelper.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateTypesHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructImplHeader.h>
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
        #pragma mark GenerateStructImplCpp
        #pragma mark

        //-------------------------------------------------------------------
        GenerateStructImplCpp::GenerateStructImplCpp() : IDLCompiler(Noop{})
        {
        }

        //-------------------------------------------------------------------
        GenerateStructImplCppPtr GenerateStructImplCpp::create()
        {
          return make_shared<GenerateStructImplCpp>();
        }

        //-------------------------------------------------------------------
        void GenerateStructImplCpp::generateUsingTypes(
                                                       std::stringstream &ss,
                                                       const String &indentStr
                                                       )
        {
          return GenerateStructHeader::generateUsingTypes(ss, indentStr);
        }

        //-------------------------------------------------------------------
        String GenerateStructImplCpp::getStructFileName(StructPtr structObj)
        {
          String result = String("impl_") + GenerateStructHeader::getStructFileName(structObj);
          result = result.substr(0, result.length() - 2);
          result += ".cpp";
          return result;
        }

        //-------------------------------------------------------------------
        String GenerateStructImplCpp::getStructInitName(StructPtr structObj)
        {
          return GenerateStructHeader::getStructInitName(structObj);
        }

        //---------------------------------------------------------------------
        String GenerateStructImplCpp::makeOptional(bool isOptional, const String &value)
        {
          return GenerateStructHeader::makeOptional(isOptional, value);
        }

        //---------------------------------------------------------------------
        String GenerateStructImplCpp::getWrapperTypeString(bool isOptional, TypePtr type)
        {
          return GenerateStructHeader::getWrapperTypeString(isOptional, type);
        }

        //---------------------------------------------------------------------
        void GenerateStructImplCpp::outputMethods(
                                                  StructPtr derivedStructObj,
                                                  StructPtr structObj,
                                                  std::stringstream &ss,
                                                  bool &foundEventHandler
                                                  )
        {
          if (!structObj) return;

          String dashedLine = GenerateHelper::getDashedComment(String());

          // output relationships
          for (auto iterRelations = structObj->mIsARelationships.begin(); iterRelations != structObj->mIsARelationships.end(); ++iterRelations)
          {
            auto relatedObj = (*iterRelations).second;

            {
              auto relatedStructObj = relatedObj->toStruct();
              if (relatedStructObj) {
                outputMethods(derivedStructObj, relatedStructObj, ss, foundEventHandler);
              }
            }
          }

          bool foundCtor = false;
          for (auto iterMethods = structObj->mMethods.begin(); iterMethods != structObj->mMethods.end(); ++iterMethods)
          {
            auto methodObj = (*iterMethods);
            if (methodObj->hasModifier(Modifier_Method_Delete)) continue;

            bool isCtor = methodObj->hasModifier(Modifier_Method_Ctor);
            if (derivedStructObj != structObj) {
              if ((isCtor) || (methodObj->hasModifier(Modifier_Static))) continue;
            }

            if (methodObj->hasModifier(Modifier_Method_EventHandler)) {
              foundEventHandler = true;
              continue;
            }

            String wrapperResultType = getWrapperTypeString(methodObj->hasModifier(Modifier_Optional), methodObj->mResult);

            ss << dashedLine;
            if (!isCtor) {
              ss << getWrapperTypeString(methodObj->hasModifier(Modifier_Optional), methodObj->mResult);
              ss << " wrapper" << (methodObj->hasModifier(Modifier_Static) ? "" : "::impl") << derivedStructObj->getPathName() << "::" << methodObj->mName;
            } else {
              foundCtor = true;
              wrapperResultType = "void";
              ss << "void wrapper::impl" << derivedStructObj->getPathName() << "::wrapper_init_" << getStructInitName(derivedStructObj);
            }

            ss << "(";

            if (methodObj->mArguments.size() > 1) ss << "\n" << "  ";

            bool firstArgument = true;
            for (auto iterParams = methodObj->mArguments.begin(); iterParams != methodObj->mArguments.end(); ++iterParams)
            {
              auto argument = (*iterParams);
              if (!firstArgument) {
                ss << ",\n";
                ss << "  ";
              }
              firstArgument = false;

              String typeStr = getWrapperTypeString(argument->hasModifier(Modifier_Optional), argument->mType);
              ss << typeStr << " " << argument->mName;
            }
            if (methodObj->mArguments.size() > 1) ss << "\n" << "  ";
            ss << ")\n";
            ss << "{\n";
            if ("void" != wrapperResultType) {
              ss << "  " << wrapperResultType << " result {};\n";
              ss << "  return result;\n";
            }
            ss << "}\n\n";
          }

          bool isDictionary = structObj->hasModifier(Modifier_Struct_Dictionary);
          for (auto iterProperties = structObj->mProperties.begin(); iterProperties != structObj->mProperties.end(); ++iterProperties)
          {
            auto propertyObj = (*iterProperties);
            bool hasGetter = propertyObj->hasModifier(Modifier_Property_Getter);
            bool hasSetter = propertyObj->hasModifier(Modifier_Property_Setter);
            bool isStatic = propertyObj->hasModifier(Modifier_Static);

            String typeStr = getWrapperTypeString(propertyObj->hasModifier(Modifier_Optional), propertyObj->mType);

            if ((!isDictionary) || (isStatic)) {
              if (!((hasGetter) || (hasSetter))) {
                hasGetter = hasSetter = true;
              }
            }

            if ((!hasGetter) && (!hasSetter)) continue;

            if (hasGetter) {
              ss << dashedLine;
              ss << typeStr << " wrapper" << (isStatic ? "" : "::impl") << derivedStructObj->getPathName() << "::get_" << propertyObj->mName << "()\n";
              ss << "{\n";
              ss << "  " << typeStr << " result {};\n";
              ss << "  return result;\n";
              ss << "}\n\n";
            }
            if (hasSetter) {
              ss << dashedLine;
              ss << "void wrapper" << (isStatic ? "" : "::impl") << derivedStructObj->getPathName() << "::set_" << propertyObj->mName << "(" << typeStr << " value)\n";
              ss << "{\n";
              ss << "}\n\n";
            }
          }
        }

        //---------------------------------------------------------------------
        void GenerateStructImplCpp::generateStructCppImpl(
                                                          StructPtr structObj,
                                                          StringSet &includedHeaders,
                                                          std::stringstream &ss
                                                          )
        {
          if (!structObj) return;
          if (GenerateHelper::isBuiltInType(structObj)) return;
          if (structObj->mGenerics.size() > 0) return;

          bool needsDefaultConstructor = GenerateHelper::needsDefaultConstructor(structObj);

          ss << "\n";

          for (auto iterStructs = structObj->mStructs.begin(); iterStructs != structObj->mStructs.end(); ++iterStructs)
          {
            auto subStructObj = (*iterStructs).second;
            generateStructCppImpl(subStructObj, includedHeaders, ss);
          }

          String dashedLine = GenerateHelper::getDashedComment(String());

          if (!structObj->hasModifier(Modifier_Static)) {
            ss << dashedLine;
            ss << "wrapper::impl" << structObj->getPathName() << "::" << structObj->mName << "()\n";
            ss << "{\n";
            ss << "}\n";
            ss << "\n";

            ss << dashedLine;
            ss << "wrapper" << structObj->getPathName() << "Ptr " << "wrapper" << structObj->getPathName() << "::wrapper_create()\n";
            ss << "{\n";
            ss << "  auto pThis = make_shared<wrapper::impl" << structObj->getPathName() << ">();\n";
            ss << "  pThis->thisWeak_ = pThis;\n";
            ss << "  return pThis;\n";
            ss << "}\n\n";
          }

          ss << dashedLine;
          ss << "wrapper::impl" << structObj->getPathName() << "::~" << structObj->mName << "()\n";
          ss << "{\n";
          ss << "}\n\n";

          if (needsDefaultConstructor) {
            ss << dashedLine;
            ss << "void " << "wrapper::impl" << structObj->getPathName() << "::wrapper_init_" << getStructInitName(structObj) << "()\n";
            ss << "{\n";
            ss << "}\n\n";
          }

          bool foundEventHandler = false;
          outputMethods(structObj, structObj, ss, foundEventHandler);

          if (foundEventHandler) {
            ss << dashedLine;
            ss << "void wrapper::impl" << structObj->getPathName() << "::wrapper_onObserverCountChanged(size_t count)\n";
            ss << "{\n";
            ss << "}\n\n";
          }
        }

        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        #pragma mark
        #pragma mark GenerateStructImplCpp::IIDLCompilerTarget
        #pragma mark

        //-------------------------------------------------------------------
        String GenerateStructImplCpp::targetKeyword()
        {
          return String("impl");
        }

        //-------------------------------------------------------------------
        String GenerateStructImplCpp::targetKeywordHelp()
        {
          return String("C++ implementation wrapper template");
        }

        //-------------------------------------------------------------------
        void GenerateStructImplCpp::targetOutput(
                                                 const String &inPathStr,
                                                 const ICompilerTypes::Config &config
                                                 ) throw (Failure)
        {
          typedef std::stack<NamespacePtr> NamespaceStack;
          typedef std::stack<String> StringList;

          String pathStr(UseHelper::fixRelativeFilePath(inPathStr, String("wrapper")));

          try {
            UseHelper::mkdir(pathStr);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to create path \"" + pathStr + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
          pathStr += "/";

          {
            auto header = GenerateStructImplHeader::create();
            header->targetOutput(pathStr, config);
          }

          const ProjectPtr &project = config.mProject;
          if (!project) return;
          if (!project->mGlobal) return;

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

              String filename = GenerateStructImplCpp::getStructFileName(structObj);

              String outputname = UseHelper::fixRelativeFilePath(pathStr, filename);

              std::stringstream ss;
              std::stringstream includeSS;
              std::stringstream structSS;

              ss << "// " ZS_EVENTING_GENERATED_BY "\n\n";

              includeSS << "#include \"" << GenerateStructImplHeader::getStructFileName(structObj) << "\"\n";

              generateUsingTypes(structSS, String());

              {
                GenerateStructImplCpp::StringSet processedHeaders;
                GenerateStructImplCpp::generateStructCppImpl(structObj, processedHeaders, structSS);
              }

              ss << includeSS.str();
              ss << "\n";
              ss << structSS.str();
              ss << "\n";

              writeBinary(outputname, UseHelper::convertToBuffer(ss.str()));
            }
          }
        }

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
