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

#include <zsLib/eventing/internal/zsLib_eventing_EventingTypes.h>
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
    ZS_DECLARE_TYPEDEF_PTR(eventing::IHelper, UseEventingHelper);

    namespace internal
    {
    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::OperationalTypes
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IEventingTypes::toString(OperationalTypes type)
    {
      switch (type)
      {
        case OperationalType_Admin:       return "Admin";
        case OperationalType_Operational: return "Operational";
        case OperationalType_Analytic:    return "Analytic";
        case OperationalType_Debug:       return "Debug";
      }

      return "unknown";
    }

    //-------------------------------------------------------------------------
    IEventingTypes::OperationalTypes IEventingTypes::toOperationalType(const char *type) throw (InvalidArgument)
    {
      String str(type);
      for (IEventingTypes::OperationalTypes index = IEventingTypes::OperationalType_First; index <= IEventingTypes::OperationalType_Last; index = static_cast<IEventingTypes::OperationalTypes>(static_cast<std::underlying_type<IEventingTypes::OperationalTypes>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IEventingTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Operational type is not valid: ") + str);
      return OperationalType_First;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::PredefinedOpCodes
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IEventingTypes::toString(PredefinedOpCodes code)
    {
      switch (code)
      {
        case PredefinedOpCode_Info:       return "Info";
        case PredefinedOpCode_Start:      return "Start";
        case PredefinedOpCode_Stop:       return "Stop";
        case PredefinedOpCode_DC_Start:   return "DC_Start";
        case PredefinedOpCode_DC_Stop:    return "DC_Stop";
        case PredefinedOpCode_Extension:  return "Extension";
        case PredefinedOpCode_Reply:      return "Reply";
        case PredefinedOpCode_Resume:     return "Resume";
        case PredefinedOpCode_Suspend:    return "Suspend";
        case PredefinedOpCode_Send:       return "Send";
        case PredefinedOpCode_Receive:    return "Receive";
      }

      return "unknown";
    }

    //-------------------------------------------------------------------------
    IEventingTypes::PredefinedOpCodes IEventingTypes::toPredefinedOpCode(const char *code) throw (InvalidArgument)
    {
      String str(code);
      for (IEventingTypes::PredefinedOpCodes index = IEventingTypes::PredefinedOpCode_First; index <= IEventingTypes::PredefinedOpCode_Last; index = static_cast<IEventingTypes::PredefinedOpCodes>(static_cast<std::underlying_type<IEventingTypes::PredefinedOpCodes>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IEventingTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Not a predefined op code: ") + str);
      return PredefinedOpCode_First;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::PredefinedLevels
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IEventingTypes::toString(PredefinedLevels level)
    {
      switch (level)
      {
        case PredefinedLevel_Critical:      return "Critical";
        case PredefinedLevel_Error:         return "Error";
        case PredefinedLevel_Warning:       return "Warning";
        case PredefinedLevel_Informational: return "Informational";
        case PredefinedLevel_Verbose:       return "Verbose";
       }

      return "unknown";
    }

    //-------------------------------------------------------------------------
    IEventingTypes::PredefinedLevels IEventingTypes::toPredefinedLevel(const char *level) throw (InvalidArgument)
    {
      String str(level);
      for (IEventingTypes::PredefinedLevels index = IEventingTypes::PredefinedLevel_First; index <= IEventingTypes::PredefinedLevel_Last; index = static_cast<IEventingTypes::PredefinedLevels>(static_cast<std::underlying_type<IEventingTypes::PredefinedLevels>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IEventingTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Not a predefined level: ") + str);
      return PredefinedLevel_First;
    }

    //-------------------------------------------------------------------------
    IEventingTypes::PredefinedLevels IEventingTypes::toPredefinedLevel(
                                                                       Log::Severity severity,
                                                                       Log::Level level
                                                                       )
    {
      switch (severity)
      {
        case Log::Fatal:    return PredefinedLevel_Critical;
        case Log::Error:    return PredefinedLevel_Error;
        case Log::Warning:  return PredefinedLevel_Warning;
        default:            break;
      }

      switch (level)
      {
        case Log::Basic:    return PredefinedLevel_Informational;
        case Log::Detail:   return PredefinedLevel_Informational;
        case Log::Debug:    return PredefinedLevel_Informational;
        default:            break;
      }
      return PredefinedLevel_Verbose;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::BaseTypes
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IEventingTypes::toString(BaseTypes type)
    {
      switch (type)
      {
        case BaseType_Boolean:    return "boolean";
        case BaseType_Integer:    return "integer";
        case BaseType_Float:      return "float";
        case BaseType_Pointer:    return "pointer";
        case BaseType_Binary:     return "binary";
        case BaseType_String:     return "string";
      }

      return "unknown";
    }

    //-------------------------------------------------------------------------
    IEventingTypes::BaseTypes IEventingTypes::toBaseType(const char *type) throw (InvalidArgument)
    {
      String str(type);
      for (IEventingTypes::BaseTypes index = IEventingTypes::BaseType_First; index <= IEventingTypes::BaseType_Last; index = static_cast<IEventingTypes::BaseTypes>(static_cast<std::underlying_type<IEventingTypes::BaseTypes>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IEventingTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Not a base type: ") + str);
      return BaseType_First;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::PredefinedTypedefs
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IEventingTypes::toString(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_void:      return "void";

        case PredefinedTypedef_bool:      return "bool";

        case PredefinedTypedef_uchar:     return "uchar";
        case PredefinedTypedef_char:      return "char";
        case PredefinedTypedef_schar:     return "schar";
        case PredefinedTypedef_ushort:    return "ushort";
        case PredefinedTypedef_short:     return "short";
        case PredefinedTypedef_sshort:    return "sshort";
        case PredefinedTypedef_uint:      return "uint";
        case PredefinedTypedef_int:       return "int";
        case PredefinedTypedef_sint:      return "sint";
        case PredefinedTypedef_ulong:     return "ulong";
        case PredefinedTypedef_long:      return "long";
        case PredefinedTypedef_slong:     return "slong";
        case PredefinedTypedef_ulonglong: return "ulonglong";
        case PredefinedTypedef_longlong:  return "longlong";
        case PredefinedTypedef_slonglong: return "slonglong";

        case PredefinedTypedef_uint8:     return "uint8";
        case PredefinedTypedef_int8:      return "int8";
        case PredefinedTypedef_sint8:     return "sint8";
        case PredefinedTypedef_uint16:    return "uint16";
        case PredefinedTypedef_int16:     return "int16";
        case PredefinedTypedef_sint16:    return "sint16";
        case PredefinedTypedef_uint32:    return "uint32";
        case PredefinedTypedef_int32:     return "int32";
        case PredefinedTypedef_sint32:    return "sint32";
        case PredefinedTypedef_uint64:    return "uint64";
        case PredefinedTypedef_int64:     return "int64";
        case PredefinedTypedef_sint64:    return "sint64";

        case PredefinedTypedef_byte:      return "byte";
        case PredefinedTypedef_word:      return "word";
        case PredefinedTypedef_dword:     return "dword";
        case PredefinedTypedef_qword:     return "qword";

        case PredefinedTypedef_float:     return "float";
        case PredefinedTypedef_double:    return "double";
        case PredefinedTypedef_ldouble:   return "ldouble";
        case PredefinedTypedef_float32:   return "float32";
        case PredefinedTypedef_float64:   return "float64";

        case PredefinedTypedef_pointer:   return "pointer";

        case PredefinedTypedef_binary:    return "binary";
        case PredefinedTypedef_size:      return "size";

        case PredefinedTypedef_string:    return "string";
        case PredefinedTypedef_astring:   return "astring";
        case PredefinedTypedef_wstring:   return "wstring";
      }

      return "unknown";
    }

    //-------------------------------------------------------------------------
    IEventingTypes::PredefinedTypedefs IEventingTypes::toPredefinedTypedef(const char *type) throw (InvalidArgument)
    {
      String str(type);
      for (IEventingTypes::PredefinedTypedefs index = IEventingTypes::PredefinedTypedef_First; index <= IEventingTypes::PredefinedTypedef_Last; index = static_cast<IEventingTypes::PredefinedTypedefs>(static_cast<std::underlying_type<IEventingTypes::PredefinedTypedefs>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IEventingTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Not a predefined type: ") + str);
      return PredefinedTypedef_First;
    }

    //-------------------------------------------------------------------------
    IEventingTypes::PredefinedTypedefs IEventingTypes::toPreferredPredefinedTypedef(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_schar:     return PredefinedTypedef_char;
        case PredefinedTypedef_sshort:    return PredefinedTypedef_short;
        case PredefinedTypedef_sint:      return PredefinedTypedef_int;
        case PredefinedTypedef_slong:     return PredefinedTypedef_long;
        case PredefinedTypedef_slonglong: return PredefinedTypedef_longlong;

        case PredefinedTypedef_sint8:     return PredefinedTypedef_int8;
        case PredefinedTypedef_sint16:    return PredefinedTypedef_int16;
        case PredefinedTypedef_sint32:    return PredefinedTypedef_int32;
        case PredefinedTypedef_sint64:    return PredefinedTypedef_int64;

        case PredefinedTypedef_byte:      return PredefinedTypedef_uint8;
        case PredefinedTypedef_word:      return PredefinedTypedef_uint16;
        case PredefinedTypedef_dword:     return PredefinedTypedef_uint32;
        case PredefinedTypedef_qword:     return PredefinedTypedef_uint64;

        case PredefinedTypedef_astring:   return PredefinedTypedef_string;
        default:                          break;
      }
      return type;
    }

    //-------------------------------------------------------------------------
    IEventingTypes::BaseTypes IEventingTypes::getBaseType(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_bool:      return BaseType_Boolean;

        case PredefinedTypedef_uchar:
        case PredefinedTypedef_char:
        case PredefinedTypedef_schar:
        case PredefinedTypedef_ushort:
        case PredefinedTypedef_short:
        case PredefinedTypedef_sshort:
        case PredefinedTypedef_uint:
        case PredefinedTypedef_int:
        case PredefinedTypedef_sint:
        case PredefinedTypedef_ulong:
        case PredefinedTypedef_long:
        case PredefinedTypedef_slong:
        case PredefinedTypedef_ulonglong:
        case PredefinedTypedef_longlong:
        case PredefinedTypedef_slonglong:

        case PredefinedTypedef_uint8:
        case PredefinedTypedef_int8:
        case PredefinedTypedef_sint8:
        case PredefinedTypedef_uint16:
        case PredefinedTypedef_int16:
        case PredefinedTypedef_sint16:
        case PredefinedTypedef_uint32:
        case PredefinedTypedef_int32:
        case PredefinedTypedef_sint32:
        case PredefinedTypedef_uint64:
        case PredefinedTypedef_int64:
        case PredefinedTypedef_sint64:

        case PredefinedTypedef_byte:
        case PredefinedTypedef_word:
        case PredefinedTypedef_dword:
        case PredefinedTypedef_qword:     return BaseType_Integer;

        case PredefinedTypedef_float:
        case PredefinedTypedef_double:
        case PredefinedTypedef_ldouble:
        case PredefinedTypedef_float32:
        case PredefinedTypedef_float64:   return BaseType_Float;

        case PredefinedTypedef_void:
        case PredefinedTypedef_pointer:   return BaseType_Pointer;

        case PredefinedTypedef_binary:    return BaseType_Binary;
        case PredefinedTypedef_size:      return BaseType_Integer;

        case PredefinedTypedef_string:
        case PredefinedTypedef_astring:
        case PredefinedTypedef_wstring:   return BaseType_String;
      }

      ZS_THROW_NOT_IMPLEMENTED(String("Missing base type conversion for predefined type:") + toString(type));
    }

    //-------------------------------------------------------------------------
    bool IEventingTypes::isSigned(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_void:
        case PredefinedTypedef_bool:

        case PredefinedTypedef_uchar:
        case PredefinedTypedef_ushort:
        case PredefinedTypedef_uint:
        case PredefinedTypedef_ulong:
        case PredefinedTypedef_ulonglong:
        case PredefinedTypedef_uint8:
        case PredefinedTypedef_uint16:
        case PredefinedTypedef_uint32:
        case PredefinedTypedef_uint64:
        case PredefinedTypedef_byte:
        case PredefinedTypedef_word:
        case PredefinedTypedef_dword:
        case PredefinedTypedef_qword:     return false;
        default:                          break;
      }

      return true;
    }

    //-------------------------------------------------------------------------
    bool IEventingTypes::isUnsigned(PredefinedTypedefs type)
    {
      return !isSigned(type);
    }

    //-------------------------------------------------------------------------
    bool IEventingTypes::isAString(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_string:    return true;
        case PredefinedTypedef_astring:   return true;
        default:                          break;
      }
      return false;
    }
    
    //-------------------------------------------------------------------------
    bool IEventingTypes::isWString(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_wstring:   return true;
        default:                          break;
      }
      return false;
    }

    //-------------------------------------------------------------------------
    size_t IEventingTypes::getMinBytes(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_bool:      return sizeof(bool);

        case PredefinedTypedef_uchar:     return sizeof(char);
        case PredefinedTypedef_char:      return sizeof(char);
        case PredefinedTypedef_schar:     return sizeof(char);
        case PredefinedTypedef_ushort:    return sizeof(short);
        case PredefinedTypedef_short:     return sizeof(short);
        case PredefinedTypedef_sshort:    return sizeof(short);
        case PredefinedTypedef_uint:      return sizeof(uint32_t);
        case PredefinedTypedef_int:       return sizeof(int32_t);
        case PredefinedTypedef_sint:      return sizeof(int32_t);
        case PredefinedTypedef_ulong:     return sizeof(uint32_t);
        case PredefinedTypedef_long:      return sizeof(int32_t);
        case PredefinedTypedef_slong:     return sizeof(int32_t);
        case PredefinedTypedef_ulonglong: return sizeof(unsigned long long);
        case PredefinedTypedef_longlong:  return sizeof(long long);
        case PredefinedTypedef_slonglong: return sizeof(long long);

        case PredefinedTypedef_uint8:     return 1;
        case PredefinedTypedef_int8:      return 1;
        case PredefinedTypedef_sint8:     return 1;
        case PredefinedTypedef_uint16:    return 2;
        case PredefinedTypedef_int16:     return 2;
        case PredefinedTypedef_sint16:    return 2;
        case PredefinedTypedef_uint32:    return 4;
        case PredefinedTypedef_int32:     return 4;
        case PredefinedTypedef_sint32:    return 4;
        case PredefinedTypedef_uint64:    return 8;
        case PredefinedTypedef_int64:     return 8;
        case PredefinedTypedef_sint64:    return 8;

        case PredefinedTypedef_byte:      return 1;
        case PredefinedTypedef_word:      return 2;
        case PredefinedTypedef_dword:     return 4;
        case PredefinedTypedef_qword:     return 8;

        case PredefinedTypedef_float:     return sizeof(float);
        case PredefinedTypedef_double:    return sizeof(float);
        case PredefinedTypedef_ldouble:   return sizeof(double);
        case PredefinedTypedef_float32:   return 4;
        case PredefinedTypedef_float64:   return 8;

        case PredefinedTypedef_void:
        case PredefinedTypedef_pointer:   return sizeof(unsigned long);

        case PredefinedTypedef_binary:    return 0;
        case PredefinedTypedef_size:      return sizeof(int);

        case PredefinedTypedef_string:    return 0;
        case PredefinedTypedef_astring:   return 0;
        case PredefinedTypedef_wstring:   return 0;
      }

      return 0;
    }

    //-------------------------------------------------------------------------
    size_t IEventingTypes::getMaxBytes(PredefinedTypedefs type)
    {
      switch (type)
      {
        case PredefinedTypedef_bool:      return sizeof(bool);

        case PredefinedTypedef_uchar:     return sizeof(unsigned char);
        case PredefinedTypedef_char:      return sizeof(char);
        case PredefinedTypedef_schar:     return sizeof(char);
        case PredefinedTypedef_ushort:    return sizeof(unsigned short);
        case PredefinedTypedef_short:     return sizeof(short);
        case PredefinedTypedef_sshort:    return sizeof(short);
        case PredefinedTypedef_uint:      return sizeof(uint64_t);
        case PredefinedTypedef_int:       return sizeof(int64_t);
        case PredefinedTypedef_sint:      return sizeof(int64_t);
        case PredefinedTypedef_ulong:     return sizeof(unsigned long long);
        case PredefinedTypedef_long:      return sizeof(long long);
        case PredefinedTypedef_slong:     return sizeof(long long);
        case PredefinedTypedef_ulonglong: return sizeof(unsigned long long);
        case PredefinedTypedef_longlong:  return sizeof(long long);
        case PredefinedTypedef_slonglong: return sizeof(long long);

        case PredefinedTypedef_uint8:     return 1;
        case PredefinedTypedef_int8:      return 1;
        case PredefinedTypedef_sint8:     return 1;
        case PredefinedTypedef_uint16:    return 2;
        case PredefinedTypedef_int16:     return 2;
        case PredefinedTypedef_sint16:    return 2;
        case PredefinedTypedef_uint32:    return 4;
        case PredefinedTypedef_int32:     return 4;
        case PredefinedTypedef_sint32:    return 4;
        case PredefinedTypedef_uint64:    return 8;
        case PredefinedTypedef_int64:     return 8;
        case PredefinedTypedef_sint64:    return 8;

        case PredefinedTypedef_byte:      return 1;
        case PredefinedTypedef_word:      return 2;
        case PredefinedTypedef_dword:     return 4;
        case PredefinedTypedef_qword:     return 8;

        case PredefinedTypedef_float:     return sizeof(float);
        case PredefinedTypedef_double:    return sizeof(long double);
        case PredefinedTypedef_ldouble:   return sizeof(long double);
        case PredefinedTypedef_float32:   return 4;
        case PredefinedTypedef_float64:   return 8;

        case PredefinedTypedef_void:
        case PredefinedTypedef_pointer:   return sizeof(long long);

        case PredefinedTypedef_binary:    return 0;
        case PredefinedTypedef_size:      return sizeof(long long);

        case PredefinedTypedef_string:    return 0;
        case PredefinedTypedef_astring:   return 0;
        case PredefinedTypedef_wstring:   return 0;
      }

      return 0;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::aliasLookup(const AliasMap *aliases, const String &value)
    {
      if (NULL == aliases) return value;
      return aliasLookup(*aliases, value);
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::aliasLookup(const AliasMap &aliases, const String &value)
    {
      if (value.isEmpty()) return value;

      auto found = aliases.find(value);
      if (found == aliases.end()) return value;

      return (*found).second;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Provider
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::Provider::Provider(const ElementPtr &rootEl) throw(InvalidContent)
    {
      parse(rootEl);
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Provider::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "provider";

      ElementPtr rootEl = Element::create(objectName);

      rootEl->adoptAsLastChild(UseEventingHelper::createElementWithText("id", string(mID)));
      rootEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      rootEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("symbol", mSymbolName));
      rootEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("description", mDescription));
      rootEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("resourceName", mResourceName));
      if (mUniqueHash.hasData()) {
        rootEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("uniqueHash", mUniqueHash));
      }

      if (mAliases.size() > 0) {
        ElementPtr aliasesEl = Element::create("aliases");
        for (auto iter = mAliases.begin(); iter != mAliases.end(); ++iter) {
          ElementPtr aliasEl = Element::create("alias");
          aliasEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("in", (*iter).first));
          aliasEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("out", (*iter).second));
          aliasesEl->adoptAsLastChild(aliasEl);
        }
      }

      if (mTypedefs.size() > 0) {
        ElementPtr typedefsEl = Element::create("typedefs");
        for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter) {
          ElementPtr typedefEl = (*iter).second->createElement("typedef");
          typedefsEl->adoptAsLastChild(typedefEl);
        }
        rootEl->adoptAsLastChild(typedefsEl);
      }

      if (mChannels.size() > 0) {
        ElementPtr channelsEl = Element::create("channels");
        for (auto iter = mChannels.begin(); iter != mChannels.end(); ++iter) {
          ElementPtr channelEl = (*iter).second->createElement("channel");
          channelsEl->adoptAsLastChild(channelEl);
        }
        rootEl->adoptAsLastChild(channelsEl);
      }

      if (mOpCodes.size() > 0) {
        ElementPtr opCodesEl = Element::create("opcodes");
        for (auto iter = mOpCodes.begin(); iter != mOpCodes.end(); ++iter) {
          ElementPtr opCodeEl = (*iter).second->createElement("opcode");
          opCodesEl->adoptAsLastChild(opCodeEl);
        }
        rootEl->adoptAsLastChild(opCodesEl);
      }

      if (mTasks.size() > 0) {
        ElementPtr tasksEl = Element::create("tasks");
        for (auto iter = mTasks.begin(); iter != mTasks.end(); ++iter) {
          ElementPtr taskEl = (*iter).second->createElement("task");
          tasksEl->adoptAsLastChild(taskEl);
        }
        rootEl->adoptAsLastChild(tasksEl);
      }

      if (mEvents.size() > 0) {
        ElementPtr eventsEl = Element::create("events");
        for (auto iter = mEvents.begin(); iter != mEvents.end(); ++iter) {
          ElementPtr eventEl = (*iter).second->createElement("event");
          eventsEl->adoptAsLastChild(eventEl);
        }
        rootEl->adoptAsLastChild(eventsEl);
      }

      if (mDataTemplates.size() > 0) {
        ElementPtr templatesEl = Element::create("templates");
        for (auto iter = mDataTemplates.begin(); iter != mDataTemplates.end(); ++iter) {
          ElementPtr templateEl = (*iter).second->createElement("template");
          templatesEl->adoptAsLastChild(templateEl);
        }
        rootEl->adoptAsLastChild(templatesEl);
      }
      
      if (mSubsystems.size() > 0) {
        ElementPtr subsystemsEl = Element::create("subsystems");
        for (auto iter = mSubsystems.begin(); iter != mSubsystems.end(); ++iter)
        {
          auto subsystemEl = (*iter).second->createElement("subsystem");
          subsystemsEl->adoptAsLastChild(subsystemEl);
        }
        rootEl->adoptAsLastChild(subsystemsEl);
      }

      return rootEl;
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::Provider::parse(const ElementPtr &rootEl) throw (InvalidContent)
    {
      createAliases(rootEl->findFirstChildElement("aliases"), mAliases);

      ElementPtr idEl = rootEl->findFirstChildElement("id");
      if (idEl) {
        String idStr = aliasLookup(UseEventingHelper::getElementTextAndDecode(idEl));

        if (!(!mID)) {  // UUID has existing value
          ZS_THROW_CUSTOM(InvalidContent, String("Provider id already has a value: ") + string(mID));
        }
        try {
          mID = Numeric<decltype(mID)>(idStr);
        } catch (const Numeric<decltype(mID)>::ValueOutOfRange &) {
          ZS_THROW_CUSTOM(InvalidContent, String("ID value is not valid, id=") + idStr);
        }
      }
      ElementPtr nameEl = rootEl->findFirstChildElement("name");
      if (nameEl) {
        if (mName.hasData()) {
          ZS_THROW_CUSTOM(InvalidContent, String("provider name already has a value: ") + mName);
        }
        mName = aliasLookup(UseEventingHelper::getElementTextAndDecode(nameEl));
      }
      ElementPtr symbolEl = rootEl->findFirstChildElement("symbol");
      if (symbolEl) {
        if (mSymbolName.hasData()) {
          ZS_THROW_CUSTOM(InvalidContent, String("provider symbol name already has a value: ") + mSymbolName);
        }
        mSymbolName = aliasLookup(UseEventingHelper::getElementTextAndDecode(symbolEl));
      }
      ElementPtr descriptionEl = rootEl->findFirstChildElement("description");
      if (descriptionEl) {
        if (mDescription.hasData()) {
          ZS_THROW_CUSTOM(InvalidContent, String("provider description name already has a value: ") + mDescription);
        }
        mDescription = aliasLookup(UseEventingHelper::getElementTextAndDecode(descriptionEl));
      }
      ElementPtr resourceNameEl = rootEl->findFirstChildElement("resourceName");
      if (resourceNameEl) {
        if (mResourceName.hasData()) {
          ZS_THROW_CUSTOM(InvalidContent, String("Provider resource name already has a value: ") + mResourceName);
        }
        mResourceName = aliasLookup(UseEventingHelper::getElementTextAndDecode(resourceNameEl));
      }

      ElementPtr uniqueHashEl = rootEl->findFirstChildElement("uniqueHash");
      if (uniqueHashEl) {
        String uniqueHash = UseEventingHelper::getElementTextAndDecode(uniqueHashEl);
        if (mUniqueHash.hasData()) {
          if (uniqueHash != mUniqueHash) {
            ZS_THROW_CUSTOM(InvalidContent, String("Provider unique hash already has a value: ") + mUniqueHash);
          }
        }
        mUniqueHash = aliasLookup(uniqueHash);
      }

      createTypesdefs(rootEl->findFirstChildElement("typedefs"), mTypedefs, &mAliases);
      createChannels(rootEl->findFirstChildElement("channels"), mChannels, &mAliases);
      createOpCodes(rootEl->findFirstChildElement("opcodes"), mOpCodes, &mAliases);
      createTasks(rootEl->findFirstChildElement("tasks"), mTasks, &mAliases);
      createKeywords(rootEl->findFirstChildElement("keywords"), mKeywords, &mAliases);
      createDataTemplates(rootEl->findFirstChildElement("templates"), mDataTemplates, mTypedefs, &mAliases);
      createEvents(rootEl->findFirstChildElement("events"), mEvents, mChannels, mOpCodes, mTasks, mKeywords, mDataTemplates, &mAliases);
      createSubsystems(rootEl->findFirstChildElement("subsystems"), mSubsystems, &mAliases);
      
      {
        for (auto iter_doNotUse = mDataTemplates.begin(); iter_doNotUse != mDataTemplates.end();)
        {
          auto current = iter_doNotUse;
          ++iter_doNotUse;

          String key = (*current).first;
          DataTemplatePtr templateObj = (*current).second;
          auto hash = templateObj->uniqueID();
          auto uniqueID = templateObj->hash();

          if (hash == uniqueID) continue;
          if (key == hash) continue;

          mDataTemplates.erase(current);
        }
      }
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Provider::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(string(mID));
      hasher->update(":");
      hasher->update(mName);
      hasher->update(":");
      hasher->update(mSymbolName);
      hasher->update(":");
      hasher->update(mDescription);
      hasher->update(":");
      hasher->update(mResourceName);
      hasher->update(":");
      hasher->update(mUniqueHash);

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

      hasher->update(":list:typedefs");
      for (auto iter = mTypedefs.begin(); iter != mTypedefs.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }

      hasher->update(":list:channels");
      for (auto iter = mChannels.begin(); iter != mChannels.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }

      hasher->update(":list:opcodes");
      for (auto iter = mOpCodes.begin(); iter != mOpCodes.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }

      hasher->update(":list:tasks");
      for (auto iter = mTasks.begin(); iter != mTasks.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }

      hasher->update(":list:keywords");
      for (auto iter = mKeywords.begin(); iter != mKeywords.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }
      
      hasher->update(":list:events");
      for (auto iter = mEvents.begin(); iter != mEvents.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }

      hasher->update(":list:templates");
      for (auto iter = mDataTemplates.begin(); iter != mDataTemplates.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }

      hasher->update(":list:subsystems");
      for (auto iter = mSubsystems.begin(); iter != mSubsystems.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }
      hasher->update(":end");
      
      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Provider::uniqueEventingHash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(string(mID));
      hasher->update(":");
      hasher->update(mName);
      hasher->update(":");
      hasher->update(mSymbolName);
      hasher->update(":");
      hasher->update(mDescription);
      hasher->update(":");
      hasher->update(mResourceName);

      hasher->update(":list:events");
      for (auto iter = mEvents.begin(); iter != mEvents.end(); ++iter)
      {
        hasher->update(":");
        hasher->update((*iter).second->hash());
      }
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Provider::aliasLookup(const String &value)
    {
      return IEventingTypes::aliasLookup(mAliases, value);
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createAliases(
                                       ElementPtr aliasesEl,
                                       AliasMap &ioAliases
                                       ) throw(InvalidContent)
    {
      if (!aliasesEl) return;

      ElementPtr aliasEl = aliasesEl->findFirstChildElement("alias");
      while (aliasEl) {
        String aliasName = UseEventingHelper::getElementTextAndDecode(aliasEl->findFirstChildElement("in"));
        String aliasValue = UseEventingHelper::getElementTextAndDecode(aliasEl->findFirstChildElement("out"));

        auto found = ioAliases.find(aliasName);
        if (found != ioAliases.end()) {
          auto foundValue = (*found).second;
          if (foundValue != aliasValue) {
            ZS_THROW_CUSTOM(InvalidContent, String("Alias remapped to new value, name=") + aliasName + ", was=" + foundValue + ", now=" + aliasValue);
          }
        }

        ioAliases[aliasName] = aliasValue;

        aliasEl = aliasEl->findNextSiblingElement("alias");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Typedef
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::Typedef::Typedef(
                                     const ElementPtr &rootEl,
                                     const AliasMap *aliases
                                     ) throw (InvalidArgument)
    {
      if (!rootEl) return;

      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      String type = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("type")));

      mType = toPredefinedTypedef(type);
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Typedef::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "typedef";

      ElementPtr resultEl = Element::create(objectName);

      resultEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      resultEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("type", toString(mType)));

      return resultEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Typedef::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(mName);
      hasher->update(":");
      hasher->update(toString(mType));
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createTypesdefs(
                                         ElementPtr typedefsEl,
                                         TypedefMap &ioTypedefs,
                                         const AliasMap *aliases
                                         ) throw(InvalidContent)
    {
      typedef std::map<String, String> StringMap;

      if (!typedefsEl) return;

      StringMap typedefs;

      // keep track of existing typedefs
      for (auto iter = ioTypedefs.begin(); iter != ioTypedefs.end(); ++iter)
      {
        auto &value = (*iter).second;
        typedefs[value->mName] = toString(value->mType);
      }

      ElementPtr typedefEl = typedefsEl->findFirstChildElement("typedef");
      while (typedefEl) {
        String name = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(typedefEl->findFirstChildElement("name")));
        String type = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(typedefEl->findFirstChildElement("type")));

        if ((name.isEmpty()) ||
          (type.isEmpty())) continue;

        auto existing = typedefs.find(name);
        if (existing != typedefs.end()) {
          auto existingType = (*existing).second;
          if (type != existingType) {
            ZS_THROW_CUSTOM(InvalidContent, String("Typedef remapped to new value, name=") + name + ", was=" + existingType + ", now=" + type);
          }
        }

        typedefs[name] = type;

        typedefEl = typedefEl->findNextSiblingElement("typedef");
      }

      StringMap tempTypedefs(typedefs);

      while (tempTypedefs.size() > 0) {
        StringMap processedTypedefs;

        auto iter = tempTypedefs.begin();

        String name = (*iter).first;
        String type = (*iter).second;

        tempTypedefs.erase(iter);

        do {
          try {
            auto predefined = IEventingTypes::toPredefinedTypedef(type);

            auto typedefObj = IEventingTypes::Typedef::create();
            typedefObj->mName = name;
            typedefObj->mType = IEventingTypes::toPreferredPredefinedTypedef(predefined);
            ioTypedefs[name] = typedefObj;
            break;
          } catch (const InvalidArgument &) {
            // valid case - ignored
          }

          auto found = typedefs.find(type);
          if (found == typedefs.end()) {
            ZS_THROW_CUSTOM(InvalidContent, String("Typedef not mapped to predefined type: ") + type);
          }

          auto foundProcessed = processedTypedefs.find(type);
          if (foundProcessed != processedTypedefs.end()) {
            ZS_THROW_CUSTOM(InvalidContent, String("Circular typedef not mapped to predefined type: ") + type);
          }

          processedTypedefs[type] = String();

          type = (*found).second;
        } while (true);
      }
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Channel
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::Channel::Channel(
                                     const ElementPtr &rootEl,
                                     const AliasMap *aliases
                                     ) throw (InvalidContent)
    {
      mID = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findLastChildElement("id")));
      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findLastChildElement("name")));
      String type = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findLastChildElement("type")));
      String value = aliasLookup(aliases, UseEventingHelper::getElementText(rootEl->findLastChildElement("value")));

      if (type.hasData()) {
        try {
          mType = IEventingTypes::toOperationalType(type);
        } catch (const InvalidArgument &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Channel operational type is invalid: ") + type);
        }
      }

      if (value.hasData()) {
        try {
          mValue = Numeric<decltype(mValue)>(value);
        } catch (const Numeric<decltype(mValue)>::ValueOutOfRange &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Channel value is out of range: ") + value);
        }
      }
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Channel::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "channel";

      ElementPtr channelEl = Element::create(objectName);
      
      if (mID.hasData()) channelEl->adoptAsLastChild(UseEventingHelper::createElementWithText("id", mID));
      if (mName.hasData()) channelEl->adoptAsLastChild(UseEventingHelper::createElementWithText("name", mName));
      channelEl->adoptAsLastChild(UseEventingHelper::createElementWithText("name", IEventingTypes::toString(mType)));
      if (0 != mValue) channelEl->adoptAsLastChild(UseEventingHelper::createElementWithNumber("name", string(mValue)));

      return channelEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Channel::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(mID);
      hasher->update(":");
      hasher->update(mName);
      hasher->update(":");
      hasher->update(toString(mType));
      hasher->update(":");
      hasher->update(string(mValue));
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createChannels(
                                        ElementPtr channelsEl,
                                        ChannelMap &outChannels,
                                        const AliasMap *aliases
                                        ) throw(InvalidContent)
    {
      if (NULL == channelsEl) return;

      ElementPtr channelEl = channelsEl->findFirstChildElement("channel");
      while (channelEl)
      {
        auto channel = Channel::create(channelEl, aliases);

        if (outChannels.find(channel->mID) != outChannels.end()) {
          ZS_THROW_CUSTOM(InvalidContent, String("Channel already exists: ") + channel->mID);
        }
        outChannels[channel->mID] = channel;
        channelEl = channelEl->findNextSiblingElement("channel");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Task
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::Task::Task(
                               const ElementPtr &rootEl,
                               const AliasMap *aliases
                               ) throw (InvalidContent)
    {
      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      String value = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("value")));

      if (value.hasData()) {
        try {
          mValue = Numeric<decltype(mValue)>(value);
        } catch (const Numeric<decltype(mValue)>::ValueOutOfRange &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Task value is out of range: ") + value);
        }
      }

      createOpCodes(rootEl->findFirstChildElement("opcodes"), mOpCodes, aliases);
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Task::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "channel";

      ElementPtr taskEl = Element::create(objectName);

      if (mName.hasData()) taskEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      if (0 != mValue) taskEl->adoptAsLastChild(UseEventingHelper::createElementWithNumber("value", string(mValue)));

      if (mOpCodes.size() > 0) {
        ElementPtr opCodesEl = Element::create("opcodes");

        for (auto iter = mOpCodes.begin(); iter != mOpCodes.end(); ++iter) {
          ElementPtr opCode = (*iter).second->createElement("opcode");
          opCodesEl->adoptAsLastChild(opCode);
        }
        taskEl->adoptAsLastChild(opCodesEl);
      }

      return taskEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Task::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(mName);

      hasher->update(":list");
      for (auto iter = mOpCodes.begin(); iter != mOpCodes.end(); ++iter)
      {
        auto opCode = (*iter).second;

        hasher->update(":");
        String hash = opCode->hash(true);
        hasher->update(hash);
      }
      hasher->update(":");
      hasher->update(string(mValue));
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createTasks(
                                     ElementPtr tasksEl,
                                     TaskMap &outTasks,
                                     const AliasMap *aliases
                                     ) throw (InvalidContent)
    {
      if (NULL == tasksEl) return;

      ElementPtr taskEl = tasksEl->findFirstChildElement("task");
      while (taskEl)
      {
        auto task = Task::create(taskEl);
        for (auto iter = task->mOpCodes.begin(); iter != task->mOpCodes.end(); ++iter)
        {
          auto opCode = (*iter).second;
          opCode->mTask = task;
        }
        if (outTasks.find(task->mName) != outTasks.end()) {
          ZS_THROW_CUSTOM(InvalidContent, String("Task already exists: ") + task->mName);
        }
        outTasks[task->mName] = task;
        taskEl = taskEl->findNextSiblingElement("task");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Keyword
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::Keyword::Keyword(
                                     const ElementPtr &rootEl,
                                     const AliasMap *aliases
                                     ) throw (InvalidContent)
    {
      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      String mask = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("mask")));

      if (mask.hasData()) {
        try {
          mMask = Numeric<decltype(mMask)>(mask);
        } catch (const Numeric<decltype(mMask)>::ValueOutOfRange &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Keyword mask is out of range: ") + mask);
        }
      }
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Keyword::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "keyword";

      ElementPtr keywordEl = Element::create(objectName);

      if (mName.hasData()) keywordEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      if (0 != mMask) keywordEl->adoptAsLastChild(UseEventingHelper::createElementWithNumber("mask", string(mMask)));

      return keywordEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Keyword::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(mName);
      hasher->update(":");
      hasher->update(reinterpret_cast<const BYTE *>(&mMask), sizeof(mMask));
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createKeywords(
                                        ElementPtr keywordsEl,
                                        KeywordMap &outKeywords,
                                        const AliasMap *aliases
                                        ) throw (InvalidContent)
    {
      if (NULL == keywordsEl) return;

      ElementPtr keywordEl = keywordsEl->findFirstChildElement("keyword");
      while (keywordEl)
      {
        auto keyword = Keyword::create(keywordEl);
        if (outKeywords.find(keyword->mName) != outKeywords.end()) {
          ZS_THROW_CUSTOM(InvalidContent, String("Keyword already exists: ") + keyword->mName);
        }
        outKeywords[keyword->mName] = keyword;
        keywordEl = keywordEl->findNextSiblingElement("keyword");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::OpCode
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::OpCode::OpCode(
                                   const ElementPtr &rootEl,
                                   const AliasMap *aliases
                                   ) throw (InvalidContent)
    {
      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      String value = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("value")));

      if (value.hasData()) {
        try {
          mValue = Numeric<decltype(mValue)>(value);
        } catch (const Numeric<decltype(mValue)>::ValueOutOfRange &) {
          ZS_THROW_CUSTOM(InvalidContent, String("OpCode value is out of range: ") + value);
        }
      }
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::OpCode::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "opcode";

      ElementPtr opCodeEl = Element::create(objectName);
      if (mName.hasData()) opCodeEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      if (0 != mValue) opCodeEl->adoptAsLastChild(UseEventingHelper::createElementWithNumber("value", string(mValue)));
      return opCodeEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::OpCode::hash(bool calledFromTask) const
    {
      auto hasher = IHasher::sha256();

      hasher->update(mName);

      hasher->update(":");
      auto task = mTask.lock();
      if ((task) &&
          (!calledFromTask)) {
        hasher->update(task->hash());
      }
      hasher->update(":");
      hasher->update(string(mValue));
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createOpCodes(
                                       ElementPtr opCodesEl,
                                       OpCodeMap &outOpCodes,
                                       const AliasMap *aliases
                                       ) throw (InvalidContent)
    {
      if (!opCodesEl) return;

      ElementPtr opCodeEl = opCodesEl->findFirstChildElement("opcode");
      while (opCodeEl)
      {
        auto opCode = OpCode::create(opCodeEl);
        if (outOpCodes.find(opCode->mName) != outOpCodes.end()) {
          ZS_THROW_CUSTOM(InvalidContent, String("OpCode already exists: ") + opCode->mName);
        }
        outOpCodes[opCode->mName] = opCode;
        opCodeEl = opCodeEl->findNextSiblingElement("opcode");
      }
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Event
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::Event::Event(
                                 const ElementPtr &rootEl,
                                 const AliasMap *aliases
                                 ) throw (InvalidContent)
    {
      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      mSubsystem = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("subsystem")));

      String severityStr = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("severity")));
      String levelStr = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("level")));
      try {
        mSeverity = Log::toSeverity(severityStr);
      } catch (const InvalidArgument &e) {
        ZS_THROW_CUSTOM(InvalidContent, String("Event severity is invalid: ") + e.message());
      }
      try {
        mLevel = Log::toLevel(levelStr);
      } catch (const InvalidArgument &e) {
        ZS_THROW_CUSTOM(InvalidContent, String("Event level is invalid: ") + e.message());
      }
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Event::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "event";
      ElementPtr eventEl = Element::create(objectName);

      if (mName.hasData()) {
        eventEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      }
      if (mSubsystem.hasData()) {
        eventEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("subsytem", mSubsystem));
      }
      eventEl->adoptAsLastChild(UseEventingHelper::createElementWithText("severity", Log::toString(mSeverity)));
      eventEl->adoptAsLastChild(UseEventingHelper::createElementWithText("level", Log::toString(mLevel)));

      if (mChannel) {
        eventEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("channel", mChannel->mID));
      }
      if (mTask) {
        eventEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("task", mTask->mName));
      }
      if (mKeywords.size() > 0) {
        auto keywordsEl = Element::create("keywords");
        for (auto iter = mKeywords.begin(); iter != mKeywords.end(); ++iter) {
          auto keyword = (*iter).second;
          
          keywordsEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("keyword", keyword->mName));
        }
        eventEl->adoptAsLastChild(keywordsEl);
      }
      if (mOpCode) {
        eventEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("opcode", mOpCode->mName));
      }
      if (mDataTemplate) {
        eventEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("template", mDataTemplate->hash()));
      }
      if (0 != mValue) eventEl->adoptAsLastChild(UseEventingHelper::createElementWithNumber("value", string(mValue)));
      return eventEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Event::hash() const
    {
      auto hasher = IHasher::sha256();

      hasher->update(mName);
      hasher->update(":");
      hasher->update(Log::toString(mSeverity));
      hasher->update(":");
      hasher->update(Log::toString(mLevel));
      hasher->update(":");
      if (mChannel) {
        hasher->update(mChannel->hash());
      }
      hasher->update(":");
      if (mTask) {
        hasher->update(mTask->hash());
      }
      hasher->update(":keywords:start:");
      for (auto iter = mKeywords.begin(); iter != mKeywords.end(); ++iter) {
        auto keyword = (*iter).second;
        hasher->update(keyword->mName);
        hasher->update(":");
      }
      hasher->update(":keywords:end:");
      hasher->update(":");
      if (mOpCode) {
        hasher->update(mOpCode->hash());
      }
      hasher->update(":");
      if (mDataTemplate) {
        hasher->update(mDataTemplate->hash());
      }
      hasher->update(":");
      hasher->update(string(mValue));
      hasher->update(":end");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createEvents(
                                      ElementPtr eventsEl,
                                      EventMap &outEvents,
                                      const ChannelMap &channels,
                                      OpCodeMap &opCodes,
                                      const TaskMap &tasks,
                                      const KeywordMap &keywords,
                                      const DataTemplateMap &dataTemplates,
                                      const AliasMap *aliases
                                      ) throw (InvalidContent)
    {
      if (!eventsEl) return;

      ElementPtr eventEl = eventsEl->findFirstChildElement("event");
      while (eventEl)
      {
        auto event = Event::create(eventEl);
        outEvents[event->mName] = event;

        String channel = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(eventEl->findFirstChildElement("channel")));
        String opCode = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(eventEl->findFirstChildElement("opcode")));
        String task = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(eventEl->findFirstChildElement("task")));
        String templateStr = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(eventEl->findFirstChildElement("template")));
        String valueStr = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(eventEl->findFirstChildElement("value")));

        if (channel.hasData()) {
          auto found = channels.find(channel);
          if (found == channels.end()) {
            ZS_THROW_CUSTOM(InvalidContent, String("Event \"") + event->mName + "\" links to invalid channel:" + channel);
          }
          event->mChannel = (*found).second;
        }

        if (task.hasData()) {
          auto found = tasks.find(task);
          if (found == tasks.end()) {
            ZS_THROW_CUSTOM(InvalidContent, String("Event \"") + event->mName + "\" links to invalid task:" + task);
          }

          auto taskObj = (*found).second;

          if (opCode.hasData()) {
            auto foundOpCode = taskObj->mOpCodes.find(opCode);
            if (foundOpCode != taskObj->mOpCodes.end()) {
              auto opCodeObj = (*foundOpCode).second;
              opCode = String();
              event->mOpCode = opCodeObj;
            }
          }

          event->mTask = taskObj;
        }

        if (opCode.hasData()) {
          auto found = opCodes.find(opCode);
          if (found == opCodes.end()) {
            try {
              auto predefinedOpCode = IEventingTypes::toPredefinedOpCode(opCode);
              event->mOpCode = IEventingTypes::OpCode::create();
              event->mOpCode->mName = opCode;
              event->mOpCode->mValue = predefinedOpCode;
              opCodes[opCode] = event->mOpCode;
            } catch (const InvalidArgument &) {
              ZS_THROW_CUSTOM(InvalidContent, String("Event \"") + event->mName + "\" links to invalid opcode:" + opCode);
            }
          } else {
            event->mOpCode = (*found).second;
          }
        }

        if (templateStr.hasData()) {
          auto found = dataTemplates.find(templateStr);
          if (found == dataTemplates.end()) {
            ZS_THROW_CUSTOM(InvalidContent, String("Event \"") + event->mName + "\" links to invalid data template:" + templateStr);
          }
          event->mDataTemplate = (*found).second;
        }

        if (valueStr.hasData()) {
          try {
            event->mValue = Numeric<decltype(event->mValue)>(valueStr);
          } catch (const Numeric<decltype(event->mValue)>::ValueOutOfRange &) {
            ZS_THROW_CUSTOM(InvalidContent, String("Event \"") + event->mName + "\" has invalid value:" + valueStr);
          }
        }

        eventEl = eventEl->findNextSiblingElement("event");
      }
    }


    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::DataTemplate
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::DataTemplate::DataTemplate(
                                               const ElementPtr &rootEl,
                                               const AliasMap *aliases
                                               ) throw (InvalidContent)
    {
      mID = UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("id"));
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::DataTemplate::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "template";

      auto templateEl = Element::create("template");

      if (mDataTypes.size() > 0) {
        String id = hash(); // do not use the provided unique ID, generate a new ID

        templateEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("id", id));
        
        ElementPtr dataTypesEl = Element::create("dataTypes");
        for (auto iter = mDataTypes.begin(); iter != mDataTypes.end(); ++iter)
        {
          auto dataType = (*iter);
          auto dataTypeEl = dataType->createElement("dataType");

          dataTypesEl->adoptAsLastChild(dataTypeEl);
        }
        templateEl->adoptAsLastChild(dataTypesEl);
      }

      return templateEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::DataTemplate::uniqueID() const
    {
      if (mID.hasData()) return mID;
      return hash();
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::DataTemplate::hash() const
    {
      auto hasher = IHasher::sha256();

      for (auto iter = mDataTypes.begin(); iter != mDataTypes.end(); ++iter)
      {
        auto dataType = (*iter);
        auto typeStr = IEventingTypes::toString(dataType->mType);
        hasher->update(typeStr);
        hasher->update(":");
        hasher->update(dataType->mValueName);
        hasher->update(":!:");
      }

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createDataTemplates(
                                             ElementPtr templatesEl,
                                             DataTemplateMap &outDataTemplates,
                                             const TypedefMap &typedefs,
                                             const AliasMap *aliases
                                             ) throw (InvalidContent)
    {
      if (!templatesEl) return;

      ElementPtr templateEl = templatesEl->findFirstChildElement("template");
      while (templateEl)
      {
        auto templateObj = DataTemplate::create(templateEl, aliases);

        ElementPtr typesEl = templateEl->findFirstChildElement("dataTypes");
        createDataTypes(typesEl, templateObj->mDataTypes, typedefs, aliases);

        String uniqueID = templateObj->uniqueID();
        String hash = templateObj->hash();

        {
          auto found = outDataTemplates.find(uniqueID);
          if (found == outDataTemplates.end()) {
            // only insert the data template if it's not been inserted before
            outDataTemplates[uniqueID] = templateObj;
          }
        }

        {
          auto found = outDataTemplates.find(hash);
          if (found == outDataTemplates.end()) {
            // only insert the data template if it's not been inserted before
            outDataTemplates[hash] = templateObj;
          }
        }

        templateEl = templateEl->findNextSiblingElement("template");
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::DataType
    #pragma mark

    //-------------------------------------------------------------------------
    IEventingTypes::DataType::DataType(
                                       const ElementPtr &rootEl,
                                       const TypedefMap *typedefs,
                                       const AliasMap *aliases
                                       ) throw (InvalidContent)
    {
      mValueName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      String type = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("type")));

      if (NULL != typedefs) {
        auto found = typedefs->find(type);
        if (found != typedefs->end()) {
          mType = (*found).second->mType;
          type = String();
        }
      }

      if (type.hasData()) {
        try {
          mType = IEventingTypes::toPredefinedTypedef(type);
        } catch (const InvalidArgument &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Data type \"") + mValueName + "\" type is not valid: " + type);
        }
      }
    }

    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::DataType::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "dataType";

      ElementPtr dataTypeEl = Element::create(objectName);

      if (mValueName.hasData()) dataTypeEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mValueName));
      dataTypeEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("type", IEventingTypes::toString(mType)));

      return dataTypeEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::DataType::hash() const
    {
      auto hasher = IHasher::sha256();

      auto typeStr = IEventingTypes::toString(mType);
      hasher->update(typeStr);
      hasher->update(":");
      hasher->update(mValueName);
      hasher->update(":!:");

      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }

    //-------------------------------------------------------------------------
    void IEventingTypes::createDataTypes(
                                         ElementPtr dataTypesEl,
                                         DataTypeList &outDataTypes,
                                         const TypedefMap &typedefs,
                                         const AliasMap *aliases
                                         ) throw (InvalidContent)
    {
      if (!dataTypesEl) return;

      ElementPtr dataTypeEl = dataTypesEl->findFirstChildElement("dataType");
      while (dataTypeEl)
      {
        auto dataType = DataType::create(dataTypeEl, &typedefs, aliases);
        dataTypeEl = dataTypeEl->findNextSiblingElement("dataType");
        outDataTypes.push_back(dataType);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IEventingTypes::Subsystem
    #pragma mark
    
    //-------------------------------------------------------------------------
    IEventingTypes::Subsystem::Subsystem(
                                         const ElementPtr &rootEl,
                                         const AliasMap *aliases
                                         ) throw (InvalidContent)
    {
      mName = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("name")));
      String levelStr = aliasLookup(aliases, UseEventingHelper::getElementTextAndDecode(rootEl->findFirstChildElement("level")));
      
      if (levelStr.hasData()) {
        try {
          mLevel = zsLib::Log::toLevel(levelStr);
        } catch (const InvalidArgument &) {
          ZS_THROW_CUSTOM(InvalidContent, String("Subsystem \"") + mName + "\" level is not valid: " + levelStr);
        }
      }
    }
    
    //-------------------------------------------------------------------------
    ElementPtr IEventingTypes::Subsystem::createElement(const char *objectName) const
    {
      if (NULL == objectName) objectName = "subsystem";
      
      ElementPtr subsystemEl = Element::create(objectName);
      
      if (mName.hasData()) subsystemEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("name", mName));
      subsystemEl->adoptAsLastChild(UseEventingHelper::createElementWithTextAndJSONEncode("level", zsLib::Log::toString(mLevel)));
      
      return subsystemEl;
    }

    //-------------------------------------------------------------------------
    String IEventingTypes::Subsystem::hash() const
    {
      auto hasher = IHasher::sha256();
      
      hasher->update(":subsystem:");
      hasher->update(mName);
      hasher->update(":");
      hasher->update(Log::toString(mLevel));
      hasher->update(":");
      
      return UseEventingHelper::convertToHex(hasher->finalize(), hasher->digestSize());
    }
    
    //-------------------------------------------------------------------------
    void IEventingTypes::createSubsystems(
                                          ElementPtr subsystemsEl,
                                          SubsystemMap &outSubsystems,
                                          const AliasMap *aliases
                                          ) throw (InvalidContent)
    {
      if (!subsystemsEl) return;
      
      ElementPtr subsystemEl = subsystemsEl->findFirstChildElement("subsystem");
      while (subsystemEl)
      {
        auto subsystem = Subsystem::create(subsystemEl, aliases);
        subsystemEl = subsystemEl->findNextSiblingElement("subsystem");

        // NOTE: This could override a previous value, but that's okay
        outSubsystems[subsystem->mName] = subsystem;
      }
    }
    
  } // namespace eventing
} // namespace zsLib
