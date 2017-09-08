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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_IDLCompiler.h>

#include <sstream>

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
        #pragma mark GenerateStructCx
        #pragma mark

        struct GenerateStructCx : public IIDLCompilerTarget,
                                  public IDLCompiler
        {
          typedef String NamePath;
          typedef std::set<String> StringSet;
          typedef std::set<StructPtr> StructSet;
          typedef std::map<NamePath, StructSet> NamePathStructSetMap;

          struct HelperFile
          {
            NamespacePtr mGlobal;
            NamePathStructSetMap mDerives;

            String mHeaderFileName;
            String mCppFileName;

            String mHeaderIndentStr;

            std::stringstream mHeaderIncludeSS;
            std::stringstream mHeaderStructSS;
            std::stringstream mHeaderFinalSS;
            std::stringstream mCppIncludeSS;
            std::stringstream mCppBodySS;

            StringSet mHeaderAlreadyIncluded;
            StringSet mCppAlreadyIncluded;

            void includeHeader(const String &headerFile);
            void includeCpp(const String &headerFile);
          };

          struct StructFile
          {
            StructPtr mStruct;
            std::stringstream mHeaderIncludeSS;
            std::stringstream mHeaderPreStructSS;
            std::stringstream mHeaderStructPrivateSS;
            std::stringstream mHeaderStructObserverSS;
            std::stringstream mHeaderStructObserverFinalSS;
            std::stringstream mHeaderStructPublicSS;
            std::stringstream mCppIncludeSS;
            std::stringstream mCppBodySS;
            String mHeaderIndentStr;
            String mHeaderStructIndentStr;

            StringSet mCppAlreadyIncluded;

            void includeCpp(const String &headerFile);
          };

          GenerateStructCx();

          static GenerateStructCxPtr create();

          static String fixName(const String &originalName);
          static String fixNamePath(ContextPtr context);
          static String fixStructName(StructPtr structObj);
          static String fixMethodDeclaration(ContextPtr context);
          static String fixMethodDeclaration(
                                             StructPtr derivedStruct,
                                             ContextPtr context
                                             );
          static String fixStructFileName(StructPtr structObj);
          static String getStructInitName(StructPtr structObj);
          static String getCxStructInitName(StructPtr structObj);
          static String fixEnumName(EnumTypePtr enumObj);
          static String fixArgumentName(const String &originalName);

          static void processTypesNamespace(
                                            std::stringstream &ss,
                                            const String &inIndentStr,
                                            NamespacePtr namespaceObj
                                            );
          static void processTypesStruct(
                                         std::stringstream &ss,
                                         const String &inIndentStr,
                                         StructPtr structObj,
                                         bool &firstFound
                                         );
          static bool processTypesEnum(
                                       std::stringstream &ss,
                                       const String &inIndentStr,
                                       ContextPtr context
                                       );

          static SecureByteBlockPtr generateTypesHeader(ProjectPtr project) throw (Failure);

          static void calculateRelations(
                                         NamespacePtr namespaceObj,
                                         NamePathStructSetMap &ioDerivesInfo
                                         );
          static void calculateRelations(
                                         StructPtr structObj,
                                         NamePathStructSetMap &ioDerivesInfo
                                         );

          static void insertInto(
                                 StructPtr structObj,
                                 const NamePath &namePath,
                                 NamePathStructSetMap &ioDerivesInfo
                                 );

          static void generateSpecialHelpers(HelperFile &helperFile);
          static void generateBasicTypesHelper(HelperFile &helperFile);
          static void generateExceptionHelper(HelperFile &helperFile);
          static void generateStringHelper(HelperFile &helperFile);
          static void generateBinaryHelper(HelperFile &helperFile);
          static void generateDurationHelper(
                                             HelperFile &helperFile,
                                             const String &durationType
                                             );
          static void generateTimeHelper(HelperFile &helperFile);
          static void generatePromiseHelper(HelperFile &helperFile);
          static void generatePromiseWithHelper(HelperFile &helperFile);
          static void generateDefaultPromiseRejections(
                                                       HelperFile &helperFile,
                                                       const String &indentStr
                                                       );
          static void generatePromiseRejection(
                                               HelperFile &helperFile,
                                               const String &indentStr,
                                               TypePtr rejectionType
                                               );

          static void generateForNamespace(
                                           HelperFile &helperFile,
                                           NamespacePtr namespaceObj,
                                           const String &inIndentStr
                                           );

          static void generateForStruct(
                                        HelperFile &helperFile,
                                        StructPtr structObj,
                                        const String &inIndentStr
                                        );
          static void generateForEnum(
                                      HelperFile &helperFile,
                                      EnumTypePtr enumObj
                                      );
          static void generateForStandardStruct(
                                                HelperFile &helperFile,
                                                StructPtr structObj
                                                );
          static void generateStructFile(
                                         HelperFile &helperFile,
                                         StructPtr structObj
                                         );
          static void generateStructMethods(
                                            HelperFile &helperFile, 
                                            StructFile &structFile,
                                            StructPtr derivedStructObj,
                                            StructPtr structObj,
                                            bool createConstructors,
                                            bool hasEvents
                                            );
          static void generateForList(
                                      HelperFile &helperFile,
                                      StructPtr structObj
                                      );
          static void generateForMap(
                                     HelperFile &helperFile,
                                     StructPtr structObj
                                     );
          static void generateForSet(
                                     HelperFile &helperFile,
                                     StructPtr structObj
                                     );

          static String getBasicCxTypeString(
                                             bool isOptional,
                                             BasicTypePtr type,
                                             bool isReturnType = false
                                             );
          static String makeCxOptional(
                                       bool isOptional,
                                       const String &value
                                       );
          static String getCppType(
                                   bool isOptional,
                                   TypePtr type
                                   );
          static String getCxType(
                                  bool isOptional,
                                  TypePtr type,
                                  bool isReturnType = false
                                  );
          static String getCxAttributes(const StringList &attributes);
          static String getCxAttributesLine(
                                            const String &linePrefix,
                                            const StringList &attributes
                                            );
          static String getToFromCxName(TypePtr type);
          static String getToCxName(TypePtr type);
          static String getFromCxName(TypePtr type);
          static void includeCppForType(
                                        StructFile &structFile,
                                        TypePtr type
                                        );

          struct IncludeProcessedInfo
          {
            StringSet processedTypes_;
            StringSet structProcessedTypes_;
            StringSet templatedProcessedTypes_;
          };

          static void includeCppForType(
                                        IncludeProcessedInfo &processed,
                                        StructFile &structFile,
                                        TypePtr type
                                        );
          static void includeTemplatedStructForType(
                                                    IncludeProcessedInfo &processed,
                                                    StructFile &structFile,
                                                    StructPtr structObj
                                                    );
          static void includeTemplatedStructForType(
                                                    IncludeProcessedInfo &processed,
                                                    StructFile &structFile,
                                                    TemplatedStructTypePtr templatedStructObj
                                                    );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark GenerateStructCx::IIDLCompilerTarget
          #pragma mark

          //-------------------------------------------------------------------
          virtual String targetKeyword() override;
          virtual String targetKeywordHelp() override;
          virtual void targetOutput(
                                    const String &inPathStr,
                                    const ICompilerTypes::Config &config
                                    ) throw (Failure) override;
        };
         
      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
