//**********************************************************************`
//* This is an include file generated by Message Compiler.             *`
//*                                                                    *`
//* Copyright (c) Microsoft Corporation. All Rights Reserved.          *`
//**********************************************************************`
#pragma once
#include <wmistr.h>
#include <evntrace.h>
#include "evntprov.h"
//
//  Initial Defs
//
#if !defined(ETW_INLINE)
#define ETW_INLINE DECLSPEC_NOINLINE __inline
#endif

#if defined(__cplusplus)
extern "C" {
#endif

//
// Allow disabling of code generation
//
#ifndef MCGEN_DISABLE_PROVIDER_CODE_GENERATION
#if  !defined(McGenDebug)
#define McGenDebug(a,b)
#endif 


#if !defined(MCGEN_TRACE_CONTEXT_DEF)
#define MCGEN_TRACE_CONTEXT_DEF
typedef struct _MCGEN_TRACE_CONTEXT
{
    TRACEHANDLE            RegistrationHandle;
    TRACEHANDLE            Logger;
    ULONGLONG              MatchAnyKeyword;
    ULONGLONG              MatchAllKeyword;
    ULONG                  Flags;
    ULONG                  IsEnabled;
    UCHAR                  Level; 
    UCHAR                  Reserve;
    USHORT                 EnableBitsCount;
    PULONG                 EnableBitMask;
    const ULONGLONG*       EnableKeyWords;
    const UCHAR*           EnableLevel;
} MCGEN_TRACE_CONTEXT, *PMCGEN_TRACE_CONTEXT;
#endif

#if !defined(MCGEN_LEVEL_KEYWORD_ENABLED_DEF)
#define MCGEN_LEVEL_KEYWORD_ENABLED_DEF
FORCEINLINE
BOOLEAN
McGenLevelKeywordEnabled(
    _In_ PMCGEN_TRACE_CONTEXT EnableInfo,
    _In_ UCHAR Level,
    _In_ ULONGLONG Keyword
    )
{
    //
    // Check if the event Level is lower than the level at which
    // the channel is enabled.
    // If the event Level is 0 or the channel is enabled at level 0,
    // all levels are enabled.
    //

    if ((Level <= EnableInfo->Level) || // This also covers the case of Level == 0.
        (EnableInfo->Level == 0)) {

        //
        // Check if Keyword is enabled
        //

        if ((Keyword == (ULONGLONG)0) ||
            ((Keyword & EnableInfo->MatchAnyKeyword) &&
             ((Keyword & EnableInfo->MatchAllKeyword) == EnableInfo->MatchAllKeyword))) {
            return TRUE;
        }
    }

    return FALSE;

}
#endif

#if !defined(MCGEN_EVENT_ENABLED_DEF)
#define MCGEN_EVENT_ENABLED_DEF
FORCEINLINE
BOOLEAN
McGenEventEnabled(
    _In_ PMCGEN_TRACE_CONTEXT EnableInfo,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor
    )
{

    return McGenLevelKeywordEnabled(EnableInfo, EventDescriptor->Level, EventDescriptor->Keyword);

}
#endif


//
// EnableCheckMacro
//
#ifndef MCGEN_ENABLE_CHECK
#define MCGEN_ENABLE_CHECK(Context, Descriptor) (Context.IsEnabled &&  McGenEventEnabled(&Context, &Descriptor))
#endif

#if !defined(MCGEN_CONTROL_CALLBACK)
#define MCGEN_CONTROL_CALLBACK

DECLSPEC_NOINLINE __inline
VOID
__stdcall
McGenControlCallbackV2(
    _In_ LPCGUID SourceId,
    _In_ ULONG ControlCode,
    _In_ UCHAR Level,
    _In_ ULONGLONG MatchAnyKeyword,
    _In_ ULONGLONG MatchAllKeyword,
    _In_opt_ PEVENT_FILTER_DESCRIPTOR FilterData,
    _Inout_opt_ PVOID CallbackContext
    )
/*++

Routine Description:

    This is the notification callback for Windows Vista and later.

Arguments:

    SourceId - The GUID that identifies the session that enabled the provider. 

    ControlCode - The parameter indicates whether the provider 
                  is being enabled or disabled.

    Level - The level at which the event is enabled.

    MatchAnyKeyword - The bitmask of keywords that the provider uses to 
                      determine the category of events that it writes.

    MatchAllKeyword - This bitmask additionally restricts the category 
                      of events that the provider writes. 

    FilterData - The provider-defined data.

    CallbackContext - The context of the callback that is defined when the provider 
                      called EtwRegister to register itself.

Remarks:

    ETW calls this function to notify provider of enable/disable

--*/
{
    PMCGEN_TRACE_CONTEXT Ctx = (PMCGEN_TRACE_CONTEXT)CallbackContext;
    ULONG Ix;
#ifndef MCGEN_PRIVATE_ENABLE_CALLBACK_V2
    UNREFERENCED_PARAMETER(SourceId);
    UNREFERENCED_PARAMETER(FilterData);
#endif

    if (Ctx == NULL) {
        return;
    }

    switch (ControlCode) {

        case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
            Ctx->Level = Level;
            Ctx->MatchAnyKeyword = MatchAnyKeyword;
            Ctx->MatchAllKeyword = MatchAllKeyword;
            Ctx->IsEnabled = EVENT_CONTROL_CODE_ENABLE_PROVIDER;

            for (Ix = 0; Ix < Ctx->EnableBitsCount; Ix += 1) {
                if (McGenLevelKeywordEnabled(Ctx, Ctx->EnableLevel[Ix], Ctx->EnableKeyWords[Ix]) != FALSE) {
                    Ctx->EnableBitMask[Ix >> 5] |= (1 << (Ix % 32));
                } else {
                    Ctx->EnableBitMask[Ix >> 5] &= ~(1 << (Ix % 32));
                }
            }
            break;

        case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
            Ctx->IsEnabled = EVENT_CONTROL_CODE_DISABLE_PROVIDER;
            Ctx->Level = 0;
            Ctx->MatchAnyKeyword = 0;
            Ctx->MatchAllKeyword = 0;
            if (Ctx->EnableBitsCount > 0) {
                RtlZeroMemory(Ctx->EnableBitMask, (((Ctx->EnableBitsCount - 1) / 32) + 1) * sizeof(ULONG));
            }
            break;
 
        default:
            break;
    }

#ifdef MCGEN_PRIVATE_ENABLE_CALLBACK_V2
    //
    // Call user defined callback
    //
    MCGEN_PRIVATE_ENABLE_CALLBACK_V2(
        SourceId,
        ControlCode,
        Level,
        MatchAnyKeyword,
        MatchAllKeyword,
        FilterData,
        CallbackContext
        );
#endif
   
    return;
}

#endif
#endif // MCGEN_DISABLE_PROVIDER_CODE_GENERATION
//+
// Provider OrtcLibStatsReport Event Count 6
//+
EXTERN_C __declspec(selectany) const GUID PROVIDER_ORTCLIBSTATSREPORT = {0x12ceb95f, 0x6bcf, 0x4083, {0x90, 0xf1, 0xb7, 0x57, 0xc4, 0x05, 0xc4, 0x61}};

//
// Channel
//
#define CHANNEL_ORTCLIBSTATS 0x10

//
// Tasks
//
#define TASK_STATS 0x1

//
// Event Descriptors
//
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR OrtcStatsReportCommand = {0x6a, 0x0, 0x10, 0x4, 0x1, 0x1, 0x8000000000000000};
#define OrtcStatsReportCommand_value 0x6a
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR StatsReportBool = {0x69, 0x0, 0x10, 0x4, 0x0, 0x1, 0x8000000000000000};
#define StatsReportBool_value 0x69
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR StatsReportFloat = {0x67, 0x0, 0x10, 0x4, 0x0, 0x1, 0x8000000000000000};
#define StatsReportFloat_value 0x67
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR StatsReportInt32 = {0x65, 0x0, 0x10, 0x4, 0x0, 0x1, 0x8000000000000000};
#define StatsReportInt32_value 0x65
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR StatsReportInt64 = {0x66, 0x0, 0x10, 0x4, 0x0, 0x1, 0x8000000000000000};
#define StatsReportInt64_value 0x66
EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR StatsReportString = {0x68, 0x0, 0x10, 0x4, 0x0, 0x1, 0x8000000000000000};
#define StatsReportString_value 0x68

//
// Note on Generate Code from Manifest for Windows Vista and above
//
//Structures :  are handled as a size and pointer pairs. The macro for the event will have an extra 
//parameter for the size in bytes of the structure. Make sure that your structures have no extra padding.
//
//Strings: There are several cases that can be described in the manifest. For array of variable length 
//strings, the generated code will take the count of characters for the whole array as an input parameter. 
//
//SID No support for array of SIDs, the macro will take a pointer to the SID and use appropriate 
//GetLengthSid function to get the length.
//

//
// Allow disabling of code generation
//
#ifndef MCGEN_DISABLE_PROVIDER_CODE_GENERATION

//
// Globals 
//


//
// Event Enablement Bits
//

EXTERN_C __declspec(selectany) DECLSPEC_CACHEALIGN ULONG OrtcLibStatsReportEnableBits[1];
EXTERN_C __declspec(selectany) const ULONGLONG OrtcLibStatsReportKeywords[1] = {0x8000000000000000};
EXTERN_C __declspec(selectany) const UCHAR OrtcLibStatsReportLevels[1] = {4};
EXTERN_C __declspec(selectany) MCGEN_TRACE_CONTEXT PROVIDER_ORTCLIBSTATSREPORT_Context = {0, 0, 0, 0, 0, 0, 0, 0, 1, OrtcLibStatsReportEnableBits, OrtcLibStatsReportKeywords, OrtcLibStatsReportLevels};

EXTERN_C __declspec(selectany) REGHANDLE OrtcLibStatsReportHandle = (REGHANDLE)0;

#if !defined(McGenEventRegisterUnregister)
#define McGenEventRegisterUnregister
#pragma warning(push)
#pragma warning(disable:6103)
DECLSPEC_NOINLINE __inline
ULONG __stdcall
McGenEventRegister(
    _In_ LPCGUID ProviderId,
    _In_opt_ PENABLECALLBACK EnableCallback,
    _In_opt_ PVOID CallbackContext,
    _Inout_ PREGHANDLE RegHandle
    )
/*++

Routine Description:

    This function registers the provider with ETW USER mode.

Arguments:
    ProviderId - Provider ID to be register with ETW.

    EnableCallback - Callback to be used.

    CallbackContext - Context for this provider.

    RegHandle - Pointer to registration handle.

Remarks:

    If the handle != NULL will return ERROR_SUCCESS

--*/
{
    ULONG Error;


    if (*RegHandle) {
        //
        // already registered
        //
        return ERROR_SUCCESS;
    }

    Error = EventRegister( ProviderId, EnableCallback, CallbackContext, RegHandle); 

    return Error;
}
#pragma warning(pop)


DECLSPEC_NOINLINE __inline
ULONG __stdcall
McGenEventUnregister(_Inout_ PREGHANDLE RegHandle)
/*++

Routine Description:

    Unregister from ETW USER mode

Arguments:
            RegHandle this is the pointer to the provider context
Remarks:
            If provider has not been registered, RegHandle == NULL,
            return ERROR_SUCCESS
--*/
{
    ULONG Error;


    if(!(*RegHandle)) {
        //
        // Provider has not registerd
        //
        return ERROR_SUCCESS;
    }

    Error = EventUnregister(*RegHandle); 
    *RegHandle = (REGHANDLE)0;
    
    return Error;
}
#endif
//
// Register with ETW Vista +
//
#ifndef EventRegisterOrtcLibStatsReport
#define EventRegisterOrtcLibStatsReport() McGenEventRegister(&PROVIDER_ORTCLIBSTATSREPORT, McGenControlCallbackV2, &PROVIDER_ORTCLIBSTATSREPORT_Context, &OrtcLibStatsReportHandle) 
#endif

//
// UnRegister with ETW
//
#ifndef EventUnregisterOrtcLibStatsReport
#define EventUnregisterOrtcLibStatsReport() McGenEventUnregister(&OrtcLibStatsReportHandle) 
#endif

//
// Enablement check macro for OrtcStatsReportCommand
//

#define EventEnabledOrtcStatsReportCommand() ((OrtcLibStatsReportEnableBits[0] & 0x00000001) != 0)

//
// Event Macro for OrtcStatsReportCommand
//
#define EventWriteOrtcStatsReportCommand(_subsystem, _function, _line, command_name)\
        EventEnabledOrtcStatsReportCommand() ?\
        Template_ssqs(OrtcLibStatsReportHandle, &OrtcStatsReportCommand, _subsystem, _function, _line, command_name)\
        : ERROR_SUCCESS\

//
// Enablement check macro for StatsReportBool
//

#define EventEnabledStatsReportBool() ((OrtcLibStatsReportEnableBits[0] & 0x00000001) != 0)

//
// Event Macro for StatsReportBool
//
#define EventWriteStatsReportBool(_subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        EventEnabledStatsReportBool() ?\
        Template_ssqsgst(OrtcLibStatsReportHandle, &StatsReportBool, _subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        : ERROR_SUCCESS\

//
// Enablement check macro for StatsReportFloat
//

#define EventEnabledStatsReportFloat() ((OrtcLibStatsReportEnableBits[0] & 0x00000001) != 0)

//
// Event Macro for StatsReportFloat
//
#define EventWriteStatsReportFloat(_subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        EventEnabledStatsReportFloat() ?\
        Template_ssqsgsf(OrtcLibStatsReportHandle, &StatsReportFloat, _subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        : ERROR_SUCCESS\

//
// Enablement check macro for StatsReportInt32
//

#define EventEnabledStatsReportInt32() ((OrtcLibStatsReportEnableBits[0] & 0x00000001) != 0)

//
// Event Macro for StatsReportInt32
//
#define EventWriteStatsReportInt32(_subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        EventEnabledStatsReportInt32() ?\
        Template_ssqsgsd(OrtcLibStatsReportHandle, &StatsReportInt32, _subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        : ERROR_SUCCESS\

//
// Enablement check macro for StatsReportInt64
//

#define EventEnabledStatsReportInt64() ((OrtcLibStatsReportEnableBits[0] & 0x00000001) != 0)

//
// Event Macro for StatsReportInt64
//
#define EventWriteStatsReportInt64(_subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        EventEnabledStatsReportInt64() ?\
        Template_ssqsgsi(OrtcLibStatsReportHandle, &StatsReportInt64, _subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        : ERROR_SUCCESS\

//
// Enablement check macro for StatsReportString
//

#define EventEnabledStatsReportString() ((OrtcLibStatsReportEnableBits[0] & 0x00000001) != 0)

//
// Event Macro for StatsReportString
//
#define EventWriteStatsReportString(_subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        EventEnabledStatsReportString() ?\
        Template_ssqsgss(OrtcLibStatsReportHandle, &StatsReportString, _subsystem, _function, _line, stat_group_name, timestamp, stat_name, stat_value)\
        : ERROR_SUCCESS\

#endif // MCGEN_DISABLE_PROVIDER_CODE_GENERATION


//
// Allow Diasabling of code generation
//
#ifndef MCGEN_DISABLE_PROVIDER_CODE_GENERATION

//
// Template Functions 
//
//
//Template from manifest : T_45fb6a75876bc7576fe103d8fcf3230176b4da1a79f72f0708869827407ad2ec
//
#ifndef Template_ssqs_def
#define Template_ssqs_def
ETW_INLINE
ULONG
Template_ssqs(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR Descriptor,
    _In_opt_ LPCSTR  _Arg0,
    _In_opt_ LPCSTR  _Arg1,
    _In_ const unsigned int  _Arg2,
    _In_opt_ LPCSTR  _Arg3
    )
{
#define ARGUMENT_COUNT_ssqs 4

    EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_ssqs];

    EventDataDescCreate(&EventData[0], 
                        (_Arg0 != NULL) ? _Arg0 : "NULL",
                        (_Arg0 != NULL) ? (ULONG)((strlen(_Arg0) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[1], 
                        (_Arg1 != NULL) ? _Arg1 : "NULL",
                        (_Arg1 != NULL) ? (ULONG)((strlen(_Arg1) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int)  );

    EventDataDescCreate(&EventData[3], 
                        (_Arg3 != NULL) ? _Arg3 : "NULL",
                        (_Arg3 != NULL) ? (ULONG)((strlen(_Arg3) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_ssqs, EventData);
}
#endif

//
//Template from manifest : T_f8f1b0a87398e071d047c26ee257936e1217234bbbfac9e616592a36f9451902
//
#ifndef Template_ssqsgst_def
#define Template_ssqsgst_def
ETW_INLINE
ULONG
Template_ssqsgst(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR Descriptor,
    _In_opt_ LPCSTR  _Arg0,
    _In_opt_ LPCSTR  _Arg1,
    _In_ const unsigned int  _Arg2,
    _In_opt_ LPCSTR  _Arg3,
    _In_ const double  _Arg4,
    _In_opt_ LPCSTR  _Arg5,
    _In_ const BOOL  _Arg6
    )
{
#define ARGUMENT_COUNT_ssqsgst 7

    EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_ssqsgst];

    EventDataDescCreate(&EventData[0], 
                        (_Arg0 != NULL) ? _Arg0 : "NULL",
                        (_Arg0 != NULL) ? (ULONG)((strlen(_Arg0) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[1], 
                        (_Arg1 != NULL) ? _Arg1 : "NULL",
                        (_Arg1 != NULL) ? (ULONG)((strlen(_Arg1) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int)  );

    EventDataDescCreate(&EventData[3], 
                        (_Arg3 != NULL) ? _Arg3 : "NULL",
                        (_Arg3 != NULL) ? (ULONG)((strlen(_Arg3) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const double)  );

    EventDataDescCreate(&EventData[5], 
                        (_Arg5 != NULL) ? _Arg5 : "NULL",
                        (_Arg5 != NULL) ? (ULONG)((strlen(_Arg5) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const BOOL)  );

    return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_ssqsgst, EventData);
}
#endif

//
//Template from manifest : T_55a701fd44054f622c490be66487810ec9745e41048a4422f3bb307f8c193f50
//
#ifndef Template_ssqsgsf_def
#define Template_ssqsgsf_def
ETW_INLINE
ULONG
Template_ssqsgsf(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR Descriptor,
    _In_opt_ LPCSTR  _Arg0,
    _In_opt_ LPCSTR  _Arg1,
    _In_ const unsigned int  _Arg2,
    _In_opt_ LPCSTR  _Arg3,
    _In_ const double  _Arg4,
    _In_opt_ LPCSTR  _Arg5,
    _In_ const float  _Arg6
    )
{
#define ARGUMENT_COUNT_ssqsgsf 7

    EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_ssqsgsf];

    EventDataDescCreate(&EventData[0], 
                        (_Arg0 != NULL) ? _Arg0 : "NULL",
                        (_Arg0 != NULL) ? (ULONG)((strlen(_Arg0) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[1], 
                        (_Arg1 != NULL) ? _Arg1 : "NULL",
                        (_Arg1 != NULL) ? (ULONG)((strlen(_Arg1) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int)  );

    EventDataDescCreate(&EventData[3], 
                        (_Arg3 != NULL) ? _Arg3 : "NULL",
                        (_Arg3 != NULL) ? (ULONG)((strlen(_Arg3) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const double)  );

    EventDataDescCreate(&EventData[5], 
                        (_Arg5 != NULL) ? _Arg5 : "NULL",
                        (_Arg5 != NULL) ? (ULONG)((strlen(_Arg5) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const float)  );

    return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_ssqsgsf, EventData);
}
#endif

//
//Template from manifest : T_4ec711f97742ba23e57441577c652b5c448d48db7175f4a5bc6fb0f8b66496ae
//
#ifndef Template_ssqsgsd_def
#define Template_ssqsgsd_def
ETW_INLINE
ULONG
Template_ssqsgsd(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR Descriptor,
    _In_opt_ LPCSTR  _Arg0,
    _In_opt_ LPCSTR  _Arg1,
    _In_ const unsigned int  _Arg2,
    _In_opt_ LPCSTR  _Arg3,
    _In_ const double  _Arg4,
    _In_opt_ LPCSTR  _Arg5,
    _In_ const signed int  _Arg6
    )
{
#define ARGUMENT_COUNT_ssqsgsd 7

    EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_ssqsgsd];

    EventDataDescCreate(&EventData[0], 
                        (_Arg0 != NULL) ? _Arg0 : "NULL",
                        (_Arg0 != NULL) ? (ULONG)((strlen(_Arg0) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[1], 
                        (_Arg1 != NULL) ? _Arg1 : "NULL",
                        (_Arg1 != NULL) ? (ULONG)((strlen(_Arg1) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int)  );

    EventDataDescCreate(&EventData[3], 
                        (_Arg3 != NULL) ? _Arg3 : "NULL",
                        (_Arg3 != NULL) ? (ULONG)((strlen(_Arg3) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const double)  );

    EventDataDescCreate(&EventData[5], 
                        (_Arg5 != NULL) ? _Arg5 : "NULL",
                        (_Arg5 != NULL) ? (ULONG)((strlen(_Arg5) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[6], &_Arg6, sizeof(const signed int)  );

    return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_ssqsgsd, EventData);
}
#endif

//
//Template from manifest : T_e48af8cf9a56afe4670285b70d5f4fb3bf4531798d3c66557445f02ef171cdc2
//
#ifndef Template_ssqsgsi_def
#define Template_ssqsgsi_def
ETW_INLINE
ULONG
Template_ssqsgsi(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR Descriptor,
    _In_opt_ LPCSTR  _Arg0,
    _In_opt_ LPCSTR  _Arg1,
    _In_ const unsigned int  _Arg2,
    _In_opt_ LPCSTR  _Arg3,
    _In_ const double  _Arg4,
    _In_opt_ LPCSTR  _Arg5,
    _In_ signed __int64  _Arg6
    )
{
#define ARGUMENT_COUNT_ssqsgsi 7

    EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_ssqsgsi];

    EventDataDescCreate(&EventData[0], 
                        (_Arg0 != NULL) ? _Arg0 : "NULL",
                        (_Arg0 != NULL) ? (ULONG)((strlen(_Arg0) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[1], 
                        (_Arg1 != NULL) ? _Arg1 : "NULL",
                        (_Arg1 != NULL) ? (ULONG)((strlen(_Arg1) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int)  );

    EventDataDescCreate(&EventData[3], 
                        (_Arg3 != NULL) ? _Arg3 : "NULL",
                        (_Arg3 != NULL) ? (ULONG)((strlen(_Arg3) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const double)  );

    EventDataDescCreate(&EventData[5], 
                        (_Arg5 != NULL) ? _Arg5 : "NULL",
                        (_Arg5 != NULL) ? (ULONG)((strlen(_Arg5) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[6], &_Arg6, sizeof(signed __int64)  );

    return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_ssqsgsi, EventData);
}
#endif

//
//Template from manifest : T_ae8b629592e4ecba55f25069f4b6a6dd9977918df786a1dadcf103ce2dfa70d4
//
#ifndef Template_ssqsgss_def
#define Template_ssqsgss_def
ETW_INLINE
ULONG
Template_ssqsgss(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR Descriptor,
    _In_opt_ LPCSTR  _Arg0,
    _In_opt_ LPCSTR  _Arg1,
    _In_ const unsigned int  _Arg2,
    _In_opt_ LPCSTR  _Arg3,
    _In_ const double  _Arg4,
    _In_opt_ LPCSTR  _Arg5,
    _In_opt_ LPCSTR  _Arg6
    )
{
#define ARGUMENT_COUNT_ssqsgss 7

    EVENT_DATA_DESCRIPTOR EventData[ARGUMENT_COUNT_ssqsgss];

    EventDataDescCreate(&EventData[0], 
                        (_Arg0 != NULL) ? _Arg0 : "NULL",
                        (_Arg0 != NULL) ? (ULONG)((strlen(_Arg0) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[1], 
                        (_Arg1 != NULL) ? _Arg1 : "NULL",
                        (_Arg1 != NULL) ? (ULONG)((strlen(_Arg1) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[2], &_Arg2, sizeof(const unsigned int)  );

    EventDataDescCreate(&EventData[3], 
                        (_Arg3 != NULL) ? _Arg3 : "NULL",
                        (_Arg3 != NULL) ? (ULONG)((strlen(_Arg3) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[4], &_Arg4, sizeof(const double)  );

    EventDataDescCreate(&EventData[5], 
                        (_Arg5 != NULL) ? _Arg5 : "NULL",
                        (_Arg5 != NULL) ? (ULONG)((strlen(_Arg5) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    EventDataDescCreate(&EventData[6], 
                        (_Arg6 != NULL) ? _Arg6 : "NULL",
                        (_Arg6 != NULL) ? (ULONG)((strlen(_Arg6) + 1) * sizeof(CHAR)) : (ULONG)sizeof("NULL"));

    return EventWrite(RegHandle, Descriptor, ARGUMENT_COUNT_ssqsgss, EventData);
}
#endif

#endif // MCGEN_DISABLE_PROVIDER_CODE_GENERATION

#if defined(__cplusplus)
};
#endif

#define MSG_level_Informational              0x50000004L
#define MSG_Task_Stats                       0x70000001L
#define MSG_Provider                         0x90000001L
#define MSG_Channel_ols                      0x90000002L
#define MSG_Event_StatsReportInt32           0xB0000065L
#define MSG_Event_StatsReportInt64           0xB0000066L
#define MSG_Event_StatsReportFloat           0xB0000067L
#define MSG_Event_StatsReportString          0xB0000068L
#define MSG_Event_StatsReportBool            0xB0000069L
#define MSG_Event_OrtcStatsReportCommand     0xB000006AL
