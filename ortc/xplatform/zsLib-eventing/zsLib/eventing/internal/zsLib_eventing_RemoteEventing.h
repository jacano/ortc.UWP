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

#include <zsLib/eventing/internal/types.h>

#include <zsLib/eventing/IRemoteEventing.h>

#include <zsLib/IPAddress.h>
#include <zsLib/ITimer.h>
#include <zsLib/IWakeDelegate.h>
#include <zsLib/Log.h>
#include <zsLib/Socket.h>

#include <cryptopp/queue.h>

#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_DATA_SIZE                                    "zsLib/eventing/remote-eventing/max-data-size-in-bytes"
#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_PACKED_SIZE                                  "zsLib/eventing/remote-eventing/max-packed-data-size-in-bytes"
#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_OUTSTANDING_EVENTS                           "zsLib/eventing/remote-eventing/max-outstanding-events-in-bytes"
#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_QUEUED_ASYNC_DATA_BEFORED_EVENTS_DROPPED     "zsLib/eventing/remote-eventing/max-queued-async-data-before-events-dropped"
#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_QUEUED_OUTGOING_DATA_BEFORED_EVENTS_DROPPED  "zsLib/eventing/remote-eventing/max-queued-outgoing-data-before-events-dropped"
#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_NOTIFY_TIMER                                     "zsLib/eventing/remote-eventing/notify-timer-in-seconds"
#define ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_USE_IPV6                                         "zsLib/eventing/remote-eventing/use-ipv6"

namespace zsLib
{
  namespace eventing
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteEventingInternalTypes
      #pragma mark
      
      interaction IRemoteEventingInternalTypes
      {
        typedef zsLib::Log::ProviderHandle ProviderHandle;
        typedef zsLib::Log::KeywordBitmaskType KeywordBitmaskType;
        
        struct ProviderInfo
        {
          ProviderHandle mHandle {};
          ProviderHandle mRemoteHandle {};
          bool mSelfRegistered {};
          PUID mRelatedToRemoteEventingObjectID {};
          UUID mProviderID {};
          String mProviderName;
          String mProviderHash;
          KeywordBitmaskType mBitmask {};
        };
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IRemoteEventingAsyncDelegate
      #pragma mark
      
      interaction IRemoteEventingAsyncDelegate : public IRemoteEventingInternalTypes
      {
        ZS_DECLARE_TYPEDEF_PTR(CryptoPP::ByteQueue, ByteQueue);
        typedef zsLib::Log::KeywordBitmaskType KeywordBitmaskType;
        
        virtual void onRemoteEventingSubscribeLogger() = 0;
        virtual void onRemoteEventingUnsubscribeLogger() = 0;
        
        virtual void onRemoteEventingNewSubsystem(const char *subsystemName) = 0;

        virtual void onRemoteEventingProviderRegistered(ProviderInfo *info) = 0;
        virtual void onRemoteEventingProviderUnregistered(ProviderInfo *info) = 0;
        virtual void onRemoteEventingProviderLoggingStateChanged(
                                                                 ProviderInfo *info,
                                                                 KeywordBitmaskType keyword
                                                                 ) = 0;

        virtual void onRemoteEventingWriteEvent(
                                                ByteQueuePtr message,
                                                size_t currentSize
                                                ) = 0;
      };
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing
      #pragma mark

      class RemoteEventing : public MessageQueueAssociator,
                             public IRemoteEventing,
                             public IWakeDelegate,
                             public ITimerDelegate,
                             public ISocketDelegate,
                             public ILogEventingProviderDelegate,
                             public ILogEventingDelegate,
                             public IRemoteEventingAsyncDelegate
      {
      protected:
        struct make_private {};

      public:
        friend interaction IRemoteEventing;
        ZS_DECLARE_TYPEDEF_PTR(CryptoPP::ByteQueue, ByteQueue);
        ZS_DECLARE_STRUCT_PTR(SubsystemInfo);
        typedef zsLib::Log::Level Level;

        typedef zsLib::Log::ProviderHandle ProviderHandle;
        typedef zsLib::Log::EventingAtomDataArray EventingAtomDataArray;
        typedef zsLib::Log::EventingAtomIndex EventingAtomIndex;
        typedef zsLib::Log::KeywordBitmaskType KeywordBitmaskType;

        enum MessageTypes
        {
          MessageType_First           = 1,
          
          MessageType_Hello           = MessageType_First,
          MessageType_Challenge       = 2,
          MessageType_ChallengeReply  = 3,
          MessageType_Welcome         = 4,

          MessageType_Goodbye         = 5,
          
          MessageType_Notify          = 8,
          MessageType_Request         = 16,
          MessageType_RequestAck      = 17,
          
          MessageType_TraceEvent      = 32,
          
          MessageType_Last            = MessageType_TraceEvent
        };
        
        static const char *toString(MessageTypes messageType);
        MessageTypes toMessageType(const char *messageType) throw (InvalidArgument);
        
        struct SubsystemInfo
        {
          String mName;
          Log::Level mLevel {Log::Level_First};
        };
        
        typedef std::set<ProviderInfo *> ProviderInfoSet;
        typedef std::map<UUID, ProviderInfo *> ProviderInfoUUIDMap;
        typedef std::map<ProviderHandle, ProviderInfo *> ProviderInfoHandleMap;
        typedef std::map<String, SubsystemInfoPtr> SubsystemMap;
        typedef std::map<String, KeywordBitmaskType> KeywordLogLevelMap;
        
      public:
        RemoteEventing(
                       const make_private &,
                       IMessageQueuePtr queue,
                       IRemoteEventingDelegatePtr connectionDelegate,
                       const char *connectionSharedSecret,
                       const IPAddress &serverIP,
                       WORD listenPort,
                       Seconds maxWaitToBindTime
                       );
        ~RemoteEventing();

      protected:
        void init();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => IRemoteEventing
        #pragma mark

        static RemoteEventingPtr connectToRemote(
                                                 IRemoteEventingDelegatePtr connectionDelegate,
                                                 const IPAddress &serverIP,
                                                 const char *connectionSharedSecret
                                                 );
        
        static RemoteEventingPtr listenForRemote(
                                                 IRemoteEventingDelegatePtr connectionDelegate,
                                                 WORD localPort,
                                                 const char *connectionSharedSecret,
                                                 Seconds maxWaitToBindTimeInSeconds
                                                 );

        virtual PUID getID() const override { return mID; }

        virtual void shutdown() override;

        virtual States getState() const override;

        virtual void setRemoteLevel(
                                    const char *remoteSubsystemName,
                                    Level level
                                    ) override;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => IWakeDelegate
        #pragma mark

        virtual void onWake() override;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => ITimerDelegate
        #pragma mark

        virtual void onTimer(ITimerPtr timer) override;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => ISocketDelegate
        #pragma mark

        virtual void onReadReady(SocketPtr socket) override;
        virtual void onWriteReady(SocketPtr socket) override;
        virtual void onException(SocketPtr socket) override;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => ILogEventingProviderDelegate
        #pragma mark

        virtual void notifyNewSubsystem(zsLib::Subsystem &inSubsystem) override;
        
        virtual void notifyEventingProviderRegistered(
                                                      ProviderHandle handle,
                                                      EventingAtomDataArray eventingAtomDataArray
                                                      ) override;
        virtual void notifyEventingProviderUnregistered(
                                                        ProviderHandle handle,
                                                        EventingAtomDataArray eventingAtomDataArray
                                                        ) override;

        virtual void notifyEventingProviderLoggingStateChanged(
                                                               ProviderHandle handle,
                                                               EventingAtomDataArray eventingAtomDataArray,
                                                               KeywordBitmaskType keywords
                                                               ) override;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => ILogEventingDelegate
        #pragma mark
        
        // (duplicate) virtual void notifyNewSubsystem(zsLib::Subsystem &inSubsystem) override;
        
        virtual void notifyWriteEvent(
                                      ProviderHandle handle,
                                      EventingAtomDataArray eventingAtomDataArray,
                                      Severity severity,
                                      Level level,
                                      EVENT_DESCRIPTOR_HANDLE descriptor,
                                      EVENT_PARAMETER_DESCRIPTOR_HANDLE parameterDescriptor,
                                      EVENT_DATA_DESCRIPTOR_HANDLE dataDescriptor,
                                      size_t dataDescriptorCount
                                      ) override;
        
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing => IRemoteEventingAsyncDelegate
        #pragma mark

        virtual void onRemoteEventingSubscribeLogger() override;
        virtual void onRemoteEventingUnsubscribeLogger() override;
        virtual void onRemoteEventingNewSubsystem(const char *subsystemName) override;

        virtual void onRemoteEventingProviderRegistered(ProviderInfo *info) override;
        virtual void onRemoteEventingProviderUnregistered(ProviderInfo *info) override;
        virtual void onRemoteEventingProviderLoggingStateChanged(
                                                                 ProviderInfo *info,
                                                                 KeywordBitmaskType keyword
                                                                 ) override;

        virtual void onRemoteEventingWriteEvent(
                                                ByteQueuePtr message,
                                                size_t currentSize
                                                ) override;
        
      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing (internal)
        #pragma mark

        static Log::Params slog(const char *message);
        Log::Params log(const char *message);
        
        bool isShuttingDown() const       { return State_ShuttingDown == mState; }
        bool isShutdown() const           { return State_Shutdown == mState; }
        bool isListeningMode() const      { return 0 != mListenPort; }
        bool isConnectingMode() const     { return 0 == mListenPort; }
        SocketPtr getActiveSocket() const { if (isListeningMode()) return mAcceptedSocket; return mConnectSocket; }
        bool isAuthorized() const         { return MessageType_Welcome == mHandshakeState; }

        void disconnect();
        void cancel();
        void step();
        
        bool stepSocketBind();
        bool stepWaitForAccept();
        
        bool stepSocketConnect();
        bool stepWaitConnected();
        bool stepHello();
        
        bool stepNotifyTimer();
        bool stepAuthorized();
        
        void setState(States state);
        void resetConnection();
        void prepareNewConnection();
        void readIncomingMessage();
        void sendOutgoingData();

        void sendData(
                      MessageTypes messageType,
                      const SecureByteBlock &buffer
                      );
        void sendData(
                      MessageTypes messageType,
                      const ElementPtr &rootEl
                      );
        void sendData(
                      MessageTypes messageType,
                      const std::string &message
                      );
        void sendAck(
                     const String &requestID,
                     int errorNumber = 0,
                     const char *reason = NULL
                     );

        void handleHandshakeMessage(
                                    MessageTypes messageType,
                                    SecureByteBlock &buffer
                                    );
        void handleAuthorizedMessage(
                                     MessageTypes messageType,
                                     SecureByteBlock &buffer
                                     );
        
        void handleHello(const ElementPtr &rootEl);
        void handleChallenge(const ElementPtr &rootEl);
        void handleChallengeReply(const ElementPtr &rootEl);
        void handleWelcome(const ElementPtr &rootEl);
        
        void handleNotify(const ElementPtr &rootEl);
        void handleNotifyGeneralInfo(const ElementPtr &rootEl);
        void handleNotifyRemoteSubsystem(const ElementPtr &rootEl);
        void handleNotifyRemoteProvider(const ElementPtr &rootEl);
        void handleNotifyRemoteProviderKeywordLogging(const ElementPtr &rootEl);
        void handleRequest(const ElementPtr &rootEl);
        void handleRequestAck(const ElementPtr &rootEl);
        
        void handleEvent(SecureByteBlock &buffer);
        
        void sendWelcome();
        void sendNotify();
        void requestSetRemoteSubsystemLevel(SubsystemInfoPtr info);
        void requestSetRemoteEventProviderLogging(
                                                  const String &providerName,
                                                  KeywordBitmaskType bitmask
                                                  );
        void announceProviderToRemote(
                                      ProviderInfo *info,
                                      bool announceNew = true
                                      );
        void announceProviderLoggingStateChangedToRemote(
                                                         ProviderInfo *info,
                                                         KeywordBitmaskType bitmask
                                                         );
        
        void announceSubsystemToRemote(SubsystemInfoPtr info);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark RemoteEventing (data)
        #pragma mark

        mutable RecursiveLock mLock;
        AutoPUID mID;
        RemoteEventingWeakPtr mThisWeak;
        RemoteEventingPtr mGracefulShutdownReference;

        IRemoteEventingDelegatePtr mDelegate;
        
        size_t mMaxDataSize {};
        size_t mMaxPackedSize {};
        size_t mMaxOutstandingEvents {};
        size_t mMaxQueuedAsyncDataBeforeEventsDropped {};
        size_t mMaxQueuedOutgoingDataBeforeEventsDropped {};
        bool mUseIPv6 {};
        
        EventingAtomIndex mEventingAtomIndex {};

        States mState {State_Pending};

        IPAddress mServerIP;
        WORD mListenPort {};
        String mSharedSecret;
        Seconds mMaxWaitToBindTime {};
        Time mBindFailureTime {};
        
        ITimerPtr mRebindTimer;
        SocketPtr mBindSocket;
        SocketPtr mAcceptedSocket;
        IPAddress mRemoteIP;

        SocketPtr mConnectSocket;
        bool mConnected {false};
        
        ITimerPtr mNotifyTimer;
        size_t mAnnouncedLocalDropped {};
        size_t mAnnouncedRemoteDropped {};

        ByteQueue mIncomingQueue;
        ByteQueue mOutgoingQueue;
        bool mWriteReady {false};
        
        MessageTypes mHandshakeState {MessageType_First};
        String mHelloSalt;
        String mExpectingHelloProofInChallenge;
        
        String mChallengeSalt;
        String mExpectingChallengeProofInReply;
        
        bool mFlipEndianInt {false};
        bool mFlipEndianFloat {false};
        
        SubsystemMap mLocalSubsystems;
        SubsystemMap mRemoteSubsystems;
        SubsystemMap mSetRemoteSubsystemsLevels;

        ProviderInfoUUIDMap mLocalAnnouncedProviders;
        ProviderInfoUUIDMap mRemoteRegisteredProvidersByUUID;
        ProviderInfoHandleMap mRemoteRegisteredProvidersByRemoteHandle;
        KeywordLogLevelMap mRequestRemoteProviderKeywordLevel;
        ProviderInfoHandleMap mRequestedRemoteProviderKeywordLevel;

        ProviderInfoSet mCleanUpProviderInfos;

        mutable RecursiveLock mAsyncSelfLock;
        IRemoteEventingAsyncDelegatePtr mAsyncSelf;

        std::atomic<size_t> mTotalDroppedEvents {};
        std::atomic<size_t> mOutstandingEvents {};
        std::atomic<size_t> mEventDataInAsyncQueue {};
        std::atomic<size_t> mEventDataInOutgoingQueue {};
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(zsLib::eventing::internal::IRemoteEventingAsyncDelegate)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::eventing::internal::IRemoteEventingInternalTypes::ProviderInfo, ProviderInfo)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::eventing::internal::IRemoteEventingAsyncDelegate::KeywordBitmaskType, KeywordBitmaskType)
ZS_DECLARE_PROXY_TYPEDEF(zsLib::eventing::internal::IRemoteEventingAsyncDelegate::ByteQueuePtr, ByteQueuePtr)
ZS_DECLARE_PROXY_TYPEDEF(std::size_t, size_t)
ZS_DECLARE_PROXY_METHOD_0(onRemoteEventingSubscribeLogger)
ZS_DECLARE_PROXY_METHOD_0(onRemoteEventingUnsubscribeLogger)
ZS_DECLARE_PROXY_METHOD_1(onRemoteEventingNewSubsystem, const char *)
ZS_DECLARE_PROXY_METHOD_1(onRemoteEventingProviderRegistered, ProviderInfo *)
ZS_DECLARE_PROXY_METHOD_1(onRemoteEventingProviderUnregistered, ProviderInfo *)
ZS_DECLARE_PROXY_METHOD_2(onRemoteEventingProviderLoggingStateChanged, ProviderInfo *, KeywordBitmaskType)
ZS_DECLARE_PROXY_METHOD_2(onRemoteEventingWriteEvent, ByteQueuePtr, size_t)
ZS_DECLARE_PROXY_END()
