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
#include <zsLib/eventing/IIDLTypes.h>

#include <stack>

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
        #pragma mark IDLCompiler
        #pragma mark

        class IDLCompiler : public ICompiler,
                            public IIDLTypes
        {
          struct make_private {};

        public:
          ZS_DECLARE_TYPEDEF_PTR(IIDLTypes::Project, Project);
          
          typedef std::list<ElementPtr> ElementList;
          
          typedef std::map<Name, StringList> ModifierValueMap;

          ZS_DECLARE_STRUCT_PTR(Token);
          typedef std::list<TokenPtr> TokenList;
          ZS_DECLARE_PTR(TokenList);

          typedef std::stack<TokenPtr> TokenStack;
          typedef std::stack<TokenListPtr> TokenListStack;
          typedef std::list<TokenListPtr> TokenListList;

          enum TokenTypes
          {
            TokenType_First,
            
            TokenType_Unknown = TokenType_First,

            TokenType_Directive,
            TokenType_Documentation,
            TokenType_Char,
            TokenType_Quote,

            TokenType_Number,
            TokenType_Identifier,

            TokenType_SemiColon,

            TokenType_Brace,        // (
            TokenType_CurlyBrace,   // {
            TokenType_SquareBrace,  // [
            TokenType_AngleBrace,   // <

            TokenType_Operator,
            TokenType_ScopeOperator,
            TokenType_CommaOperator,
            TokenType_ColonOperator,
            TokenType_EqualsOperator,

            TokenType_Last = TokenType_EqualsOperator,
          };

          typedef std::set<TokenTypes> TokenTypeSet;

          struct Token
          {
            TokenTypes mTokenType {TokenType_First};
            String mToken;
            ULONG mLineCount {1};

            bool isBrace() const;
            bool isOpenBrace() const;
            bool isCloseBrace() const;
            bool isOpenBrace(TokenTypes type) const;
            bool isCloseBrace(TokenTypes type) const;
            bool isIdentifier(const char *identifier) const;
          };

        public:
          //-------------------------------------------------------------------
          IDLCompiler(
                      const make_private &,
                      const Config &config
                      );
          ~IDLCompiler();

          IDLCompiler(const Noop &) {}

          static void installDefaultTargets();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark IDLCompiler => ICompiler
          #pragma mark

          static IDLCompilerPtr create(const Config &config);

          virtual void process() throw (Failure, FailureWithLine);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark IDLCompiler => (internal)
          #pragma mark

          void outputSkeleton();
          void read() throw (Failure, FailureWithLine);
          void validate() throw (Failure);

          bool parseNamespace(NamespacePtr parent) throw (FailureWithLine);
          void parseNamespaceContents(NamespacePtr namespaceObj) throw (FailureWithLine);

          bool parseUsing(NamespacePtr namespaceObj) throw (FailureWithLine);
          bool parseTypedef(ContextPtr context) throw (FailureWithLine);
          bool parseStruct(ContextPtr context) throw (FailureWithLine);
          bool parseEnum(ContextPtr context) throw (FailureWithLine);
          bool parseProperty(StructPtr context) throw (FailureWithLine);
          bool parseMethod(StructPtr context) throw (FailureWithLine);

          bool parseDocumentation();
          bool parseSemiColon();
          bool parseComma();
          bool parseModifiers() throw (FailureWithLine);
          bool parseDirective() throw (FailureWithLine);
          bool pushDirectiveTokens(TokenPtr token) throw (FailureWithLine);
          bool parseDirectiveExclusive(bool &outIgnoreMode) throw (FailureWithLine);

          ElementPtr getDocumentation();
          ElementPtr getDirectives();
          void mergeDocumentation(ElementPtr &existingDocumentation);
          void mergeDirectives(ElementPtr &existingDocumentation);
          void mergeModifiers(ContextPtr context) throw (FailureWithLine);
          void fillContext(ContextPtr context);

          static String makeTypenameFromTokens(const TokenList &tokens) throw (InvalidContent);

          void pushTokens(const TokenList &tokens);
          void pushTokens(TokenListPtr tokens);
          TokenListPtr getTokens() const;
          TokenListPtr popTokens();

          bool hasMoreTokens() const;
          TokenPtr peekNextToken(const char *whatExpectingMoreTokens) throw (FailureWithLine);
          TokenPtr extractNextToken(const char *whatExpectingMoreTokens) throw (FailureWithLine);
          void putBackToken(TokenPtr token);
          void putBackTokens(const TokenList &tokens);
          ULONG getLastLineNumber() const;
          
          static void insertBefore(
                                   TokenList &tokens,
                                   const TokenList &insertTheseTokens
                                   );
          static void insertAfter(
                                  TokenList &tokens,
                                  const TokenList &insertTheseTokens
                                  );

          bool extractToClosingBraceToken(
                                          const char *whatExpectingClosingToken,
                                          TokenList &outTokens,
                                          bool includeOuterBrace = false
                                          ) throw (FailureWithLine);

          bool extractToComma(
                              const char *whatExpectingComma,
                              TokenList &outTokens
                              ) throw (FailureWithLine);
          
          bool extractToEquals(
                               const char *whatExpectingComma,
                               TokenList &outTokens
                               ) throw (FailureWithLine);
          
          bool extractToTokenType(
                                  const char *whatExpectingComma,
                                  TokenTypes searchTokenType,
                                  TokenList &outTokens,
                                  bool includeFoundToken = false,
                                  bool processBrackets = true
                                  ) throw (FailureWithLine);

          TokenPtr peekAheadToFirstTokenOfType(const TokenTypeSet &tokenTypes);

          void processUsingNamespace(
                                     NamespacePtr currentNamespace,
                                     NamespacePtr usingNamespace
                                     );
          void processUsingType(
                                NamespacePtr currentNamespace,
                                TypePtr usingType
                                );
          void processTypedef(
                              ContextPtr context,
                              const TokenList &typeTokens,
                              const String &typeName
                              ) throw (FailureWithLine);
          StructPtr processStructForward(
                                         ContextPtr context,
                                         const String &typeName,
                                         bool *wasCreated = NULL
                                         ) throw (FailureWithLine);
          void processRelated(
                              StructPtr structObj,
                              const TokenList &typeTokens
                              ) throw (FailureWithLine);

          TypePtr findTypeOrCreateTypedef(
                                          ContextPtr context,
                                          const TokenList &tokens,
                                          TypedefTypePtr &outCreatedTypedef
                                          ) throw (FailureWithLine);

          static void writeXML(const String &outputName, const DocumentPtr &doc) throw (Failure);
          static void writeJSON(const String &outputName, const DocumentPtr &doc) throw (Failure);
          static void writeBinary(const String &outputName, const SecureByteBlockPtr &buffer) throw (Failure);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark IDLCompiler => (data)
          #pragma mark

          IDLCompilerWeakPtr mThisWeak;

          Config mConfig;
          TokenList mPendingDocumentation;
          ElementList mPendingDirectives;
          ModifierValueMap mPendingModifiers;

          TokenListStack mTokenListStack;
          TokenStack mLastTokenStack;
          TokenPtr mLastToken;
        };

      } // namespace internal
    } // namespace tool
  } // namespace eventing
} // namespace zsLib
