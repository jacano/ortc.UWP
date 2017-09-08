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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateStructHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateHelper.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_GenerateTypesHeader.h>
#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Helper.h>

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
        #pragma mark GenerateStructHeader
        #pragma mark

        //-------------------------------------------------------------------
        GenerateStructHeader::GenerateStructHeader() : IDLCompiler(Noop{})
        {
        }

        //-------------------------------------------------------------------
        GenerateStructHeaderPtr GenerateStructHeader::create()
        {
          return make_shared<GenerateStructHeader>();
        }

        //---------------------------------------------------------------------
        SecureByteBlockPtr GenerateStructHeader::generateTypesHeader(ProjectPtr project) throw (Failure)
        {
          std::stringstream ss;

          if (!project) return SecureByteBlockPtr();
          if (!project->mGlobal) return SecureByteBlockPtr();

          ss << "// " ZS_EVENTING_GENERATED_BY "\n\n";
          ss << "#pragma once\n\n";
          ss << "#include <stdint.h>\n";
          ss << "#include <list>\n";
          ss << "#include <set>\n";
          ss << "#include <map>\n";
          ss << "#include <zsLib/types.h>\n";
          ss << "#include <zsLib/String.h>\n";
          ss << "#include <zsLib/Promise.h>\n";
          ss << "#include <zsLib/eventing/types.h>\n";
          ss << "\n";
          ss << "namespace wrapper {\n";
          generateUsingTypes(ss, "  ");
          ss << "\n";

          GenerateTypesHeader::processTypesNamespace(ss, String(), project->mGlobal, true);

          ss << "} // namespace wrapper\n\n";

          return UseHelper::convertToBuffer(ss.str());
        }
        
        //---------------------------------------------------------------------
        void GenerateStructHeader::generateUsingTypes(
                                                      std::stringstream &ss,
                                                      const String &indentStr
                                                      )
        {
          ss << indentStr << "using ::zsLib::String;\n";
          ss << indentStr << "using ::zsLib::Optional;\n";
          ss << indentStr << "using ::zsLib::Any;\n";
          ss << indentStr << "using ::zsLib::AnyPtr;\n";
          ss << indentStr << "using ::zsLib::AnyHolder;\n";
          ss << indentStr << "using ::zsLib::Promise;\n";
          ss << indentStr << "using ::zsLib::PromisePtr;\n";
          ss << indentStr << "using ::zsLib::PromiseWithHolder;\n";
          ss << indentStr << "using ::zsLib::PromiseWithHolderPtr;\n";
          ss << indentStr << "using ::zsLib::eventing::SecureByteBlock;\n";
          ss << indentStr << "using ::zsLib::eventing::SecureByteBlockPtr;\n";
          ss << indentStr << "using ::std::shared_ptr;\n";
          ss << indentStr << "using ::std::weak_ptr;\n";
          ss << indentStr << "using ::std::make_shared;\n";
          ss << indentStr << "using ::std::list;\n";
          ss << indentStr << "using ::std::set;\n";
          ss << indentStr << "using ::std::map;\n";
        }

        //-------------------------------------------------------------------
        String GenerateStructHeader::getStructFileName(StructPtr structObj)
        {
          String filename = getStructInitName(structObj);
          filename += ".h";
          return filename;
        }

        //-------------------------------------------------------------------
        String GenerateStructHeader::getStructInitName(StructPtr structObj)
        {
          String namePathStr = structObj->getPathName();
          namePathStr.replaceAll("::", "_");
          namePathStr.trim("_");
          return namePathStr;
        }


        //---------------------------------------------------------------------
        String GenerateStructHeader::makeOptional(bool isOptional, const String &value)
        {
          if (!isOptional) return value;
          return "Optional< " + value + " >";
        }

        //---------------------------------------------------------------------
        String GenerateStructHeader::getWrapperTypeString(bool isOptional, TypePtr type)
        {
          if (!type) return String();

          type = type->getOriginalType();

          {
            auto typedefType = type->toTypedefType();
            if (typedefType) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Typedef failed to resolve to original type: " + typedefType->getPathName());
            }
          }

          {
            auto basicType = type->toBasicType();
            if (basicType) {
              return makeOptional(isOptional, GenerateHelper::getBasicTypeString(basicType));
            }
          }

          {
            auto structType = type->toStruct();
            if (structType) {
              if (structType->mGenerics.size() > 0) return String();
              if (structType->hasModifier(Modifier_Special)) {
                String specialName = structType->getPathName();
                if ("::zs::Any" == specialName) return "AnyPtr";
                if ("::zs::Promise" == specialName) return "PromisePtr";
                if ("::zs::exceptions::Exception" == specialName) return "::zsLib::Exception";
                if ("::zs::exceptions::InvalidArgument" == specialName) return "::zsLib::Exceptions::InvalidArgument";
                if ("::zs::exceptions::BadState" == specialName) return "::zsLib::Exceptions::BadState";
                if ("::zs::exceptions::NotImplemented" == specialName) return "::zsLib::Exceptions::NotImplemented";
                if ("::zs::exceptions::NotSupported" == specialName) return "::zsLib::Exceptions::NotSupported";
                if ("::zs::exceptions::UnexpectedError" == specialName) return "::zsLib::Exceptions::UnexpectedError";
                if ("::zs::Time" == specialName) return makeOptional(isOptional, "::zsLib::Time");
                if ("::zs::Milliseconds" == specialName) return makeOptional(isOptional, "::zsLib::Milliseconds");
                if ("::zs::Microseconds" == specialName) return makeOptional(isOptional, "::zsLib::Microseconds");
                if ("::zs::Nanoseconds" == specialName) return makeOptional(isOptional, "::zsLib::Nanoseconds");
                if ("::zs::Seconds" == specialName) return makeOptional(isOptional, "::zsLib::Seconds");
                if ("::zs::Minutes" == specialName) return makeOptional(isOptional, "::zsLib::Minutes");
                if ("::zs::Hours" == specialName) return makeOptional(isOptional, "::zsLib::Hours");
              }

              return makeOptional(isOptional, "wrapper" + structType->getPathName() + "Ptr");
            }
          }

          {
            auto enumType = type->toEnumType();
            if (enumType) {
              return makeOptional(isOptional, "wrapper" + enumType->getPathName());
            }
          }

          {
            auto templatedType = type->toTemplatedStructType();
            if (templatedType) {
              String templatedTypeStr;
              String specialName;
              bool specialTemplate = false;

              {
                auto parent = type->getParent();
                if (parent) {
                  auto parentStruct = parent->toStruct();
                  if (parentStruct) {
                    if (parentStruct->hasModifier(Modifier_Special)) {
                      specialName = parentStruct->getPathName();
                      if ("::std::set" == specialName) templatedTypeStr = "shared_ptr< set< ";
                      if ("::std::list" == specialName) templatedTypeStr = "shared_ptr< list< ";
                      if ("::std::map" == specialName) templatedTypeStr = "shared_ptr< map< ";
                      if ("::zs::PromiseWith" == specialName) {
                        auto firstArg = templatedType->mTemplateArguments.begin();
                        if (firstArg != templatedType->mTemplateArguments.end()) {
                          auto typePtr = (*firstArg);
                          if (typePtr->toBasicType()) {
                            templatedTypeStr = "shared_ptr< PromiseWithHolder< ";
                          }
                        }
                        if (templatedTypeStr.isEmpty()) {
                          templatedTypeStr = "shared_ptr< PromiseWithHolderPtr< ";
                        }
                      }
                      specialTemplate = templatedTypeStr.hasData();
                    }
                  }
                }
              }

              if (templatedTypeStr.isEmpty()) {
                templatedTypeStr = "wrapper" + templatedType->getPathName() + "< ";
              }
              bool first = true;
              for (auto iter = templatedType->mTemplateArguments.begin(); iter != templatedType->mTemplateArguments.end(); ++iter)
              {
                auto templateArg = (*iter);
                String typeStr = getWrapperTypeString(false, templateArg);
                if ("void" == typeStr) continue;
                if (!first) templatedTypeStr += ", ";
                templatedTypeStr += typeStr;
                first = false;
              }
              templatedTypeStr += " >";
              if (specialTemplate) templatedTypeStr += " >";
              return makeOptional(isOptional, templatedTypeStr);
            }
          }
          return String();
        }

        //-------------------------------------------------------------------
        void GenerateStructHeader::generateStruct(
                                                  StructPtr structObj,
                                                  String indentStr,
                                                  StringSet &includedHeaders,
                                                  std::stringstream &includeSS,
                                                  std::stringstream &ss
                                                  )
        {
          if (!structObj) return;
          if (GenerateHelper::isBuiltInType(structObj)) return;
          if (structObj->mGenerics.size() > 0) return;

          auto rootStruct = structObj->getRootStruct();

          ss << "\n";
          ss << indentStr << "struct " << structObj->mName;

          if (structObj->mIsARelationships.size() > 0) {
            ss << " : ";
          }
          else {
            ss << "\n";
          }

          // output relationships
          {
            bool first{ true };
            for (auto iterRelations = structObj->mIsARelationships.begin(); iterRelations != structObj->mIsARelationships.end(); ++iterRelations)
            {
              auto relatedObj = (*iterRelations).second;

              {
                auto relatedStructObj = relatedObj->toStruct();
                if (relatedStructObj) {
                  auto relatedRootStructObj = relatedStructObj->getRootStruct();
                  if (rootStruct != relatedRootStructObj) {
                    auto includeStr = getStructFileName(relatedStructObj);
                    if (includedHeaders.end() == includedHeaders.find(includeStr)) {
                      includeSS << "#include \"" << includeStr << "\"\n";
                      includedHeaders.insert(includeStr);
                    }
                  }
                }

                if (!first) {
                  ss << ",\n";
                  ss << indentStr << "          ";
                  for (size_t index = 0; index < structObj->mName.length(); ++index)
                  {
                    ss << " ";
                  }
                }
                ss << "public wrapper" << relatedStructObj->getPathName();
              }

              first = false;
            }
            if (!first) ss << "\n";
          }

          ss << indentStr << "{\n";

          String currentIdentStr = indentStr;
          indentStr += "  ";

          bool outputSubTypes = false;

          for (auto iterSubStruct = structObj->mStructs.begin(); iterSubStruct != structObj->mStructs.end(); ++iterSubStruct)
          {
            auto subStructObj = (*iterSubStruct).second;
            if (!subStructObj) continue;
            if (GenerateHelper::isBuiltInType(subStructObj)) continue;
            if (subStructObj->mGenerics.size() > 0) continue;
            ss << indentStr << "ZS_DECLARE_STRUCT_PTR(" << subStructObj->mName << ");\n";
            outputSubTypes = true;
          }

          for (auto iterEnums = structObj->mEnums.begin(); iterEnums != structObj->mEnums.end(); ++iterEnums)
          {
            auto enumObj = (*iterEnums).second;

            ss << "\n";
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
            outputSubTypes = true;
          }

          if (outputSubTypes) ss << "\n";

          if (!structObj->hasModifier(Modifier_Static)) {
            ss << indentStr << "static " << structObj->mName << "Ptr wrapper_create();\n";
          }
          ss << indentStr << "virtual ~" << structObj->mName << "() {}\n\n";

          for (auto iterStructs = structObj->mStructs.begin(); iterStructs != structObj->mStructs.end(); ++iterStructs)
          {
            auto subStructObj = (*iterStructs).second;
            generateStruct(subStructObj, indentStr, includedHeaders, includeSS, ss);
          }

          std::stringstream observerSS;
          std::stringstream observerMethodsSS;

          bool foundCtor = false;
          bool foundEventHandler = false;
          bool firstMethod = true;
          for (auto iterMethods = structObj->mMethods.begin(); iterMethods != structObj->mMethods.end(); ++iterMethods)
          {
            auto methodObj = (*iterMethods);

            if (methodObj->hasModifier(Modifier_Method_Delete)) continue;

            bool isCtor = methodObj->hasModifier(Modifier_Method_Ctor);

            if (methodObj->hasModifier(Modifier_Method_EventHandler)) {
              if (!foundEventHandler) {
                observerSS << "\n";
                observerSS << indentStr << "ZS_DECLARE_STRUCT_PTR(WrapperObserver);\n";
                observerSS << "\n";
                observerSS << indentStr << "struct WrapperObserver\n";
                observerSS << indentStr << "{\n";
                observerMethodsSS << indentStr << "};\n\n";
                observerMethodsSS << indentStr << "::zsLib::Lock wrapper_observerLock;\n";
                observerMethodsSS << indentStr << "ZS_DECLARE_TYPEDEF_PTR(::std::list<WrapperObserverWeakPtr>, WrapperObserverWeakList);\n";
                observerMethodsSS << indentStr << "WrapperObserverWeakListPtr wrapper_observers {::std::make_shared<WrapperObserverWeakList>()};\n";
                observerMethodsSS << "\n";
                observerMethodsSS << indentStr << "virtual void wrapper_onObserverCountChanged(size_t count) = 0;\n";
                observerMethodsSS << "\n";
                observerMethodsSS << indentStr << "void wrapper_installObserver(WrapperObserverPtr observer)\n";
                observerMethodsSS << indentStr << "{\n";
                observerMethodsSS << indentStr << "  size_t count {};\n";
                observerMethodsSS << indentStr << "  {\n";
                observerMethodsSS << indentStr << "    ::zsLib::AutoLock lock(wrapper_observerLock);\n";
                observerMethodsSS << indentStr << "    WrapperObserverWeakListPtr oldList;\n";
                observerMethodsSS << indentStr << "    WrapperObserverWeakListPtr newList(make_shared<WrapperObserverWeakList>());\n";
                observerMethodsSS << indentStr << "    if (observer) { newList->push_back(observer); }\n";
                observerMethodsSS << indentStr << "    for (auto iter = oldList->begin(); iter != oldList->end(); ++iter) {\n";
                observerMethodsSS << indentStr << "      WrapperObserverPtr existingObserver = (*iter).lock();\n";
                observerMethodsSS << indentStr << "      if (observer) { newList->push_back(existingObserver); }\n";
                observerMethodsSS << indentStr << "    }\n";
                observerMethodsSS << indentStr << "    count = newList->size();\n";
                observerMethodsSS << indentStr << "    wrapper_observers = newList;\n";
                observerMethodsSS << indentStr << "  }\n";
                observerMethodsSS << indentStr << "  wrapper_onObserverCountChanged(count);\n";
                observerMethodsSS << indentStr << "}\n";
                observerMethodsSS << indentStr << "void wrapper_observerClean() { wrapper_installObserver(WrapperObserverPtr()); }\n\n";
                foundEventHandler = true;
              }
              observerSS << indentStr << "  virtual void " << methodObj->mName << "(";
              observerMethodsSS << indentStr << "void " << methodObj->mName << "(";

              std::stringstream observerMethodsParamsSS;
              observerMethodsParamsSS << ")\n";
              observerMethodsParamsSS << indentStr << "{\n";
              observerMethodsParamsSS << indentStr << "  WrapperObserverWeakListPtr observers;\n";
              observerMethodsParamsSS << indentStr << "  { ::zsLib::AutoLock lock(wrapper_observerLock); observers = wrapper_observers; }\n";
              observerMethodsParamsSS << indentStr << "  bool clean {false};\n";
              observerMethodsParamsSS << indentStr << "  for (auto iter = observers->begin(); iter != observers->end(); ++iter) {\n";
              observerMethodsParamsSS << indentStr << "    auto observer = (*iter).lock();\n";
              observerMethodsParamsSS << indentStr << "    if (!observer) { clean = true; continue; }\n";
              observerMethodsParamsSS << indentStr << "    observer->" << methodObj->mName << "(";

              {
                bool firstArgument = true;
                for (auto iterParams = methodObj->mArguments.begin(); iterParams != methodObj->mArguments.end(); ++iterParams)
                {
                  auto argument = (*iterParams);
                  if (!firstArgument) {
                    observerSS << ", ";
                    observerMethodsSS << ", ";
                    observerMethodsParamsSS << ", ";
                  }
                  firstArgument = false;

                  String typeStr = getWrapperTypeString(argument->hasModifier(Modifier_Optional), argument->mType);
                  observerSS << typeStr << " " << argument->mName;
                  observerMethodsSS << typeStr << " " << argument->mName;
                  observerMethodsParamsSS << argument->mName;
                }
              }
              observerSS << ") = 0;\n";

              observerMethodsParamsSS << ");\n";
              observerMethodsParamsSS << indentStr << "  }\n";
              observerMethodsParamsSS << indentStr << "  if (clean) wrapper_observerClean();\n";
              observerMethodsParamsSS << indentStr << "}\n";

              observerMethodsSS << observerMethodsParamsSS.str();
              continue;
            }

            if (firstMethod) ss << "\n";
            firstMethod = false;

            ss << indentStr;
            if (methodObj->hasModifier(Modifier_Static))
              ss << "static ";
            else
              ss << "virtual ";

            if (isCtor) {
              ss << "void wrapper_init_" << getStructInitName(structObj) << "(";
              foundCtor = true;
            } else {
              ss << getWrapperTypeString(methodObj->hasModifier(Modifier_Optional), methodObj->mResult);
              ss << " " << methodObj->mName << "(";
            }

            // append arguments
            {
              bool firstArgument = true;
              for (auto iterParams = methodObj->mArguments.begin(); iterParams != methodObj->mArguments.end(); ++iterParams)
              {
                auto argument = (*iterParams);
                if (!firstArgument) ss << ", ";
                firstArgument = false;

                if (methodObj->mArguments.size() > 1) ss << "\n" << indentStr << "  ";

                String typeStr = getWrapperTypeString(argument->hasModifier(Modifier_Optional), argument->mType);
                ss << typeStr << " " << argument->mName;
              }
            }

            if (methodObj->mArguments.size() > 1) ss << "\n" << indentStr << "  ";
            if (methodObj->hasModifier(Modifier_Static)) {
              ss << ");\n";
            } else {
              if (isCtor) {
                ss << ") {}\n";
              } else {
                ss << ") = 0;\n";
              }
            }
          }

          if (GenerateHelper::needsDefaultConstructor(structObj)) {
            ss << indentStr << "virtual void wrapper_init_" << getStructInitName(structObj) << "() {}\n";
          }

          bool isDictionary = structObj->hasModifier(Modifier_Struct_Dictionary);
          bool firstProperty = true;
          for (auto iterProperties = structObj->mProperties.begin(); iterProperties != structObj->mProperties.end(); ++iterProperties)
          {
            auto propertyObj = (*iterProperties);
            bool hasGetter = propertyObj->hasModifier(Modifier_Property_Getter);
            bool hasSetter = propertyObj->hasModifier(Modifier_Property_Setter);

            String typeStr = getWrapperTypeString(propertyObj->hasModifier(Modifier_Optional), propertyObj->mType);

            if (!isDictionary) {
              if (!((hasGetter) || (hasSetter))) {
                hasGetter = hasSetter = true;
              }
            }

            if (firstProperty) ss << "\n";
            firstProperty = false;

            if (propertyObj->hasModifier(Modifier_Static)) {
              if (!((hasGetter) || (hasSetter))) {
                hasGetter = hasSetter = true;
              }

              if (hasGetter) {
                ss << indentStr << "static " << typeStr << " get_" << propertyObj->mName << "();\n";
              }
              if (hasSetter) {
                ss << indentStr << "static void set_" << propertyObj->mName << "(" << typeStr << " value);\n";
              }
              continue;
            }

            if (!((hasGetter) || (hasSetter))) {
              ss << indentStr << typeStr << " " << propertyObj->mName << " {";
              if (propertyObj->mDefaultValue.hasData()) {
                String defaultValue = propertyObj->mDefaultValue;

                {
                  auto enumType = propertyObj->mType->toEnumType();
                  if (enumType) {
                    defaultValue = String("wrapper") + enumType->getPathName() + "_" + defaultValue;
                  }
                }
                ss << defaultValue;
              }
              ss << "};\n";
              continue;
            }

            if (hasGetter) {
              ss << indentStr << "virtual " << typeStr << " get_" << propertyObj->mName << "() = 0;\n";
            }
            if (hasSetter) {
              ss << indentStr << "virtual void set_" << propertyObj->mName << "(" << typeStr << " value) = 0;\n";
            }
          }

          ss << observerSS.str();
          ss << observerMethodsSS.str();

          indentStr = currentIdentStr;
          ss << indentStr << "};\n";
        }

        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        #pragma mark
        #pragma mark GenerateStructHeader::IIDLCompilerTarget
        #pragma mark

        //-------------------------------------------------------------------
        String GenerateStructHeader::targetKeyword()
        {
          return String("wrapper");
        }

        //-------------------------------------------------------------------
        String GenerateStructHeader::targetKeywordHelp()
        {
          return String("C++ wrapper API");
        }

        //-------------------------------------------------------------------
        void GenerateStructHeader::targetOutput(
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
          pathStr = UseHelper::fixRelativeFilePath(pathStr, String("generated"));
          try {
            UseHelper::mkdir(pathStr);
          } catch (const StdError &e) {
            ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_SYSTEM_ERROR, "Failed to create path \"" + pathStr + "\": " + " error=" + string(e.result()) + ", reason=" + e.message());
          }
          pathStr += "/";

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

              String filename = GenerateStructHeader::getStructFileName(structObj);

              String outputname = UseHelper::fixRelativeFilePath(pathStr, filename);

              std::stringstream ss;
              std::stringstream includeSS;
              std::stringstream structSS;
              StringList endStrings;

              ss << "// " ZS_EVENTING_GENERATED_BY "\n\n";
              ss << "#pragma once\n\n";
              ss << "#include \"types.h\"\n";

              structSS << "namespace wrapper {\n";

              NamespaceStack parentStack;
              auto parent = structObj->getParent();
              while (parent) {
                auto parentNamespace = parent->toNamespace();
                if (parentNamespace) {
                  parentStack.push(parentNamespace);
                }
                parent = parent->getParent();
              }

              String indentStr = "  ";

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
                GenerateStructHeader::StringSet processedHeaders;
                GenerateStructHeader::generateStruct(structObj, indentStr, processedHeaders, includeSS, structSS);
              }

              ss << includeSS.str();
              ss << "\n";
              ss << structSS.str();
              ss << "\n";
              while (endStrings.size() > 0) {
                ss << endStrings.top();
                endStrings.pop();
              }
              ss << "} // namespace wrapper\n\n";

              writeBinary(outputname, UseHelper::convertToBuffer(ss.str()));
            }
          }
        }

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
