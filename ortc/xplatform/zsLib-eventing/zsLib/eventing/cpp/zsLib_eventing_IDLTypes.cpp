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

#include <zsLib/eventing/internal/zsLib_eventing_IDLTypes.h>
#include <zsLib/eventing/internal/zsLib_eventing_Helper.h>

#include <zsLib/eventing/IHasher.h>

#include <zsLib/Numeric.h>
#include <zsLib/Stringize.h>


#include <cstdio>

namespace zsLib { namespace eventing { ZS_DECLARE_SUBSYSTEM(zsLib_eventing); } }

namespace zsLib
{
  namespace eventing
  {
    ZS_DECLARE_TYPEDEF_PTR(eventing::IHelper, UseHelper);

    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark
      
      //-----------------------------------------------------------------------
      bool splitToNamePath(
                           const String &typeNameWithPath,
                           String &outPath,
                           String &outName
                           )
      {
        outPath = String();
        outName = String();
        
        UseHelper::SplitMap splits;
        UseHelper::split(typeNameWithPath, splits, "::");
        UseHelper::splitTrim(splits);
        UseHelper::splitPruneEmpty(splits);
        
        if (splits.size() < 1) return false;
        
        {
          auto found = splits.find(splits.size()-1);
          ZS_THROW_INVALID_ASSUMPTION_IF(found == splits.end());
          
          outName = (*found).second;
          
          splits.erase(found);
        }

        outPath = UseHelper::combine(splits, "::");

        // put back global prefix if has global prefix
        if ("::" == typeNameWithPath.substr(0, 2)) {
          outPath = "::" + outPath;
        }

        return true;
      }
    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IIDLTypes::toString(Modifiers value)
    {
      switch (value)
      {
        case IIDLTypes::Modifier_Struct_Dictionary:         return "dictionary";
        case IIDLTypes::Modifier_Struct_Exception:          return "exception";

        case IIDLTypes::Modifier_Method_Ctor:               return "constructor";
        case IIDLTypes::Modifier_Method_EventHandler:       return "event";
        case IIDLTypes::Modifier_Method_Default:            return "default";
        case IIDLTypes::Modifier_Method_Delete:             return "delete";
          
        case IIDLTypes::Modifier_Method_Argument_In:        return "in";
        case IIDLTypes::Modifier_Method_Argument_Out:       return "out";
        case IIDLTypes::Modifier_Method_Argument_Grouping:  return "grouping";

        case IIDLTypes::Modifier_Property_ReadOnly:         return "readonly";
        case IIDLTypes::Modifier_Property_WriteOnly:        return "writeonly";
        case IIDLTypes::Modifier_Property_Getter:           return "getter";
        case IIDLTypes::Modifier_Property_Setter:           return "setter";

        case IIDLTypes::Modifier_Static:                    return "static";
        case IIDLTypes::Modifier_AltName:                   return "altname";
        case IIDLTypes::Modifier_Special:                   return "special";
        case IIDLTypes::Modifier_Platform:                  return "platform";
        case IIDLTypes::Modifier_Nullable:                  return "nullable";
        case IIDLTypes::Modifier_Optional:                  return "optional";
        case IIDLTypes::Modifier_Dynamic:                   return "dynamic";
        case IIDLTypes::Modifier_Obsolete:                  return "obsolete";
      }

      return "unknown";
    }
    
    //-------------------------------------------------------------------------
    int IIDLTypes::getTotalParams(Modifiers value)
    {
      switch (value)
      {
        case IIDLTypes::Modifier_Struct_Dictionary:         return 0;
        case IIDLTypes::Modifier_Struct_Exception:          return 0;

        case IIDLTypes::Modifier_Method_Ctor:               return 0;
        case IIDLTypes::Modifier_Method_EventHandler:       return -1;
        case IIDLTypes::Modifier_Method_Default:            return 0;
        case IIDLTypes::Modifier_Method_Delete:             return 0;

        case IIDLTypes::Modifier_Method_Argument_In:        return 0;
        case IIDLTypes::Modifier_Method_Argument_Out:       return 0;
        case IIDLTypes::Modifier_Method_Argument_Grouping:  return 1;

        case IIDLTypes::Modifier_Property_ReadOnly:         return 0;
        case IIDLTypes::Modifier_Property_WriteOnly:        return 0;
        case IIDLTypes::Modifier_Property_Getter:           return 0;
        case IIDLTypes::Modifier_Property_Setter:           return 0;

        case IIDLTypes::Modifier_Static:                    return 0;
        case IIDLTypes::Modifier_AltName:                   return 1;
        case IIDLTypes::Modifier_Special:                   return 0;
        case IIDLTypes::Modifier_Platform:                  return -1;
        case IIDLTypes::Modifier_Nullable:                  return 0;
        case IIDLTypes::Modifier_Optional:                  return 0;
        case IIDLTypes::Modifier_Dynamic:                   return 0;
        case IIDLTypes::Modifier_Obsolete:                  return 1;
      }

      return -1;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::Modifiers IIDLTypes::toModifier(const char *value) throw (InvalidArgument)
    {
      String str(value);
      for (IIDLTypes::Modifiers index = IIDLTypes::Modifier_First; index <= IIDLTypes::Modifier_Last; index = static_cast<IIDLTypes::Modifiers>(static_cast<std::underlying_type<IIDLTypes::Modifiers>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IIDLTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Not a valid modifier: ") + str);
    }
    
    //-------------------------------------------------------------------------
    bool IIDLTypes::isValidForAll(Modifiers value)
    {
      switch (value)
      {
        case IIDLTypes::Modifier_AltName:
        case IIDLTypes::Modifier_Platform:
        {
          return true;
        }
        default:                                          break;
      }
      return false;
    }
    
    
    //-------------------------------------------------------------------------
    bool IIDLTypes::isValidForNamespace(Modifiers value)
    {
      if (isValidForAll(value)) return true;
      
      switch (value)
      {
        case IIDLTypes::Modifier_Special:
        case IIDLTypes::Modifier_Obsolete:
        {
          return true;
        }
        default:                                              break;
      }
      
      return false;
    }

    //-------------------------------------------------------------------------
    bool IIDLTypes::isValidForStruct(Modifiers value)
    {
      if (isValidForAll(value)) return true;
      
      switch (value)
      {
        case IIDLTypes::Modifier_Struct_Dictionary:
        case IIDLTypes::Modifier_Struct_Exception:
        case IIDLTypes::Modifier_Static:
        case IIDLTypes::Modifier_Special:
        case IIDLTypes::Modifier_Obsolete:
        {
          return true;
        }
        default:                                              break;
      }
      
      return false;
    }

    //-------------------------------------------------------------------------
    bool IIDLTypes::isValidForMethod(Modifiers value)
    {
      if (isValidForAll(value)) return true;

      switch (value)
      {
        case IIDLTypes::Modifier_Method_Ctor:
        case IIDLTypes::Modifier_Method_EventHandler:
        case IIDLTypes::Modifier_Method_Default:
        case IIDLTypes::Modifier_Method_Delete:
        case IIDLTypes::Modifier_Static:
        case IIDLTypes::Modifier_Nullable:
        case IIDLTypes::Modifier_Dynamic:
        case IIDLTypes::Modifier_Obsolete:
        {
          return true;
        }
        default:                                          break;
      }
      
      return false;
    }
    
    //-------------------------------------------------------------------------
    bool IIDLTypes::isValidForMethodArgument(Modifiers value)
    {
      if (isValidForAll(value)) return true;
      
      switch (value)
      {
        case IIDLTypes::Modifier_Method_Argument_In:
        case IIDLTypes::Modifier_Method_Argument_Out:
        case IIDLTypes::Modifier_Method_Argument_Grouping:
        case IIDLTypes::Modifier_Nullable:
        case IIDLTypes::Modifier_Optional:
        case IIDLTypes::Modifier_Dynamic:
        {
          return true;
        }
        default:                                                break;
      }
      
      return false;
    }
    
    //-------------------------------------------------------------------------
    bool IIDLTypes::isValidForProperty(Modifiers value)
    {
      if (isValidForAll(value)) return true;
      
      switch (value)
      {
        case IIDLTypes::Modifier_Property_ReadOnly:
        case IIDLTypes::Modifier_Property_WriteOnly:
        case IIDLTypes::Modifier_Property_Getter:
        case IIDLTypes::Modifier_Property_Setter:
        case IIDLTypes::Modifier_Static:
        case IIDLTypes::Modifier_Nullable:
        case IIDLTypes::Modifier_Optional:
        case IIDLTypes::Modifier_Dynamic:
        case IIDLTypes::Modifier_Obsolete:
        {
          return true;
        }
        default:                                          break;
      }

      return false;
    }
    

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Context
    #pragma mark

    //-------------------------------------------------------------------------
    String IIDLTypes::Context::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update(mName);

      if (mDocumentation) {
        hasher->update(":documentation:");
        hasher->update(UseHelper::toString(mDocumentation));
      }

      if (mModifiers.size() > 0) {
        hasher->update(":modifiers:");
        for (auto iter = mModifiers.begin(); iter != mModifiers.end(); ++iter) {
          auto name = (*iter).first;
          auto values = (*iter).second;
          
          hasher->update(name);
          hasher->update(":values:");
          for (auto iterValues = values.begin(); iterValues != values.end(); ++iterValues)
          {
            auto value = (*iterValues);
            hasher->update(value);
            hasher->update(":value:");
          }
          hasher->update(":next:");
        }
        
      }

      hasher->update(":end");

      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    IIDLTypes::ContextPtr IIDLTypes::Context::getParent() const
    {
      return mContext.lock();
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::ContextPtr IIDLTypes::Context::getRoot() const
    {
      auto parent = getParent();
      if (!parent) return ContextPtr();
      
      while (true)
      {
        auto temp = parent->mContext.lock();
        if (!temp) return parent;

        parent = temp;
      }
    }

    //-------------------------------------------------------------------------
    IIDLTypes::ProjectPtr IIDLTypes::Context::getProject() const
    {
      auto project = getRoot();
      if (!project) return ProjectPtr();
      return project->toProject();
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::Context::getPath() const
    {
      auto parent = getParent();
      if (!parent) return String();
      
      ContextList parents;

      while (parent) {
        parents.push_front(parent);
        
        parent = parent->getParent();
      }

      String pathStr;

      for (auto iter = parents.begin(); iter != parents.end(); ++iter)
      {
        parent = (*iter);
        
        {
          auto namespaceObj = parent->toNamespace();
          if (namespaceObj) {
            String mappingName = namespaceObj->getMappingName();
            if (mappingName.hasData()) {
              pathStr += "::";
              pathStr += mappingName;
            }
            goto next;
          }
        }

        {
          auto structObj = parent->toStruct();
          if (structObj) {
            pathStr += "::";
            pathStr += structObj->getMappingName();
            goto next;
          }
        }
        
      next:
        {
        }
      }

      if (pathStr.isEmpty()) return String("::");

      return pathStr;
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::Context::getPathName() const
    {
      String path = getPath();
      if ("::" == path) return path + getMappingName();
      return path + "::" + getMappingName();
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::Context::findType(
                                                    const String &typeNameWithPath,
                                                    const FindTypeOptions *options
                                                    ) const
    {
      FindTypeOptions defaultOptions;
      if (!options) options = &defaultOptions;
      
      String path;
      String name;

      if (!internal::splitToNamePath(typeNameWithPath, path, name)) return TypePtr();

      return findType(path, name, *options);
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::Context::findType(
                                                    const String &pathStr,
                                                    const String &typeName,
                                                    const FindTypeOptions &options
                                                    ) const
    {
      if (options.mSearchParents) {
        auto parent = getParent();
        if (!parent) return TypePtr();

        return parent->findType(pathStr, typeName, options);
      }
      return TypePtr();
    }
    
    //-------------------------------------------------------------------------
    bool IIDLTypes::Context::hasModifier(Modifiers modifier) const
    {
      auto found = mModifiers.find(toString(modifier));
      if (found == mModifiers.end()) return false;
      return true;
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::Context::getModifierValue(
                                                Modifiers modifier,
                                                size_t index
                                                ) const
    {
      auto found = mModifiers.find(toString(modifier));
      if (found == mModifiers.end()) return String();
     
      auto &values = (*found).second;
      auto iter = values.begin();
      for (size_t pos = 0; pos <= index && (iter != values.end()); ++pos, ++iter) {
        if (pos != index) continue;
        return (*iter);
      }
      
      return String();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::getModifierValues(
                                               Modifiers modifier,
                                               StringList &outValues
                                               ) const
    {
      auto found = mModifiers.find(toString(modifier));
      if (found == mModifiers.end()) return;
      
      outValues = (*found).second;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::clearModifier(Modifiers modifier)
    {
      auto found = mModifiers.find(toString(modifier));
      if (found == mModifiers.end()) return;

      mModifiers.erase(found);
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::setModifier(Modifiers modifier)
    {
      StringList values;
      setModifier(modifier, values);
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::setModifier(
                                         Modifiers modifier,
                                         const String &value
                                         )
    {
      StringList values;
      values.push_back(value);
      setModifier(modifier, values);
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::setModifier(
                                         Modifiers modifier,
                                         const StringList &values
                                         )
    {
      clearModifier(modifier);
      
      if (values.size() < 1) {
        mModifiers[toString(modifier)] = StringList();
        return;
      }

      mModifiers[toString(modifier)] = values;
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::Context::aliasLookup(const String &value)
    {
      auto project = getProject();
      if (!project) return value;
      return project->aliasLookup(value);
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::init()
    {
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Context::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      mName = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Context::write(ElementPtr &rootEl) const
    {
      if (mName.hasData()) {
        rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("name", mName));
      }

      if (mDocumentation) {
        rootEl->adoptAsLastChild(mDocumentation->clone());
      }
      if (mModifiers.size() > 0) {
        auto modifiersEl = Element::create("modifiers");
        for (auto iter = mModifiers.begin(); iter != mModifiers.end(); ++iter) {
          auto name = (*iter).first;
          auto values = (*iter).second;
          auto modifierEl = Element::create("modifier");

          modifierEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("name", name));
          if (values.size() > 0) {
            auto valuesEl = Element::create("values");
            for (auto iterValues = values.begin(); iterValues != values.end(); ++iterValues)
            {
              auto value = (*iterValues);
              modifierEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("value", value));
            }
            modifierEl->adoptAsLastChild(valuesEl);
          }
          modifiersEl->adoptAsLastChild(modifierEl);
        }
        rootEl->adoptAsLastChild(modifiersEl);
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Context::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      if (!mName.hasData()) {
        mName = UseHelper::getElementTextAndDecode(rootEl->findLastChildElement("name"));
      }

      auto docEl = rootEl->findFirstChildElement("documentation");
      if (docEl) {
        mDocumentation = docEl->clone()->toElement();
      }
      auto modifiersEl = rootEl->findFirstChildElement("modifiers");
      if (modifiersEl) {
        auto modifierEl = modifiersEl->findFirstChildElement("modifier");
        while (modifierEl) {
          auto name = UseHelper::getElementTextAndDecode(modifierEl->findLastChildElement("name"));
          
          StringList values;
          auto valuesEl = modifierEl->findFirstChildElement("values");
          if (valuesEl) {
            auto valueEl = valuesEl->findFirstChildElement("value");
            while (valueEl) {
              auto value = UseHelper::getElementTextAndDecode(valueEl);
              values.push_back(value);
              valueEl = valuesEl->findNextSiblingElement("value");
            }
          }

          mModifiers[name] = values;
          modifierEl = modifierEl->findNextSiblingElement("modifier");
        }
      }
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Context::copyContentsFrom(ContextPtr originalContext)
    {
      if (!originalContext) return;
      
      auto currentParent = mContext.lock();
      if (!currentParent) {
        mContext = originalContext->mContext.lock();
      }

      mName = originalContext->mName;
      mDocumentation = originalContext->mDocumentation ? originalContext->mDocumentation->clone()->toElement() : ElementPtr();

      mModifiers.clear();
      for (auto iter = originalContext->mModifiers.begin(); iter != originalContext->mModifiers.end(); ++iter)
      {
        auto name = (*iter).first;
        auto values = (*iter).second;
        mModifiers[name] = values;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Project
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::Project::init()
    {
      Context::init();
      createBaseTypes();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Project::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      createBaseTypes();
      parse(rootEl);
    }

    //-------------------------------------------------------------------------
    IIDLTypes::ProjectPtr IIDLTypes::Project::create()
    {
      auto pThis(make_shared<Project>(make_private{}));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    IIDLTypes::ProjectPtr IIDLTypes::Project::create(const ElementPtr &el) throw (InvalidContent)
    {
      auto pThis(make_shared<Project>(make_private{}));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::Project::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "project";

      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);

      rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("name", mName));

      if (mAliases.size() > 0) {
        auto aliasesEl = Element::create("aliases");
        for (auto iter = mAliases.begin(); iter != mAliases.end(); ++iter) {
          auto aliasEl = Element::create("alias");
          aliasEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("in", (*iter).first));
          aliasEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("out", (*iter).second));
          aliasesEl->adoptAsLastChild(aliasEl);
        }
      }
      
      if (mDefinedExclusives.size() > 0) {
        auto exclusivesEl = Element::create("exclusives");
        for (auto iter = mDefinedExclusives.begin(); iter != mDefinedExclusives.end(); ++iter) {
          auto exclusiveStr = (*iter);
          exclusivesEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("exclusive", exclusiveStr));
        }
      }

      if (mGlobal) {
        rootEl->adoptAsLastChild(mGlobal->createElement());
      }

      return rootEl;
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Project::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;
      
      Context::parse(rootEl);

      createAliases(rootEl->findFirstChildElement("aliases"), mAliases);
      
      auto exclusivesEl = rootEl->findFirstChildElement("exclusives");
      if (exclusivesEl) {
        auto exclusiveEl = exclusivesEl->findFirstChildElement("exclusive");
        while (exclusiveEl) {
          auto exclusiveStr = UseHelper::getElementTextAndDecode(exclusiveEl);
          mDefinedExclusives.insert(exclusiveStr);
          exclusiveEl = exclusiveEl->findNextSiblingElement("exclusive");
        }
      }

      ElementPtr namespaceEl = rootEl->findFirstChildElement("namespace");
      if (namespaceEl) {
        if (!mGlobal) {
          mGlobal = Namespace::createForwards(toContext(), namespaceEl);
          mGlobal->parse(namespaceEl);
        }
      } else {
        if (!mGlobal) {
          mGlobal = Namespace::create(toContext());
        }
      }
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::Project::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update("project:");
      hasher->update(Context::hash());

      hasher->update(":list:aliases");
      for (auto iter = mAliases.begin(); iter != mAliases.end(); ++iter)
      {
        auto &aliasIn = (*iter).first;
        auto &aliasOut = (*iter).second;

        hasher->update(":next:");
        hasher->update(aliasIn);
        hasher->update(":");
        hasher->update(aliasOut);
      }
      
      hasher->update(":list:exclusives");
      for (auto iter = mDefinedExclusives.begin(); iter != mDefinedExclusives.end(); ++iter)
      {
        auto exclusiveStr = (*iter);
        hasher->update(":next:");
        hasher->update(exclusiveStr);
      }

      hasher->update(":global:");
      
      if (mGlobal) {
        hasher->update(mGlobal->hash());
      }

      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::Project::findType(
                                                    const String &pathStr,
                                                    const String &typeName,
                                                    const FindTypeOptions &options
                                                    ) const
    {
      if (mGlobal) {
        return mGlobal->findType(pathStr, typeName, options);
      }

      auto found = mBasicTypes.find(typeName);
      if (found == mBasicTypes.end()) return TypePtr();

      return (*found).second;
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Project::resolveTypedefs() throw (InvalidContent)
    {
      if (mGlobal) {
        mGlobal->resolveTypedefs();
      }

      for (auto iter = mBasicTypes.begin(); iter != mBasicTypes.end(); ++iter) {
        auto basicType = (*iter).second;
        basicType->resolveTypedefs();
      }
    }

    //-------------------------------------------------------------------------
    bool IIDLTypes::Project::fixTemplateHashMapping()
    {
      bool didFix = false;
      if (mGlobal) {
        do {
          didFix = mGlobal->fixTemplateHashMapping();
        } while (didFix);
      }
      return didFix;
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::Project::aliasLookup(const String &value)
    {
      return IIDLTypes::aliasLookup(mAliases, value);
    }

    //-------------------------------------------------------------------------
    IIDLTypes::BasicTypePtr IIDLTypes::Project::findBasicType(IEventingTypes::PredefinedTypedefs findType) const
    {
      for (auto iter = mBasicTypes.begin(); iter != mBasicTypes.end(); ++iter)
      {
        auto checkType = (*iter).second;
        if (findType == checkType->mBaseType) return checkType;
      }
      return BasicTypePtr();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Project::createBaseTypes()
    {
      auto context = toContext();

      for (IEventingTypes::PredefinedTypedefs index = IEventingTypes::PredefinedTypedef_First; index <= IEventingTypes::PredefinedTypedef_Last; index = static_cast<IEventingTypes::PredefinedTypedefs>(static_cast<std::underlying_type<IEventingTypes::PredefinedTypedefs>::type>(index) + 1)) {
        auto type = BasicType::create(context);
        type->mBaseType = index;
        type->mName = IEventingTypes::toString(index);
        mBasicTypes[type->getMappingName()] = type;
      }
    }
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Namespace
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::Namespace::init()
    {
      Context::init();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Namespace::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      
      if (!rootEl) return;
      
      auto context = toContext();

      // scan for other nested namespaces, enums, structs and typedefs
      createNamespaceForwards(context, rootEl->findFirstChildElement("namespaces"), mNamespaces);
      createEnumForwards(context, rootEl->findFirstChildElement("enums"), mEnums);
      createStructForwards(context, rootEl->findFirstChildElement("structs"), mStructs);
      createTypedefForwards(context, rootEl->findFirstChildElement("typedefs"), mTypedefs);
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::NamespacePtr IIDLTypes::Namespace::create(ContextPtr context)
    {
      auto pThis(make_shared<Namespace>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::NamespacePtr IIDLTypes::Namespace::createForwards(
                                                                 ContextPtr context,
                                                                 const ElementPtr &el
                                                                 ) throw (InvalidContent)
    {
      auto pThis(make_shared<Namespace>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::Namespace::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "namespace";

      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);

      if (mNamespaces.size() > 0) {
        auto namespacesEl = Element::create("namespaces");
        
        for (auto iter = mNamespaces.begin(); iter != mNamespaces.end(); ++iter)
        {
          auto namespaceObj = (*iter).second;
          namespacesEl->adoptAsLastChild(namespaceObj->createElement());
        }
        rootEl->adoptAsLastChild(namespacesEl);
      }
      
      if (mEnums.size() > 0) {
        auto enumsEl = Element::create("enums");
        
        for (auto iter = mEnums.begin(); iter != mEnums.end(); ++iter)
        {
          auto enumObj = (*iter).second;
          enumsEl->adoptAsLastChild(enumObj->createElement());
        }
        rootEl->adoptAsLastChild(enumsEl);
      }
      
      if (mStructs.size() > 0) {
        auto structsEl = Element::create("structs");
        
        for (auto iter = mStructs.begin(); iter != mStructs.end(); ++iter)
        {
          auto structObj = (*iter).second;
          structsEl->adoptAsLastChild(structObj->createElement());
        }
        rootEl->adoptAsLastChild(structsEl);
      }
      
      if (mTypedefs.size() > 0) {
        auto typedefsEl = Element::create("typedefs");
        
        for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter)
        {
          auto typedefObj = (*iter).second;
          typedefsEl->adoptAsLastChild(typedefObj->createElement());
        }
        rootEl->adoptAsLastChild(typedefsEl);
      }

      return rootEl;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Namespace::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;
      
      auto context = toContext();

      Context::parse(rootEl);

      // scan for other nested namespaces, enums, structs and typedefs
      parseNamespaces(context, rootEl->findFirstChildElement("namespaces"), mNamespaces);
      parseEnums(context, rootEl->findFirstChildElement("enums"), mEnums);
      parseStructs(context, rootEl->findFirstChildElement("structs"), mStructs);
      parseTypedefs(context, rootEl->findFirstChildElement("typedefs"), mTypedefs);
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::Namespace::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("namespace:");
      hasher->update(Context::hash());

      hasher->update(":namespaces:");
      for (auto iter = mNamespaces.begin(); iter != mNamespaces.end(); ++iter)
      {
        auto namespaceObj = (*iter).second;
        hasher->update(namespaceObj->hash());
        hasher->update(":next:");
      }

      hasher->update(":enums:");
      for (auto iter = mEnums.begin(); iter != mEnums.end(); ++iter)
      {
        auto enumObj = (*iter).second;
        hasher->update(enumObj->hash());
        hasher->update(":next:");
      }
      
      hasher->update(":structs:");
      for (auto iter = mStructs.begin(); iter != mStructs.end(); ++iter)
      {
        auto structObj = (*iter).second;
        hasher->update(structObj->hash());
        hasher->update(":next:");
      }
      
      hasher->update(":typedefs:");
      for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter)
      {
        auto typedefObj = (*iter).second;
        hasher->update(typedefObj->hash());
        hasher->update(":next:");
      }

      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::Namespace::findType(
                                                      const String &pathStr,
                                                      const String &typeName,
                                                      const FindTypeOptions &options
                                                      ) const
    {
      String checkPath = pathStr;

      if ("::" == checkPath.substr(0, 2)) {
        auto parent = getParent();
        if (!parent) return TypePtr();

        if (!parent->toProject()) {
          if (!options.mSearchParents) return TypePtr();
          return parent->findType(pathStr, typeName, options);
        }
        
        // strip the global namespace if at the global namespace
        checkPath = pathStr.substr(2);
      }
      
      if (checkPath.hasData()) {
        UseHelper::SplitMap splitPaths;
        UseHelper::split(pathStr, splitPaths, "::");
        
        if (splitPaths.size() < 1) return TypePtr();
        
        String searchPath = splitPaths[0];

        splitPaths.erase(splitPaths.begin());

        checkPath = UseHelper::combine(splitPaths, "::");

        {
          auto found = mNamespaces.find(searchPath);
          if (found != mNamespaces.end()) {
            auto namespaceObj = (*found).second;
            return namespaceObj->findType(checkPath, typeName, options);
          }
        }
        
        {
          auto found = mStructs.find(searchPath);
          if (found != mStructs.end()) {
            auto structObj = (*found).second;
            return structObj->findType(checkPath, typeName, options);
          }
        }
        
        auto parent = getParent();
        if (parent) {
          if (!parent->toProject()) {
            return parent->findType(pathStr, typeName, options);
          }
        }

        // type not found
        return TypePtr();
      }

      {
        auto found = mEnums.find(typeName);
        if (found != mEnums.end()) return (*found).second;
      }
      
      {
        auto found = mStructs.find(typeName);
        if (found != mStructs.end()) return (*found).second;
      }

      {
        auto found = mTypedefs.find(typeName);
        if (found != mTypedefs.end()) return (*found).second;
      }

      if (options.mSearchParents) {
        auto parent = getParent();
        if (parent) {
          auto project = parent->toProject();
          if (project) {
            auto found = project->mBasicTypes.find(typeName);
            if (found == project->mBasicTypes.end()) return TypePtr();

            return (*found).second;
          }

          return parent->findType(pathStr, typeName, options);
        }
      }

      return TypePtr();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Namespace::resolveTypedefs() throw (InvalidContent)
    {
      for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }
      for (auto iter = mNamespaces.begin(); iter != mNamespaces.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }
      for (auto iter = mEnums.begin(); iter != mEnums.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }
      for (auto iter = mStructs.begin(); iter != mStructs.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }
    }

    //-------------------------------------------------------------------------
    bool IIDLTypes::Namespace::fixTemplateHashMapping()
    {
      bool didFix = false;
      for (auto iter_doNotUse = mNamespaces.begin(); iter_doNotUse != mNamespaces.end(); ) {
        auto current = iter_doNotUse;
        ++iter_doNotUse;

        auto obj = (*current).second;
        if (obj->fixTemplateHashMapping()) didFix = true;
      }
      for (auto iter_doNotUse = mStructs.begin(); iter_doNotUse != mStructs.end(); ) {
        auto current = iter_doNotUse;
        ++iter_doNotUse;

        auto obj = (*current).second;
        if (obj->fixTemplateHashMapping()) didFix = true;
      }
      return didFix;
    }

    //-------------------------------------------------------------------------
    IIDLTypes::NamespacePtr IIDLTypes::Namespace::findNamespace(const String &nameWithPath) const
    {
      String path;
      String name;
      
      if (!internal::splitToNamePath(nameWithPath, path, name)) return NamespacePtr();

      return findNamespace(path, name);
    }

    //-------------------------------------------------------------------------
    IIDLTypes::NamespacePtr IIDLTypes::Namespace::findNamespace(
                                                                const String &pathStr,
                                                                const String &name
                                                                ) const
    {
      NamespacePtr parentNamespace;
      
      String checkPath = pathStr;
      
      if ("::" == checkPath.substr(0, 2)) {
        auto parent = getParent();
        if (!parent) return NamespacePtr();
        
        parentNamespace = parent->toNamespace();

        if (parentNamespace) {
          return parentNamespace->findNamespace(pathStr, name);
        }

        // strip the global namespace if at the global namespace
        checkPath = pathStr.substr(2);
      }
      
      if (pathStr.hasData()) {
        UseHelper::SplitMap splitPaths;
        UseHelper::split(pathStr, splitPaths, "::");

        if (splitPaths.size() < 1) return NamespacePtr();
        
        String searchPath = splitPaths[0];
        
        splitPaths.erase(splitPaths.begin());
        
        checkPath = UseHelper::combine(splitPaths, "::");
        
        {
          auto found = mNamespaces.find(searchPath);
          if (found != mNamespaces.end()) {
            auto namespaceObj = (*found).second;
            return namespaceObj->findNamespace(checkPath, name);
          }
        }

        auto parent = getParent();
        if (!parent) return NamespacePtr();
        
        parentNamespace = parent->toNamespace();
        if (!parentNamespace) return NamespacePtr();

        return parentNamespace->findNamespace(pathStr, name);
      }
      
      {
        auto found = mNamespaces.find(name);
        if (found != mNamespaces.end()) return (*found).second;
      }

      {
        auto parent = getParent();
        if (!parent) return NamespacePtr();
        
        parentNamespace = parent->toNamespace();
        if (!parentNamespace) return NamespacePtr();

      }

      return parentNamespace->findNamespace(pathStr, name);
    }

    //-------------------------------------------------------------------------
    bool IIDLTypes::Namespace::isGlobal() const
    {
      if (mName.hasData()) return false;
      auto parent = getParent();
      if (!parent) return true;
      auto namespaceObj = parent->toNamespace();
      if (namespaceObj) return false;
      return true;
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::createNamespaceForwards(
                                            ContextPtr context,
                                            ElementPtr namespacesEl,
                                            NamespaceMap &outNamespaces
                                            ) throw (InvalidContent)
    {
      if (!namespacesEl) return;

      auto namespaceEl = namespacesEl->findFirstChildElement("namespace");

      while (namespaceEl) {
        auto namespaceObj = Namespace::createForwards(context, namespaceEl);
        outNamespaces[namespaceObj->getMappingName()] = namespaceObj;

        namespaceEl = namespaceEl->findNextSiblingElement("namespace");
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::parseNamespaces(
                                    ContextPtr context,
                                    ElementPtr namespacesEl,
                                    NamespaceMap &ioNamespaces
                                    ) throw (InvalidContent)
    {
      if (!namespacesEl) return;
      
      auto namespaceEl = namespacesEl->findFirstChildElement("namespace");

      while (namespaceEl) {
        auto name = context->aliasLookup(UseHelper::getElementTextAndDecode(namespaceEl->findFirstChildElement("name")));
        
        NamespacePtr namespaceObj;

        auto found = ioNamespaces.find(name);
        if (found == ioNamespaces.end()) {
          namespaceObj = Namespace::createForwards(context, namespaceEl);
          ioNamespaces[namespaceObj->getMappingName()] = namespaceObj;
        } else {
          namespaceObj = (*found).second;
        }
        namespaceObj->parse(namespaceEl);

        namespaceEl = namespaceEl->findNextSiblingElement("namespace");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Type
    #pragma mark

    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::Type::createReferencedType(
                                                             ContextPtr context,
                                                             ElementPtr parentEl
                                                             ) throw (InvalidContent)
    {
      FindTypeOptions options;

      {
        auto typedefEl = parentEl->findFirstChildElement("typedef");
        if (typedefEl) {
          auto typedefObj = TypedefType::createForwards(context, typedefEl);
          typedefObj->parse(typedefEl);
          return typedefObj;
        }
      }

      {
        auto basicEl = parentEl->findFirstChildElement("basic");
        if (basicEl) {
          auto baseStr = context->aliasLookup(UseHelper::getElementTextAndDecode(basicEl->findFirstChildElement("base")));
          auto foundType = context->findType(String(), baseStr, options);
          if (!foundType) {
            ZS_THROW_CUSTOM(InvalidContent, String("Basic type was not found: ") + baseStr);
          }
          return foundType;
        }
      }

      {
        auto referenceEl = parentEl->findFirstChildElement("reference");
        if (referenceEl) {
          auto pathStr = context->aliasLookup(UseHelper::getElementTextAndDecode(referenceEl->findFirstChildElement("path")));
          auto baseStr = context->aliasLookup(UseHelper::getElementTextAndDecode(referenceEl->findFirstChildElement("base")));

          auto foundType = context->findType(pathStr, baseStr, options);
          if (!foundType) {
            ZS_THROW_CUSTOM(InvalidContent, String("Referenced type was not found, path=") + pathStr + ", base=" + baseStr);
          }
          return foundType;
        }
      }

      ZS_THROW_CUSTOM(InvalidContent, "Referenced type was not found for context, context=" + context->getMappingName());
      return TypePtr();
    }

    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::Type::createReferenceTypeElement() const
    {
      {
        auto typedefType = toTypedefType();
        if (typedefType) return typedefType->createElement();
      }

      {
        auto basicType = toBasicType();
        if (basicType) {
          auto typeEl = Element::create("basic");
          typeEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("base", IEventingTypes::toString(basicType->mBaseType)));
          return typeEl;
        }
      }

      {
        auto typeEl = Element::create("reference");
        auto pathStr = getPath();
        typeEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("path", pathStr));
        typeEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("base", getMappingName()));
        return typeEl;
      }

      ZS_THROW_INVALID_ASSUMPTION("Unexpected type reference found: " + getMappingName());
      return ElementPtr();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::BasicType
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::BasicType::init()
    {
      Context::init();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::BasicType::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      
      if (!rootEl) return;

      Context::parse(rootEl);

      String baseTypeStr = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("type")));
      
      if (baseTypeStr.hasData()) {
        try {
          mBaseType = IEventingTypes::toPredefinedTypedef(baseTypeStr);
        } catch (const InvalidArgument &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Invalid base type") + baseTypeStr);
        }
      }

      if (mName.isEmpty()) {
        mName = IEventingTypes::toString(mBaseType);
      }
    }

    //-------------------------------------------------------------------------
    IIDLTypes::BasicTypePtr IIDLTypes::BasicType::create(ContextPtr context)
    {
      auto pThis(make_shared<BasicType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::BasicType::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "basic";

      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);

      rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("type", IEventingTypes::toString(mBaseType)));

      return rootEl;
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::BasicType::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("basic:");
      hasher->update(Context::hash());
      hasher->update(":");
      hasher->update(IEventingTypes::toString(mBaseType));
      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }
    

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::EnumType
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::EnumType::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::EnumType::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      
      if (!rootEl) return;
    }

    //-------------------------------------------------------------------------
    IIDLTypes::EnumTypePtr IIDLTypes::EnumType::create(ContextPtr context)
    {
      auto pThis(make_shared<EnumType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::EnumTypePtr IIDLTypes::EnumType::createForwards(
                                                               ContextPtr context,
                                                               const ElementPtr &el
                                                               ) throw (InvalidContent)
    {
      auto pThis(make_shared<EnumType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::EnumType::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "enum";
      
      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);
      
      rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("type", IEventingTypes::toString(mBaseType)));
      
      if (mValues.size() > 0) {
        auto valuesEl = Element::create("values");
        for (auto iter = mValues.begin(); iter != mValues.end(); ++iter) {
          auto value = (*iter);
          valuesEl->adoptAsFirstChild(value->createElement());
        }

        rootEl->adoptAsLastChild(valuesEl);
      }

      return rootEl;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::EnumType::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      Context::parse(rootEl);

      String baseTypeStr = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("type")));

      if (baseTypeStr.hasData()) {
        try {
          mBaseType = IEventingTypes::toPredefinedTypedef(baseTypeStr);
        } catch (const InvalidArgument &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Invalid base type") + baseTypeStr);
        }
      }

      IIDLTypes::createEnumValues(toContext(), rootEl->findFirstChildElement("values"), mValues);
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::EnumType::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update("enum:");
      hasher->update(Context::hash());
      
      hasher->update(":values:");
      for (auto iter = mValues.begin(); iter != mValues.end(); ++iter)
      {
        auto value = (*iter);
        hasher->update(value->hash());
        hasher->update(":next:");
      }

      hasher->update(":end");

      return hasher->finalizeAsString();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::createEnumForwards(
                                       ContextPtr context,
                                       ElementPtr enumsEl,
                                       EnumMap &outEnums
                                       ) throw (InvalidContent)
    {
      if (!enumsEl) return;

      auto enumEl = enumsEl->findFirstChildElement("enum");
      while (enumEl) {
        auto enumObj = EnumType::createForwards(context, enumEl);
        outEnums[enumObj->getMappingName()] = enumObj;
        enumEl = enumEl->findNextSiblingElement("enum");
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::parseEnums(
                               ContextPtr context,
                               ElementPtr enumsEl,
                               EnumMap &ioEnums
                               ) throw (InvalidContent)
    {
      if (!enumsEl) return;
      
      auto enumEl = enumsEl->findFirstChildElement("enum");
      while (enumEl) {
        auto name = context->aliasLookup(UseHelper::getElementTextAndDecode(enumEl->findFirstChildElement("name")));
        
        EnumTypePtr enumObj;
        
        auto found = ioEnums.find(name);
        if (found == ioEnums.end()) {
          enumObj = EnumType::createForwards(context, enumEl);
          ioEnums[enumObj->getMappingName()] = enumObj;
        } else {
          enumObj = (*found).second;
        }
        enumObj->parse(enumEl);

        enumEl = enumEl->findNextSiblingElement("enum");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::EnumTypeValue
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::EnumTypeValue::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::EnumTypeValue::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      
      if (!rootEl) return;
    }

    //-------------------------------------------------------------------------
    IIDLTypes::EnumTypeValuePtr IIDLTypes::EnumTypeValue::create(ContextPtr context)
    {
      auto pThis(make_shared<EnumTypeValue>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::EnumTypeValuePtr IIDLTypes::EnumTypeValue::create(
                                                                 ContextPtr context,
                                                                 const ElementPtr &el
                                                                 ) throw (InvalidContent)
    {
      auto pThis(make_shared<EnumTypeValue>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::EnumTypeValue::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "value";
      
      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);

      rootEl->adoptAsFirstChild(UseHelper::createElementWithTextAndJSONEncode("value", mValue));

      return rootEl;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::EnumTypeValue::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      Context::parse(rootEl);

      mValue = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("value")));
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::EnumTypeValue::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update("enumValue:");
      hasher->update(Context::hash());
      
      hasher->update(":");
      hasher->update(mValue);
      hasher->update(":end");

      return hasher->finalizeAsString();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::createEnumValues(
                                     ContextPtr context,
                                     ElementPtr enumValuesEl,
                                     EnumTypeValueList &outEnumValues
                                     ) throw (InvalidContent)
    {
      if (!enumValuesEl) return;

      auto valueEl = enumValuesEl->findFirstChildElement("value");
      while (valueEl) {
        auto enumObj = EnumTypeValue ::create(context, valueEl);
        outEnumValues.push_back(enumObj);
        valueEl = valueEl->findNextSiblingElement("value");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::TypedefType
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::TypedefType::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::TypedefType::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      Context::init(rootEl);
    }

    //-------------------------------------------------------------------------
    IIDLTypes::TypedefTypePtr IIDLTypes::TypedefType::create(ContextPtr context)
    {
      auto pThis(make_shared<TypedefType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::TypedefTypePtr IIDLTypes::TypedefType::createForwards(
                                                                             ContextPtr context,
                                                                             const ElementPtr &el
                                                                             ) throw (InvalidContent)
    {
      auto pThis(make_shared<TypedefType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::TypedefType::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "typedef";
      
      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);

      auto originalType = mOriginalType.lock();

      if (originalType) {

        // attempt to figure out type of original
        {
          auto basicType = originalType->toBasicType();
          if (basicType) {
            rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("base", IEventingTypes::toString(basicType->mBaseType)));
            goto done;
          }
        }

        {
          auto enumType = originalType->toEnumType();
          if (enumType) goto insert_path_and_base;
        }

        {
          auto structType = originalType->toStruct();
          if (structType) goto insert_path_and_base;
        }

        {
          auto genericType = originalType->toGenericType();
          if (genericType) goto insert_path_and_base;
        }

        {
          auto templatedStructType = originalType->toTemplatedStructType();
          if (templatedStructType) goto insert_path_and_base;
        }

        ZS_THROW_BAD_STATE("typedef is not resolved to a proper type");

      insert_path_and_base:
        {
          auto pathStr = originalType->getPath();
          rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("path", pathStr));
          rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("base", originalType->getMappingName()));
          goto done;
        }

      done:
        {
        }
      } else {
        // must point to something if not generic template typedef
        ZS_THROW_BAD_STATE("typedef does not map to an original type");
      }

      return rootEl;
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::TypedefType::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      Context::parse(rootEl);

      String pathStr = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("path")));
      String baseStr = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("base")));

      FindTypeOptions options;
      auto foundType = findType(pathStr, baseStr, options);

      if (!foundType) {
        ZS_THROW_CUSTOM(InvalidContent, String("Type was not found, path=") + pathStr + ", base=" + baseStr);
      }

      mOriginalType = foundType;
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::TypedefType::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("typedef:");
      hasher->update(Context::hash());

      auto original = mOriginalType.lock();
      auto name = original->getMappingName();
      auto path = original->getPath();

      hasher->update(":");
      hasher->update(path);
      hasher->update(":");
      hasher->update(name);

      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::TypedefType::resolveTypedefs() throw (InvalidContent)
    {
      auto originalType = mOriginalType.lock();

      TypePtr baseType;
      TypedefTypeList linkedTypedefs;
      TypedefTypeSet typedefsSet;

      while (originalType) {
        auto typedefType = originalType->toTypedefType();
        if (typedefType) {
          if (typedefsSet.find(typedefType) != typedefsSet.end()) {
            ZS_THROW_CUSTOM(InvalidContent, String("Typedef is circular: ") + typedefType->getMappingName());
          }

          typedefsSet.insert(typedefType);
          linkedTypedefs.push_front(typedefType);
          
          originalType = typedefType->mOriginalType.lock();
          continue;
        }

        baseType = originalType;
        break;
      }

      if (linkedTypedefs.size() < 1) return;  // nothing to do

      if (!baseType) {
        ZS_THROW_CUSTOM(InvalidContent, String("No linked base typedef found: ") + getMappingName());
      }

      for (auto iter = linkedTypedefs.begin(); iter != linkedTypedefs.end(); ++iter) {
        auto typedefObj = (*iter);
        
        for (auto iter = typedefObj->mModifiers.begin(); iter != typedefObj->mModifiers.end(); ++iter)
        {
          auto name = (*iter).first;
          auto values = (*iter).second;
          
          if (mModifiers.end() != mModifiers.find(name)) {
            ZS_THROW_CUSTOM(InvalidContent, String("Not allowed to combine these type modifiers, modifier=") + name);
          }
          mModifiers[name] = values;
        }
      }

      mOriginalType = baseType;
    }

    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::TypedefType::getOriginalType() const
    {
      auto originalType = mOriginalType.lock();
      if (originalType) return originalType;

      return toType();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::createTypedefForwards(
                                          ContextPtr context,
                                          ElementPtr typedefsEl,
                                          TypedefTypeMap &outTypedefs
                                          ) throw (InvalidContent)
    {
      if (!typedefsEl) return;
      
      auto typedefEl = typedefsEl->findFirstChildElement("typedef");
      while (typedefEl) {
        auto typedefObj = TypedefType::createForwards(context, typedefsEl);
        outTypedefs[typedefObj->getMappingName()] = typedefObj;
        typedefEl = typedefEl->findNextSiblingElement("typedef");
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::parseTypedefs(
                                  ContextPtr context,
                                  ElementPtr typedefsEl,
                                  TypedefTypeMap &ioTypedefs
                                  ) throw (InvalidContent)
    {
      if (!typedefsEl) return;

      auto typedefEl = typedefsEl->findFirstChildElement("typedef");
      while (typedefEl) {
        auto name = context->aliasLookup(UseHelper::getElementTextAndDecode(typedefEl->findFirstChildElement("name")));
        
        TypedefTypePtr typedefObj;
        
        auto found = ioTypedefs.find(name);
        if (found == ioTypedefs.end()) {
          typedefObj = TypedefType::createForwards(context, typedefEl);
          ioTypedefs[typedefObj->getMappingName()] = typedefObj;
        } else {
          typedefObj = (*found).second;
        }

        typedefObj->parse(typedefEl);

        typedefEl = typedefEl->findNextSiblingElement("typedef");
      }
    }
    

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Struct
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::Struct::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Struct::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      
      if (!rootEl) return;

      auto context = toContext();

      createEnumForwards(context, rootEl->findFirstChildElement("enums"), mEnums);
      createStructForwards(context, rootEl->findFirstChildElement("structs"), mStructs);
      createTypedefForwards(context, rootEl->findFirstChildElement("typedefs"), mTypedefs);
      createGenericForwards(context, rootEl->findFirstChildElement("generics"), mGenerics);
      createTemplatedStructTypeForwards(context, rootEl->findFirstChildElement("templatedStructs"), mTemplatedStructs);
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::StructPtr IIDLTypes::Struct::create(ContextPtr context)
    {
      auto pThis(make_shared<Struct>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::StructPtr IIDLTypes::Struct::createForwards(
                                                                   ContextPtr context,
                                                                   const ElementPtr &el
                                                                   ) throw (InvalidContent)
    {
      auto pThis(make_shared<Struct>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }

    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::Struct::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "struct";
      
      ElementPtr rootEl = Element::create(objectName);

      Context::write(rootEl);

      if (mGenerics.size() > 0) {
        ElementPtr genericsEl = Element::create("generics");

        for (auto iter = mGenerics.begin(); iter != mGenerics.end(); ++iter) {
          auto genericType = (*iter);

          genericsEl->adoptAsLastChild(genericType->createElement());
        }
      }

      if (mGenericDefaultTypes.size() > 0) {
        ElementPtr templateDefaultsEl = Element::create("genericDefaults");
        
        for (auto iter = mGenericDefaultTypes.begin(); iter != mGenericDefaultTypes.end(); ++iter) {
          auto templateType = (*iter);

          ElementPtr templateEl = Element::create("genericDefault");

          if (!templateType) {
            templateEl->adoptAsLastChild(UseHelper::createElementWithNumber("value", "null"));
          } else {
            templateEl->adoptAsLastChild(templateType->createReferenceTypeElement());
          }
          templateDefaultsEl->adoptAsLastChild(templateEl);
        }
      }

      if (mGenerics.size() > 0) {
        ElementPtr templateStructsEl = Element::create("templatedStructs");
        
        for (auto iter = mTemplatedStructs.begin(); iter != mTemplatedStructs.end(); ++iter) {
          auto templatedStruct = (*iter).second;
          templateStructsEl->adoptAsLastChild(templatedStruct->createElement());
        }
      }

      if (mIsARelationships.size() > 0) {
        ElementPtr relationshipsEl = Element::create("relationships");
        for (auto iter = mIsARelationships.begin(); iter != mIsARelationships.end(); ++iter) {
          auto baseType = (*iter).second;

          auto relationshipEl = Element::create("relationsip");

          relationshipEl->adoptAsLastChild(baseType->createReferenceTypeElement());
          relationshipsEl->adoptAsLastChild(relationshipEl);
        }
        rootEl->adoptAsLastChild(relationshipsEl);
      }

      if (mEnums.size() > 0) {
        auto enumsEl = Element::create("enums");

        for (auto iter = mEnums.begin(); iter != mEnums.end(); ++iter)
        {
          auto enumObj = (*iter).second;
          enumsEl->adoptAsLastChild(enumObj->createElement());
        }
        rootEl->adoptAsLastChild(enumsEl);
      }

      if (mStructs.size() > 0) {
        auto structsEl = Element::create("structs");

        for (auto iter = mStructs.begin(); iter != mStructs.end(); ++iter)
        {
          auto structObj = (*iter).second;
          structsEl->adoptAsLastChild(structObj->createElement());
        }
        rootEl->adoptAsLastChild(structsEl);
      }

      if (mTypedefs.size() > 0) {
        auto typedefsEl = Element::create("typedefs");

        for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter)
        {
          auto typedefObj = (*iter).second;
          typedefsEl->adoptAsLastChild(typedefObj->createElement());
        }
        rootEl->adoptAsLastChild(typedefsEl);
      }

      if (mProperties.size() > 0) {
        auto propertiesEl = Element::create("properties");

        for (auto iter = mProperties.begin(); iter != mProperties.end(); ++iter)
        {
          auto propertyObj = (*iter);
          propertiesEl->adoptAsLastChild(propertyObj->createElement());
        }
        rootEl->adoptAsLastChild(propertiesEl);
      }

      if (mMethods.size() > 0) {
        auto methodsEl = Element::create("methods");

        for (auto iter = mMethods.begin(); iter != mMethods.end(); ++iter)
        {
          auto methodObj = (*iter);
          methodsEl->adoptAsLastChild(methodObj->createElement());
        }
        rootEl->adoptAsLastChild(methodsEl);
      }

      return rootEl;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Struct::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;

      Context::parse(rootEl);

      auto context = toContext();

      parseGenerics(context, rootEl->findFirstChildElement("generics"), mGenerics);
      parseTemplatedStructTypes(context, rootEl->findFirstChildElement("templatedStructs"), mTemplatedStructs);

      {
        auto templatesEl = rootEl->findFirstChildElement("templateDefaults");
        if (templatesEl) {
          auto templateDefaultEl = templatesEl->findFirstChildElement("templateDefault");
          
          while (templateDefaultEl) {
            auto valueEl = templateDefaultEl->findFirstChildElement("value");
            if (valueEl) {
              String value = UseHelper::getElementTextAndDecode(valueEl);
              if (value != "null") {
                ZS_THROW_CUSTOM(InvalidContent, String("Template default value contains data other than expected null: ") + value);
              }
              mGenericDefaultTypes.push_back(TypePtr());
            } else {
              mGenericDefaultTypes.push_back(createReferencedType(context, templateDefaultEl));
            }
            templateDefaultEl = templateDefaultEl->findNextSiblingElement("templateDefault");
          }
        }
      }

      {
        auto relationshipsEl = rootEl->findFirstChildElement("relationships");
        if (relationshipsEl) {

          FindTypeOptions options;

          auto relationshipEl = relationshipsEl->findFirstChildElement("relationship");
          while (relationshipEl) {

            {
              auto pathStr = aliasLookup(UseHelper::getElementTextAndDecode(relationshipEl->findFirstChildElement("path")));
              auto baseStr = aliasLookup(UseHelper::getElementTextAndDecode(relationshipEl->findFirstChildElement("base")));

              auto foundType = Type::createReferencedType(context, relationshipEl);
              if (!foundType) {
                ZS_THROW_CUSTOM(InvalidContent, String("Relationship struct type was not found, path=") + pathStr + ", base=" + baseStr);
              }

              mIsARelationships[foundType->getPathName()] = foundType;
            }

            relationshipEl = relationshipEl->findNextSiblingElement("relationship");
          }
        }
      }

      // scan for other nested namespaces, enums, structs and typedefs
      parseEnums(context, rootEl->findFirstChildElement("enums"), mEnums);
      parseStructs(context, rootEl->findFirstChildElement("structs"), mStructs);
      parseTypedefs(context, rootEl->findFirstChildElement("typedefs"), mTypedefs);

      createProperties(context, rootEl->findFirstChildElement("properties"), mProperties);
      createMethods(context, rootEl->findFirstChildElement("properties"), mMethods);
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::Struct::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("struct:");
      hasher->update(Context::hash());

      hasher->update(":generics:");
      for (auto iter = mGenerics.begin(); iter != mGenerics.end(); ++iter) {
        auto genericObj = (*iter);

        hasher->update(genericObj->hash());
      }

      hasher->update(":generics:defaults:");
      for (auto iter = mGenericDefaultTypes.begin(); iter != mGenericDefaultTypes.end(); ++iter) {
        auto templateObj = (*iter);

        if (templateObj) {
          String pathStr = templateObj->getPath();
          String baseStr = templateObj->getMappingName();
          hasher->update(pathStr);
          hasher->update(":");
          hasher->update(baseStr);
        }
        hasher->update(":next:");
      }

      hasher->update(":templatedStructs:");
      for (auto iter = mTemplatedStructs.begin(); iter != mTemplatedStructs.end(); ++iter) {
        auto templatedStruct = (*iter).second;

        hasher->update(templatedStruct->hash());
      }

      hasher->update(":relationships:");
      for (auto iter = mIsARelationships.begin(); iter != mIsARelationships.end(); ++iter) {
        auto typeObj = (*iter).second;

        String pathStr = typeObj->getPath();
        String baseStr = typeObj->getMappingName();

        hasher->update(":");
        hasher->update(pathStr);
        hasher->update(":");
        hasher->update(baseStr);
        hasher->update(":next:");
      }

      hasher->update(":enums:");
      for (auto iter = mEnums.begin(); iter != mEnums.end(); ++iter)
      {
        auto enumObj = (*iter).second;
        hasher->update(enumObj->hash());
        hasher->update(":next:");
      }

      hasher->update(":structs:");
      for (auto iter = mStructs.begin(); iter != mStructs.end(); ++iter)
      {
        auto structObj = (*iter).second;
        hasher->update(structObj->hash());
        hasher->update(":next:");
      }

      hasher->update(":typedefs:");
      for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter)
      {
        auto typedefObj = (*iter).second;
        hasher->update(typedefObj->hash());
        hasher->update(":next:");
      }

      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    IIDLTypes::TypePtr IIDLTypes::Struct::findType(
                                                   const String &pathStr,
                                                   const String &typeName,
                                                   const FindTypeOptions &options
                                                   ) const
    {
      String checkPath = pathStr;
      
      if ("::" == checkPath.substr(0, 2)) {
        auto parent = getParent();
        if (!parent) return TypePtr();
        
        if (!parent->toProject()) {
          if (!options.mSearchParents) return TypePtr();
          return parent->findType(pathStr, typeName, options);
        }

        // strip the global namespace if at the global namespace
        checkPath = pathStr.substr(2);
      }
      
      if (pathStr.hasData()) {
        UseHelper::SplitMap splitPaths;
        UseHelper::split(pathStr, splitPaths, "::");
        
        if (splitPaths.size() < 1) return TypePtr();
        
        String searchPath = splitPaths[0];
        
        splitPaths.erase(splitPaths.begin());
        
        checkPath = UseHelper::combine(splitPaths, "::");

        {
          auto found = mStructs.find(searchPath);
          if (found != mStructs.end()) {
            auto structObj = (*found).second;
            return structObj->findType(checkPath, typeName, options);
          }
        }

        {
          FindTypeOptions baseOptions = options;
          baseOptions.mSearchParents = false;

          for (auto iter = mIsARelationships.begin(); iter != mIsARelationships.end(); ++iter)
          {
            auto baseType = (*iter).second;
            auto checkName = baseType->getMappingName();

            baseType->findType(pathStr, typeName, baseOptions);
            if (checkName == searchPath) {
              baseType->findType(checkPath, typeName, baseOptions);
            }
          }
        }

        if (options.mSearchParents) {
          auto parent = getParent();
          if (parent) return parent->findType(pathStr, typeName, options);
        }
        
        // type not found
        return TypePtr();
      }

      {
        auto found = mEnums.find(typeName);
        if (found != mEnums.end()) return (*found).second;
      }
      
      {
        auto found = mStructs.find(typeName);
        if (found != mStructs.end()) return (*found).second;
      }
      
      {
        auto found = mTypedefs.find(typeName);
        if (found != mTypedefs.end()) return (*found).second;
      }

      {
        for (auto iter = mGenerics.begin(); iter != mGenerics.end(); ++iter) {
          auto type = (*iter);
          auto name = type->getMappingName();

          if (name == typeName) return type;
        }
      }

      {
        auto found = mTemplatedStructs.find(typeName);
        if (found != mTemplatedStructs.end()) return (*found).second;
      }

      {
        FindTypeOptions baseOptions = options;
        baseOptions.mSearchParents = false;

        for (auto iter = mIsARelationships.begin(); iter != mIsARelationships.end(); ++iter)
        {
          auto baseType = (*iter).second;
          baseType->findType(String(), typeName, baseOptions);
        }
      }

      if (options.mSearchParents) {
        auto parent = getParent();
        if (parent) return parent->findType(pathStr, typeName, options);
      }

      return TypePtr();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Struct::resolveTypedefs() throw (InvalidContent)
    {
      for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }

      for (auto iter = mEnums.begin(); iter != mEnums.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }
      for (auto iter = mStructs.begin(); iter != mStructs.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }

      for (auto iter = mProperties.begin(); iter != mProperties.end(); ++iter) {
        auto obj = (*iter);
        obj->resolveTypedefs();
      }
      for (auto iter = mMethods.begin(); iter != mMethods.end(); ++iter) {
        auto obj = (*iter);
        obj->resolveTypedefs();
      }

      for (auto iter = mGenerics.begin(); iter != mGenerics.end(); ++iter) {
        auto obj = (*iter);
        obj->resolveTypedefs();
      }

      for (auto iter_doNotUse = mGenericDefaultTypes.begin(); iter_doNotUse != mGenericDefaultTypes.end(); ) {
        auto current = iter_doNotUse;
        ++iter_doNotUse;
        
        TypePtr obj = (*current);
        if (!obj) continue;
        
        TypePtr bypassType = obj->getOriginalType();
        if (bypassType == obj) continue;

        mGenericDefaultTypes.insert(current, bypassType);
        mGenericDefaultTypes.erase(current);
      }

      for (auto iter = mTemplatedStructs.begin(); iter != mTemplatedStructs.end(); ++iter) {
        auto obj = (*iter).second;
        obj->resolveTypedefs();
      }
    }

    //-------------------------------------------------------------------------
    bool IIDLTypes::Struct::fixTemplateHashMapping()
    {
      bool didFix = false;
      for (auto iter_doNotUse = mStructs.begin(); iter_doNotUse != mStructs.end(); ) {

        auto current = iter_doNotUse;
        ++iter_doNotUse;

        auto obj = (*current).second;
        if (obj->fixTemplateHashMapping()) didFix = true;
      }

      for (auto iter_doNotUse = mTemplatedStructs.begin(); iter_doNotUse != mTemplatedStructs.end(); ) {
        
        auto current = iter_doNotUse;
        ++iter_doNotUse;

        auto name = (*current).first;
        auto obj = (*current).second;

        String calculatedID = obj->calculateTemplateID();

        obj->mName = calculatedID;

        if (name == calculatedID) continue;

        mTemplatedStructs.erase(current);
        mTemplatedStructs[calculatedID] = obj;

        didFix = true;
      }

      return didFix;
    }

    //-------------------------------------------------------------------------
    IIDLTypes::StructPtr IIDLTypes::Struct::getRootStruct() const
    {
      StructPtr structObj = toStruct();

      do
      {
        auto parent = structObj->getParent();
        if (!parent) return structObj;
        auto parentStruct = parent->toStruct();
        if (!parentStruct) break;
        structObj = parentStruct;
      } while (true);

      return structObj;
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::createStructForwards(
                                         ContextPtr context,
                                         ElementPtr structsEl,
                                         StructMap &outStructs
                                         ) throw (InvalidContent)
    {
      if (!structsEl) return;

      auto structEl = structsEl->findFirstChildElement("struct");

      while (structEl) {
        auto structObj = Struct::createForwards(context, structEl);
        outStructs[structObj->getMappingName()] = structObj;

        structEl = structEl->findNextSiblingElement("struct");
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::parseStructs(
                                 ContextPtr context,
                                 ElementPtr structsEl,
                                 StructMap &ioStructs
                                 ) throw (InvalidContent)
    {
      if (!structsEl) return;

      auto structEl = structsEl->findFirstChildElement("struct");

      while (structEl) {
        auto name = context->aliasLookup(UseHelper::getElementTextAndDecode(structEl->findFirstChildElement("name")));

        StructPtr structObj;

        auto found = ioStructs.find(name);
        if (found == ioStructs.end()) {
          structObj = Struct::createForwards(context, structEl);
          ioStructs[structObj->getMappingName()] = structObj;
        } else {
          structObj = (*found).second;
        }
        structObj->parse(structEl);

        structEl = structEl->findNextSiblingElement("struct");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::GenericType
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::GenericType::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::GenericType::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);

      if (!rootEl) return;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::GenericTypePtr IIDLTypes::GenericType::create(ContextPtr context)
    {
      auto pThis(make_shared<GenericType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::GenericTypePtr IIDLTypes::GenericType::createForward(
                                                                    ContextPtr context,
                                                                    const ElementPtr &el
                                                                    ) throw (InvalidContent)
    {
      auto pThis(make_shared<GenericType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }

    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::GenericType::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "generic";

      ElementPtr rootEl = Element::create(objectName);
      
      Context::write(rootEl);
      
      return rootEl;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::GenericType::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;
      
      Context::parse(rootEl);
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::GenericType::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("generic:");
      hasher->update(Context::hash());
      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::createGenericForwards(
                                          ContextPtr context,
                                          ElementPtr genericsEl,
                                          GenericTypeList &outGenerics
                                          ) throw (InvalidContent)
    {
      if (!genericsEl) return;
      
      auto genericEl = genericsEl->findFirstChildElement("generic");
      
      while (genericEl) {
        auto genericObj = GenericType::createForward(context, genericEl);
        outGenerics.push_back(genericObj);

        genericEl = genericEl->findNextSiblingElement("generic");
      }
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::parseGenerics(
                                     ContextPtr context,
                                     ElementPtr genericsEl,
                                     GenericTypeList &ioGenerics
                                     ) throw (InvalidContent)
    {
      if (!genericsEl) return;
      
      auto genericEl = genericsEl->findFirstChildElement("generic");
      
      auto iter = ioGenerics.begin();
      while ((genericEl) &&
             (iter != ioGenerics.end())) {
        
        auto genericObj = (*iter);
        genericObj->parse(genericEl);

        genericEl = genericEl->findNextSiblingElement("generic");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::TemplatedStructType
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::TemplatedStructType::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::TemplatedStructType::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      
      if (!rootEl) return;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::TemplatedStructTypePtr IIDLTypes::TemplatedStructType::create(ContextPtr context)
    {
      auto pThis(make_shared<TemplatedStructType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::TemplatedStructTypePtr IIDLTypes::TemplatedStructType::createForwards(
                                                                                     ContextPtr context,
                                                                                     const ElementPtr &el
                                                                                     ) throw (InvalidContent)
    {
      auto pThis(make_shared<TemplatedStructType>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::TemplatedStructType::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "templatedStruct";
      
      ElementPtr rootEl = Element::create(objectName);

      if (mTemplateArguments.size() > 0) {
        ElementPtr templateArgumentsEl = Element::create("templateArguments");

        for (auto iter = mTemplateArguments.begin(); iter != mTemplateArguments.end(); ++iter) {
          auto templateType = (*iter);
          
          ElementPtr templateArgumentEl = Element::create("templateArgument");

          if (!templateType) {
            templateArgumentEl->adoptAsLastChild(UseHelper::createElementWithNumber("value", "null"));
          } else {
            templateArgumentEl->adoptAsLastChild(templateType->createReferenceTypeElement());
          }
          templateArgumentsEl->adoptAsLastChild(templateArgumentEl);
        }
      }

      return rootEl;
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::TemplatedStructType::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      if (!rootEl) return;
      
      Context::parse(rootEl);
      
      auto context = toContext();
      
      {
        auto templatesEl = rootEl->findFirstChildElement("templateArguments");
        if (templatesEl) {
          auto templateDefaultEl = templatesEl->findFirstChildElement("templateArgument");
          
          while (templateDefaultEl) {
            auto valueEl = templateDefaultEl->findFirstChildElement("value");
            if (valueEl) {
              String value = UseHelper::getElementTextAndDecode(valueEl);
              if (value != "null") {
                ZS_THROW_CUSTOM(InvalidContent, String("Template argument value contains data other than expected null: ") + value);
              }
              mTemplateArguments.push_back(TypePtr());
            } else {
              mTemplateArguments.push_back(createReferencedType(context, templateDefaultEl));
            }
            templateDefaultEl = templateDefaultEl->findNextSiblingElement("templateArgument");
          }
        }
      }
    }
    
    //-------------------------------------------------------------------------
    String IIDLTypes::TemplatedStructType::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("templatedStruct:");
      hasher->update(Context::hash());
      
      hasher->update(":templateArguments:");
      for (auto iter = mTemplateArguments.begin(); iter != mTemplateArguments.end(); ++iter) {
        auto templateObj = (*iter);
        
        if (templateObj) {
          String pathStr = templateObj->getPath();
          String baseStr = templateObj->getMappingName();
          hasher->update(pathStr);
          hasher->update(":");
          hasher->update(baseStr);
        }
        hasher->update(":next:");
      }

      hasher->update(":end");
      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::TemplatedStructType::resolveTypedefs() throw (InvalidContent)
    {
      for (auto iter_doNotUse = mTemplateArguments.begin(); iter_doNotUse != mTemplateArguments.end(); ) {
        auto current = iter_doNotUse;
        ++iter_doNotUse;

        TypePtr obj = (*current);
        if (!obj) continue;

        TypePtr bypassType = obj->getOriginalType();
        if (bypassType == obj) continue;

        mTemplateArguments.insert(current, bypassType);
        mTemplateArguments.erase(current);
      }
    }

    //-------------------------------------------------------------------------
    String IIDLTypes::TemplatedStructType::calculateTemplateID() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("templatedStruct:");
      for (auto iter = mTemplateArguments.begin(); iter != mTemplateArguments.end(); ++iter) {
        auto templateObj = (*iter);
        
        if (templateObj) {
          String pathStr = templateObj->getPath();
          String baseStr = templateObj->getMappingName();
          hasher->update(pathStr);
          hasher->update(":");
          hasher->update(baseStr);
        }
        hasher->update(":next:");
      }

      hasher->update(":end");
      return hasher->finalizeAsString();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::createTemplatedStructTypeForwards(
                                                      ContextPtr context,
                                                      ElementPtr templatedStructsEl,
                                                      TemplatedStructTypeMap &outTemplatedStruct
                                                      ) throw (InvalidContent)
    {
      if (!templatedStructsEl) return;
      
      auto templatedStructEl = templatedStructsEl->findFirstChildElement("templatedStruct");
      
      while (templatedStructEl) {
        auto templatedStructObj = TemplatedStructType::createForwards(context, templatedStructEl);
        outTemplatedStruct[templatedStructObj->getMappingName()] = templatedStructObj;

        templatedStructEl = templatedStructEl->findNextSiblingElement("templatedStruct");
      }
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::parseTemplatedStructTypes(
                                              ContextPtr context,
                                              ElementPtr templatedStructsEl,
                                              TemplatedStructTypeMap &ioTemplatedStruct
                                              ) throw (InvalidContent)
    {
      if (!templatedStructsEl) return;
      
      auto templatedStructEl = templatedStructsEl->findFirstChildElement("templatedStruct");
      
      while (templatedStructEl) {
        auto name = context->aliasLookup(UseHelper::getElementTextAndDecode(templatedStructEl->findFirstChildElement("name")));
        
        TemplatedStructTypePtr templatedStructObj;
        
        auto found = ioTemplatedStruct.find(name);
        if (found == ioTemplatedStruct.end()) {
          templatedStructObj = TemplatedStructType::createForwards(context, templatedStructEl);
          ioTemplatedStruct[templatedStructObj->getMappingName()] = templatedStructObj;
        } else {
          templatedStructObj = (*found).second;
        }
        templatedStructObj->parse(templatedStructEl);

        templatedStructEl = templatedStructEl->findNextSiblingElement("templatedStruct");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Property
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::Property::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Property::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      Context::parse(rootEl);

      if (!rootEl) return;

      mDefaultValue = aliasLookup(UseHelper::getElementTextAndDecode(rootEl->findFirstChildElement("default")));

      auto context = toContext();
      mType = Type::createReferencedType(context, rootEl);
    }

    //-------------------------------------------------------------------------
    IIDLTypes::PropertyPtr IIDLTypes::Property::create(ContextPtr context)
    {
      auto pThis(make_shared<Property>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::PropertyPtr IIDLTypes::Property::create(
                                                       ContextPtr context,
                                                       const ElementPtr &el
                                                       ) throw (InvalidContent)
    {
      auto pThis(make_shared<Property>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::Property::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "property";
      
      ElementPtr rootEl = Element::create(objectName);

      if (mDefaultValue.hasData()) {
        rootEl->adoptAsLastChild(UseHelper::createElementWithTextAndJSONEncode("default", mDefaultValue));
      }

      if (mType) {
        rootEl->adoptAsLastChild(mType->createReferenceTypeElement());
      }

      return rootEl;
    }
    
    
    //-------------------------------------------------------------------------
    String IIDLTypes::Property::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("property:");
      hasher->update(Context::hash());
      hasher->update(":");
      hasher->update(mDefaultValue);
      if (mType) {
        hasher->update(":");
        hasher->update(mType->getPath());
        hasher->update(":");
        hasher->update(mType->getMappingName());
      } else {
        hasher->update(":__");
      }
      hasher->update(":end");

      return hasher->finalizeAsString();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Property::resolveTypedefs() throw (InvalidContent)
    {
      if (mType) {
        mType = mType->getOriginalType();
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::createProperties(
                                     ContextPtr context,
                                     ElementPtr propertiesEl,
                                     PropertyList &outProperties
                                     ) throw (InvalidContent)
    {
      if (!propertiesEl) return;

      auto propertyEl = propertiesEl->findFirstChildElement("property");

      while (propertyEl) {
        auto propertyObj = Property::create(context, propertyEl);
        outProperties.push_back(propertyObj);
        propertyEl = propertyEl->findNextSiblingElement("property");
      }
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IIDLTypes::Method
    #pragma mark

    //-------------------------------------------------------------------------
    void IIDLTypes::Method::init()
    {
      Context::init();
    }
    
    //-------------------------------------------------------------------------
    void IIDLTypes::Method::init(const ElementPtr &rootEl) throw (InvalidContent)
    {
      Context::init(rootEl);
      Context::parse(rootEl);

      auto context = toContext();

      {
        auto resultEl = rootEl->findFirstChildElement("result");
        if (resultEl) {
          mResult = Type::createReferencedType(context, resultEl);
        }
      }

      {
        auto propertiesEl = rootEl->findFirstChildElement("arguments");
        if (propertiesEl) {
          auto propertyEl = propertiesEl->findFirstChildElement("argument");
          while (propertyEl) {
            auto propertyObj = Property::create(context, propertyEl);
            mArguments.push_back(propertyObj);
            propertyEl = propertyEl->findNextSiblingElement("argument");
          }
        }
      }

      {
        auto throwsEl = rootEl->findFirstChildElement("throws");
        if (throwsEl) {
          auto throwEl = throwsEl->findFirstChildElement("throw");
          while (throwEl) {
            auto typeObj = Type::createReferencedType(context, throwEl);
            if (!typeObj) {
              ZS_THROW_CUSTOM(InvalidContent, String("Thrown object was not found for: ") + getMappingName());
            }
            mThrows.push_back(typeObj);
            throwEl = throwEl->findNextSiblingElement("throw");
          }
        }
      }
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::MethodPtr IIDLTypes::Method::create(ContextPtr context)
    {
      auto pThis(make_shared<Method>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    IIDLTypes::MethodPtr IIDLTypes::Method::create(
                                                   ContextPtr context,
                                                   const ElementPtr &el
                                                   ) throw (InvalidContent)
    {
      auto pThis(make_shared<Method>(make_private{}, context));
      pThis->mThisWeak = pThis;
      pThis->init(el);
      return pThis;
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IIDLTypes::Method::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "method";
      
      ElementPtr rootEl = Element::create(objectName);

      if (mResult) {
        auto resultEl = Element::create("result");
        resultEl->adoptAsLastChild(mResult->createReferenceTypeElement());
        rootEl->adoptAsLastChild(resultEl);
      }

      if (mArguments.size() > 0) {
        auto argumentsEl = Element::create("arguments");
        for (auto iter = mArguments.begin(); iter != mArguments.end(); ++iter) {
          auto propertyObj = (*iter);
          argumentsEl->adoptAsLastChild(propertyObj->createElement("argument"));
        }
      }

      if (mThrows.size() > 0) {
        auto throwsEl = Element::create("throws");
        for (auto iter = mThrows.begin(); iter != mThrows.end(); ++iter) {
          auto throwObj = (*iter);
          auto throwEl = Element::create("throw");
          throwEl->adoptAsLastChild(throwObj->createReferenceTypeElement());
          throwsEl->adoptAsLastChild(throwEl);
        }
      }

      return rootEl;
    }
    
    
    //-------------------------------------------------------------------------
    String IIDLTypes::Method::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update("method:");
      hasher->update(Context::hash());

      hasher->update(":result:");
      if (mResult) {
        hasher->update(mResult->getPath());
        hasher->update(":");
        hasher->update(mResult->getMappingName());
        hasher->update(":");
      }

      hasher->update(":arguments:");
      for (auto iter = mArguments.begin(); iter != mArguments.end(); ++iter) {
        auto propertyObj = (*iter);
        hasher->update(propertyObj->getPath());
        hasher->update(":");
        hasher->update(propertyObj->getMappingName());
        hasher->update(":next:");
      }

      hasher->update(":throws:");
      for (auto iter = mThrows.begin(); iter != mThrows.end(); ++iter) {
        auto throwObj = (*iter);
        hasher->update(throwObj->getPath());
        hasher->update(":");
        hasher->update(throwObj->getMappingName());
        hasher->update(":next:");
      }

      hasher->update(":end");
      
      return hasher->finalizeAsString();
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::Method::resolveTypedefs() throw (InvalidContent)
    {
      for (auto iter = mArguments.begin(); iter != mArguments.end(); ++iter) {
        auto obj = (*iter);
        obj->resolveTypedefs();
      }

      for (auto iter_doNotUse = mThrows.begin(); iter_doNotUse != mThrows.end(); )
      {
        auto current = iter_doNotUse;
        ++iter_doNotUse;

        TypePtr type = (*current);
        if (!type) continue;

        TypePtr bypassType = type->getOriginalType();
        if (type == bypassType) continue;

        mThrows.insert(current, bypassType);
        mThrows.erase(current);
      }
    }

    //-------------------------------------------------------------------------
    void IIDLTypes::createMethods(
                                  ContextPtr context,
                                  ElementPtr methodsEl,
                                  MethodList &outMethods
                                  ) throw (InvalidContent)
    {
      if (!methodsEl) return;

      auto methodEl = methodsEl->findFirstChildElement("method");

      while (methodEl) {
        auto methodObj = Method::create(context, methodEl);
        outMethods.push_back(methodObj);
        methodEl = methodEl->findNextSiblingElement("method");
      }
    }

  } // namespace eventing
} // namespace zsLib
