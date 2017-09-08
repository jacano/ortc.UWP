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

#include <zsLib/eventing/internal/zsLib_eventing_RemoteEventing.h>

#include <zsLib/eventing/IHelper.h>
#include <zsLib/eventing/IHasher.h>
#include <zsLib/eventing/Log.h>

#include <zsLib/IMessageQueueManager.h>
#include <zsLib/ISettings.h>
#include <zsLib/Numeric.h>
#include <zsLib/Log.h>
#include <zsLib/Socket.h>
#include <zsLib/Singleton.h>

namespace zsLib { namespace eventing { ZS_DECLARE_SUBSYSTEM(zsLib_eventing); } }


#define ZSLIB_EVENTING_REMOTE_EVENTING_MAX_DATA_DESCRIPTORS (80)

#define ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_SUBSYSTEM "subsystem"
#define ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_PROVIDER "provider"
#define ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_PROVIDER_KEYWORD_LOGGING "providerKeywordLogging"
#define ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_GENERAL_INFO "info"

#define ZSLIB_EVENTING_REMOTE_EVENTING_REQUEST_SET_SUBSYSTEM_LEVEL "setSubsystemLevel"
#define ZSLIB_EVENTING_REMOTE_EVENTING_REQUEST_SET_EVENT_PROVIDER_LOGGING "setEventProviderLogging"

namespace zsLib
{
  namespace eventing
  {
    ZS_DECLARE_TYPEDEF_PTR(IHelper, UseEventingHelper);

    namespace internal
    {
      ZS_DECLARE_CLASS_PTR(RemoteEventingSettingsDefaults);

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventingSettingsDefaults
      #pragma mark
      
      class RemoteEventingSettingsDefaults : public ISettingsApplyDefaultsDelegate
      {
      public:
        //-----------------------------------------------------------------------
        ~RemoteEventingSettingsDefaults()
        {
          ISettings::removeDefaults(*this);
        }
        
        //-----------------------------------------------------------------------
        static RemoteEventingSettingsDefaultsPtr singleton()
        {
          static SingletonLazySharedPtr<RemoteEventingSettingsDefaults> singleton(create());
          return singleton.singleton();
        }
        
        //-----------------------------------------------------------------------
        static RemoteEventingSettingsDefaultsPtr create()
        {
          auto pThis(make_shared<RemoteEventingSettingsDefaults>());
          ISettings::installDefaults(pThis);
          return pThis;
        }
        
        //-----------------------------------------------------------------------
        virtual void notifySettingsApplyDefaults() override
        {
          ISettings::setUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_DATA_SIZE, (2*1024));
          ISettings::setUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_PACKED_SIZE, (128*1024));
          ISettings::setUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_OUTSTANDING_EVENTS, (256));
          ISettings::setUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_QUEUED_ASYNC_DATA_BEFORED_EVENTS_DROPPED, (100*1024));
          ISettings::setUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_QUEUED_OUTGOING_DATA_BEFORED_EVENTS_DROPPED, (100*1024));
          ISettings::setUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_NOTIFY_TIMER, 5);
          ISettings::setBool(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_USE_IPV6, false);
        }
      };

      //-------------------------------------------------------------------------
      void installRemoteEventingSettingsDefaults()
      {
        RemoteEventingSettingsDefaults::singleton();
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing (types)
      #pragma mark

      //-----------------------------------------------------------------------
      const char *RemoteEventing::toString(MessageTypes messageType)
      {
        switch (messageType)
        {
          case MessageType_Hello:           return "Hello";
          case MessageType_Welcome:         return "Welcome";
          case MessageType_Challenge:       return "Challenge";
          case MessageType_ChallengeReply:  return "Challenge reply";
          case MessageType_Goodbye:         return "Goodbye";
          case MessageType_Notify:          return "Notify";
          case MessageType_Request:         return "Request";
          case MessageType_RequestAck:      return "Request ack";
          case MessageType_TraceEvent:      return "Trace event";
        }
        
        return "unknown";
      }
      
      //-----------------------------------------------------------------------
      RemoteEventing::MessageTypes RemoteEventing::toMessageType(const char *messageType) throw (InvalidArgument)
      {
        String str(messageType);
        for (RemoteEventing::MessageTypes index = RemoteEventing::MessageType_First; index <= RemoteEventing::MessageType_Last; index = static_cast<RemoteEventing::MessageTypes>(static_cast<std::underlying_type<RemoteEventing::MessageTypes>::type>(index) + 1)) {
          if (0 == str.compareNoCase(RemoteEventing::toString(index))) return index;
        }
        
        ZS_THROW_INVALID_ARGUMENT(String("Not a message type: ") + str);
        return MessageType_First;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing
      #pragma mark

      //-----------------------------------------------------------------------
      RemoteEventing::RemoteEventing(
                                     const make_private &,
                                     IMessageQueuePtr queue,
                                     IRemoteEventingDelegatePtr connectionDelegate,
                                     const char *connectionSharedSecret,
                                     const IPAddress &serverIP,
                                     WORD listenPort,
                                     Seconds maxWaitToBindTime
                                     ) :
        MessageQueueAssociator(queue),
        mDelegate(IRemoteEventingDelegateProxy::createWeak(connectionDelegate)),
        mMaxDataSize(static_cast<decltype(mMaxDataSize)>(ISettings::getUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_DATA_SIZE))),
        mMaxPackedSize(static_cast<decltype(mMaxPackedSize)>(ISettings::getUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_PACKED_SIZE))),
        mMaxOutstandingEvents(static_cast<decltype(mMaxOutstandingEvents)>(ISettings::getUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_OUTSTANDING_EVENTS))),
        mMaxQueuedAsyncDataBeforeEventsDropped(static_cast<decltype(mMaxQueuedAsyncDataBeforeEventsDropped)>(ISettings::getUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_QUEUED_ASYNC_DATA_BEFORED_EVENTS_DROPPED))),
        mMaxQueuedOutgoingDataBeforeEventsDropped(static_cast<decltype(mMaxQueuedOutgoingDataBeforeEventsDropped)>(ISettings::getUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_MAX_QUEUED_OUTGOING_DATA_BEFORED_EVENTS_DROPPED))),
        mUseIPv6(ISettings::getBool(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_USE_IPV6)),
        mServerIP(serverIP),
        mListenPort(listenPort),
        mSharedSecret(connectionSharedSecret),
        mMaxWaitToBindTime(maxWaitToBindTime)
      {
        ZS_LOG_DETAIL(log("Created"));
      }

      //-----------------------------------------------------------------------
      RemoteEventing::~RemoteEventing()
      {
        mThisWeak.reset();
        ZS_LOG_DETAIL(log("Destroyed"));
        cancel();
        
        for (auto iter = mCleanUpProviderInfos.begin(); iter != mCleanUpProviderInfos.end(); ++iter)
        {
          auto info = (*iter);
          Log::EventingAtomDataArray providerArray;
          if (Log::getEventingWriterInfo(info->mHandle, info->mProviderID, info->mProviderName, info->mProviderHash, &providerArray)) {
            providerArray[mEventingAtomIndex] = 0;
          }
          delete info;
        }
        mCleanUpProviderInfos.clear();
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::init()
      {
        mEventingAtomIndex = zsLib::Log::registerEventingAtom("org.zsLib.eventing.RemoteEventing");
        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => IRemoteEventing
      #pragma mark

      //-----------------------------------------------------------------------
      RemoteEventingPtr RemoteEventing::connectToRemote(
                                                        IRemoteEventingDelegatePtr connectionDelegate,
                                                        const IPAddress &serverIP,
                                                        const char *connectionSharedSecret
                                                        )
      {
        auto queue = IMessageQueueManager::getMessageQueue("org.zsLib.eventing.RemoteEventing");
        auto pThis = make_shared<RemoteEventing>(make_private{}, queue, connectionDelegate, connectionSharedSecret, serverIP, static_cast<WORD>(0), Seconds());
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      RemoteEventingPtr RemoteEventing::listenForRemote(
                                                        IRemoteEventingDelegatePtr connectionDelegate,
                                                        WORD localPort,
                                                        const char *connectionSharedSecret,
                                                        Seconds maxWaitToBindTimeInSeconds
                                                        )
      {
        auto queue = IMessageQueueManager::getMessageQueue("org.zsLib.eventing.RemoteEventing");
        auto pThis = make_shared<RemoteEventing>(make_private{}, queue, connectionDelegate, connectionSharedSecret, IPAddress(), localPort, maxWaitToBindTimeInSeconds);
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::shutdown()
      {
        ZS_LOG_DEBUG(log("shutdown called"));

        AutoRecursiveLock lock(mLock);
        cancel();
      }

      //-----------------------------------------------------------------------
      IRemoteEventing::States RemoteEventing::getState() const
      {
        AutoRecursiveLock lock(mLock);
        return mState;
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::setRemoteLevel(
                                          const char *remoteSubsystemName,
                                          Level level
                                          )
      {
        AutoRecursiveLock lock(mLock);
        
        auto info = make_shared<SubsystemInfo>();
        info->mName = String(remoteSubsystemName);
        info->mLevel = level;

        mSetRemoteSubsystemsLevels[info->mName] = info;

        if (!isAuthorized()) return;

        requestSetRemoteSubsystemLevel(info);
      }
      

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => IWakeDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RemoteEventing::onWake()
      {
        ZS_LOG_DEBUG(log("on wake"));

        AutoRecursiveLock lock(mLock);
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RemoteEventing::onTimer(ITimerPtr timer)
      {
        ZS_LOG_DEBUG(log("on timer") + ZS_PARAM("timer id", timer->getID()));
        
        AutoRecursiveLock lock(mLock);
        if (timer == mNotifyTimer) {
          sendNotify();
          return;
        }
        if (mRebindTimer) {
          if (mRebindTimer->getID() == timer->getID()) {
            ZS_LOG_TRACE(log("rebind timer"));
            mRebindTimer->cancel();
            mRebindTimer.reset();
            step();
            return;
          }
        }
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => ISocketDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RemoteEventing::onReadReady(SocketPtr socket)
      {
        AutoRecursiveLock lock(mLock);
        if (socket == mBindSocket) {
          disconnect();
          
          try {
            mAcceptedSocket = mBindSocket->accept(mRemoteIP);
            if (!mAcceptedSocket) {
              ZS_LOG_WARNING(Debug, log("incoming socket rejected"));
              return;
            }
            mAcceptedSocket->setBlocking(false);
            mAcceptedSocket->setDelegate(mThisWeak.lock());
            ZS_LOG_DEBUG(log("incoming socket accepted") + ZS_PARAM("ip", mRemoteIP.string()));
          } catch (const Socket::Exceptions::Unspecified &) {
            ZS_LOG_WARNING(Debug, log("incoming socket rejected"));
            return;
          }
          setState(State_Connecting);
          step();
          return;
        }
        
        auto activeSocket = getActiveSocket();
        if (socket == activeSocket) {
          if (isConnectingMode()) {
            if (!mConnected) {
              ZS_LOG_WARNING(Trace, log("notified read ready before connected"));
              return;
            }
          }
          try {
            BYTE buffer[4096];

            auto read = activeSocket->receive(&(buffer[0]), sizeof(buffer));
            mIncomingQueue.Put(&(buffer[0]), read);
          } catch (const Socket::Exceptions::Unspecified &) {
            ZS_LOG_WARNING(Debug, log("could not read active socket"));
          }
          readIncomingMessage();
          return;
        }

        ZS_LOG_WARNING(Debug, log("read ready on obsolete socket") + ZS_PARAM("socket", static_cast<uintptr_t>(socket->getSocket())));
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::onWriteReady(SocketPtr socket)
      {
        AutoRecursiveLock lock(mLock);
        
        if (socket == mConnectSocket) {
          if (!mConnected) {
            ZS_LOG_DEBUG(log("connecting socket connected"));
            mConnected = true;
            mWriteReady = true;
            step();
            return;
          }
        }

        auto activeSocket = getActiveSocket();
        if (socket == activeSocket) {
          mWriteReady = true;
          sendOutgoingData();
          return;
        }

        ZS_LOG_WARNING(Debug, log("read ready on obsolete socket") + ZS_PARAM("socket", static_cast<uintptr_t>(socket->getSocket())));
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::onException(SocketPtr socket)
      {
        AutoRecursiveLock lock(mLock);
        
        auto activeSocket = getActiveSocket();
        if (socket == activeSocket) {
          ZS_LOG_WARNING(Detail, log("active socket closed"));

          if (isListeningMode()) {
            try {
              mAcceptedSocket->close();
            } catch (const Socket::Exceptions::Unspecified &) {
              ZS_LOG_WARNING(Debug, log("could not read active socket"));
            }
            mAcceptedSocket.reset();
            disconnect();
            return;
          }
          try {
            mConnectSocket->close();
          } catch (const Socket::Exceptions::Unspecified &) {
            ZS_LOG_WARNING(Debug, log("could not read active socket"));
          }
          mConnectSocket.reset();
          disconnect();
          return;
        }
        
        if (socket == mBindSocket) {
          ZS_LOG_WARNING(Detail, log("bind socket failed"));
          cancel();
          return;
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => ILogEventingProviderDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RemoteEventing::notifyNewSubsystem(zsLib::Subsystem &inSubsystem)
      {
        AutoRecursiveLock lock(mAsyncSelfLock);
        if (!mAsyncSelf) return;
        mAsyncSelf->onRemoteEventingNewSubsystem(inSubsystem.getName());
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::notifyEventingProviderRegistered(
                                                            ProviderHandle handle,
                                                            EventingAtomDataArray eventingAtomDataArray
                                                            )
      {
        ProviderInfo *info = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtomIndex]);
        
        bool constructed = false;

        if (info) {
          if (info->mRelatedToRemoteEventingObjectID != mID) {
            // ignore provider infos related to other remote evening engines running
            return;
          }
          if (info->mSelfRegistered) {
            // ignore re-entrant self registered provider infos
            return;
          }
        } else {
          constructed = true;
          info = new ProviderInfo;
          info->mHandle = handle;
          info->mRelatedToRemoteEventingObjectID = mID;
          if (!Log::getEventingWriterInfo(handle, info->mProviderID, info->mProviderName, info->mProviderHash)) {
            ZS_LOG_WARNING(Detail, log("told about provider that does not exist") + ZS_PARAM("provider handle", string(handle)));
            delete info;
            return;
          }
        }
        
        IRemoteEventingAsyncDelegatePtr pAsyncThis;

        {
          AutoRecursiveLock lock(mAsyncSelfLock);
          pAsyncThis = mAsyncSelf;
        }
        
        {
          AutoRecursiveLock lock(mLock);
          if (constructed) {
            eventingAtomDataArray[mEventingAtomIndex] = reinterpret_cast<uintptr_t>(info);
            mCleanUpProviderInfos.insert(info);
          }
          auto found = mRemoteRegisteredProvidersByUUID.find(info->mProviderID);
          if (found != mRemoteRegisteredProvidersByUUID.end()) {
            // ignore any providers announced by the remote party
            info->mSelfRegistered = true;
            return;
          }
        }

        if (!pAsyncThis) return;
        pAsyncThis->onRemoteEventingProviderRegistered(info);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::notifyEventingProviderUnregistered(
                                                              ProviderHandle handle,
                                                              EventingAtomDataArray eventingAtomDataArray
                                                              )
      {
        ProviderInfo *info = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtomIndex]);
        if (!info) return;

        if (info->mRelatedToRemoteEventingObjectID != mID) {
          // ignore provider infos related to other remote evening engines running
          return;
        }

        eventingAtomDataArray[mEventingAtomIndex] = static_cast<uintptr_t>(0);

        if (info->mSelfRegistered) {
          // ignore re-entrant self registered provider infos
          return;
        }

        {
          AutoRecursiveLock lock(mAsyncSelfLock);
          if (!mAsyncSelf) return;
          mAsyncSelf->onRemoteEventingProviderUnregistered(info);
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::notifyEventingProviderLoggingStateChanged(
                                                                     ProviderHandle handle,
                                                                     EventingAtomDataArray eventingAtomDataArray,
                                                                     KeywordBitmaskType keywords
                                                                     )
      {
        ProviderInfo *info = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtomIndex]);
        if (!info) return;
        
        if (info->mRelatedToRemoteEventingObjectID != mID) {
          // ignore provider infos related to other remote evening engines running
          return;
        }

        {
          AutoRecursiveLock lock(mAsyncSelfLock);
          if (!mAsyncSelf) return;
          mAsyncSelf->onRemoteEventingProviderLoggingStateChanged(info, keywords);
        }
      }


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => ILogEventingDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RemoteEventing::notifyWriteEvent(
                                            ProviderHandle handle,
                                            EventingAtomDataArray eventingAtomDataArray,
                                            Severity severity,
                                            Level level,
                                            EVENT_DESCRIPTOR_HANDLE descriptor,
                                            EVENT_PARAMETER_DESCRIPTOR_HANDLE parameterDescriptor,
                                            EVENT_DATA_DESCRIPTOR_HANDLE dataDescriptor,
                                            size_t dataDescriptorCount
                                            )
      {
        ProviderInfo *info = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtomIndex]);
        if (!info) return;

        if (info->mRelatedToRemoteEventingObjectID != mID) {
          // ignore provider infos related to other remote evening engines running
          return;
        }
        if (info->mSelfRegistered) {
          // ignore re-entrant self registered provider infos
          return;
        }
        if (dataDescriptorCount > ZSLIB_EVENTING_REMOTE_EVENTING_MAX_DATA_DESCRIPTORS) {
          ++mTotalDroppedEvents;
          ZS_LOG_WARNING(Debug, log("total descriptors exceed maximum") + ZS_PARAMIZE(dataDescriptorCount));
          return;
        }

        ByteQueuePtr packed(make_shared<ByteQueue>());

        ByteQueue &usePacked = *packed;

        size_t packedSize = (sizeof(CryptoPP::word16)*5) +
                            (sizeof(uint8_t)*4) +
                            (sizeof(uint64_t)*2) +
                            (sizeof(CryptoPP::word16)*dataDescriptorCount) +
                            (sizeof(CryptoPP::word32)*(1+dataDescriptorCount));

        for (size_t index = 0; index < dataDescriptorCount; ++index) {
          auto &data = dataDescriptor[index];
          
          CryptoPP::word32 dataSize = static_cast<CryptoPP::word32>(data.Size);
          if (dataSize > mMaxDataSize) {
            dataSize = static_cast<decltype(dataSize)>(mMaxDataSize);
          }
          if (data.Ptr) {
            packedSize += dataSize;
          }
        }

        if (packedSize > mMaxPackedSize) {
          ++mTotalDroppedEvents;
          ZS_LOG_WARNING(Debug, log("packed size exceeds maximum size") + ZS_PARAMIZE(packedSize));
          return;
        }

        size_t putSize = packedSize + (sizeof(CryptoPP::word32)); // message size not included in packedSize
        usePacked.CreatePutSpace(putSize);
        
        usePacked.PutWord32(static_cast<CryptoPP::word32>(packedSize));
        usePacked.PutWord32(static_cast<CryptoPP::word32>(MessageType_TraceEvent));

        uint64_t data64 {};
        uint64_t eventingHandle {static_cast<uint64_t>(handle)};

        IHelper::setBE64(&data64, eventingHandle);
        usePacked.Put((const BYTE *)(&data64), sizeof(data64));

        usePacked.PutWord16(static_cast<CryptoPP::word16>(severity));
        usePacked.PutWord16(static_cast<CryptoPP::word16>(level));
        usePacked.PutWord16(descriptor->Id);
        usePacked.Put(descriptor->Version);
        usePacked.Put(descriptor->Channel);
        usePacked.Put(descriptor->Level);
        usePacked.Put(descriptor->Opcode);
        usePacked.PutWord16(descriptor->Task);

        IHelper::setBE64(&data64, descriptor->Keyword);
        usePacked.Put((const BYTE *)(&data64), sizeof(data64));

        usePacked.PutWord16(static_cast<CryptoPP::word16>(dataDescriptorCount));

        for (size_t index = 0; index < dataDescriptorCount; ++index) {
          auto &param = parameterDescriptor[index];
          usePacked.PutWord16(static_cast<uint16_t>(param.Type));
        }

        for (size_t index = 0; index < dataDescriptorCount; ++index) {
          auto &data = dataDescriptor[index];
          
          uint32_t dataSize = static_cast<uint32_t>(data.Size);
          if (dataSize > mMaxDataSize) {
            dataSize = static_cast<decltype(dataSize)>(mMaxDataSize);
          }
          
          if (data.Ptr) {
            bool endianFlip {true};

            switch (parameterDescriptor[index].Type) {
              case EventParameterType_Boolean:
              case EventParameterType_UnsignedInteger:
              case EventParameterType_SignedInteger:
              case EventParameterType_Pointer:
              case EventParameterType_FloatingPoint:  {
                dataSize |= (1 << 31);
                break;
              }
              default:                                {
                endianFlip = false;
                break;
              }
            }

            usePacked.PutWord32(dataSize);

            if (endianFlip) {
              switch (data.Size) {
                case 2:  {
                  uint16_t value {};
                  memcpy(&value, (const void *)(data.Ptr), sizeof(value));
                  usePacked.PutWord16(value);
                  break;
                }
                case 4:  {
                  uint32_t value {};
                  memcpy(&value, (const void *)(data.Ptr), sizeof(value));
                  usePacked.PutWord32(value);
                  break;
                }
                case 8:  {
                  memcpy(&data64, (const void *)(data.Ptr), sizeof(data64));
                  IHelper::setBE64(&data64, data64);
                  usePacked.Put((const BYTE *)(&data64), sizeof(data64));
                  break;
                }
                default: {
                  // just put in raw format
                  usePacked.Put((const BYTE *)(data.Ptr), data.Size);
                }
              }
            } else {
              usePacked.Put((const BYTE *)(data.Ptr), data.Size);
            }
          } else {
            usePacked.PutWord32(static_cast<CryptoPP::word32>(0));
          }
        }

        {
          AutoRecursiveLock lock(mAsyncSelfLock);
          if (!mAsyncSelf) return;
          if (mOutstandingEvents > mMaxOutstandingEvents) {
            ++mTotalDroppedEvents;
            ZS_LOG_WARNING(Insane, log("too many outstanding events (event dropped)") + ZS_PARAM("events", mOutstandingEvents));
            return;
          }
          
          size_t currentSize = static_cast<decltype(currentSize)>(packed->CurrentSize());

          if (mEventDataInAsyncQueue + currentSize > mMaxQueuedAsyncDataBeforeEventsDropped) {
            ++mTotalDroppedEvents;
            ZS_LOG_WARNING(Insane, log("too many outstanding events (event dropped)") + ZS_PARAM("in queue", mEventDataInAsyncQueue) + ZS_PARAM("size", currentSize));
            return;
          }

          ++mOutstandingEvents;
          mEventDataInAsyncQueue += currentSize;
          mAsyncSelf->onRemoteEventingWriteEvent(packed, currentSize);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => IRemoteEventingAsyncDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingSubscribeLogger()
      {
        auto pThis = mThisWeak.lock();
        ZS_THROW_BAD_STATE_IF(!pThis);

        {
          AutoRecursiveLock lock(mAsyncSelfLock);
          mAsyncSelf = IRemoteEventingAsyncDelegateProxy::create(pThis);
          ZS_THROW_BAD_STATE_IF(!mAsyncSelf);
        }

        Log::addEventingProviderListener(pThis);
        Log::addEventingListener(pThis);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingUnsubscribeLogger()
      {
        if (!mAsyncSelf) return;
        
        auto pThis = mThisWeak.lock();
        ZS_THROW_BAD_STATE_IF(!pThis);
        
        Log::removeEventingProviderListener(pThis);
        Log::removeEventingListener(pThis);

        {
          AutoRecursiveLock lock(mAsyncSelfLock);
          mAsyncSelf.reset();
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingNewSubsystem(const char *subsystemName)
      {
        String subsystemStr(subsystemName);
        
        {
          AutoRecursiveLock lock(mLock);
          auto found = mLocalSubsystems.find(subsystemStr);
          if (found != mLocalSubsystems.end()) {
            ZS_LOG_TRACE(log("subsystem already announced") + ZS_PARAMIZE(subsystemStr));
            return;
          }
          
          auto info = make_shared<SubsystemInfo>();
          info->mName = subsystemStr;

          mLocalSubsystems[info->mName] = info;
          if (isAuthorized()) {
            announceSubsystemToRemote(info);
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingProviderRegistered(ProviderInfo *provider)
      {
        AutoRecursiveLock lock(mLock);
        auto found = mLocalAnnouncedProviders.find(provider->mProviderID);
        if (found != mLocalAnnouncedProviders.end()) {
          ZS_LOG_DEBUG(log("local provider already announced") + ZS_PARAM("provider", provider->mProviderName));
          return;
        }

        mLocalAnnouncedProviders[provider->mProviderID] = provider;
        announceProviderToRemote(provider);
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingProviderUnregistered(ProviderInfo *provider)
      {
        AutoRecursiveLock lock(mLock);
        auto found = mLocalAnnouncedProviders.find(provider->mProviderID);
        if (found == mLocalAnnouncedProviders.end()) {
          ZS_LOG_DEBUG(log("local provider not announced") + ZS_PARAM("provider", provider->mProviderName));
          return;
        }
        
        mLocalAnnouncedProviders.erase(found);
        announceProviderToRemote(provider, false);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingProviderLoggingStateChanged(
                                                                       ProviderInfo *provider,
                                                                       KeywordBitmaskType keywords
                                                                       )
      {
        AutoRecursiveLock lock(mLock);

        auto found = mLocalAnnouncedProviders.find(provider->mProviderID);
        if (found != mLocalAnnouncedProviders.end()) {
          if (isAuthorized()) {
            announceProviderLoggingStateChangedToRemote(provider, keywords);
          }
          return;
        }

        if (!isAuthorized()) {
          mRequestRemoteProviderKeywordLevel[provider->mProviderName] = keywords;
          return;
        }

        bool requested = false;
        for (auto iter = mRemoteRegisteredProvidersByUUID.begin(); iter != mRemoteRegisteredProvidersByUUID.end(); ++iter) {
          auto checkProvider = (*iter).second;
          if (provider->mProviderName == checkProvider->mProviderName) {
            requestSetRemoteEventProviderLogging(provider->mProviderName, keywords);
            requested = true;
          }
        }
        
        if (!requested) {
          mRequestRemoteProviderKeywordLevel[provider->mProviderName] = keywords;
          return;
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::onRemoteEventingWriteEvent(
                                                      ByteQueuePtr message,
                                                      size_t currentSize
                                                      )
      {
        --mOutstandingEvents;
        mEventDataInAsyncQueue -= currentSize;
        
        if (mEventDataInOutgoingQueue + currentSize > mMaxQueuedOutgoingDataBeforeEventsDropped) {
          ++mTotalDroppedEvents;
          ZS_LOG_WARNING(Trace, log("too much data in outgoing queue (event dropped)"));
          return;
        }

        AutoRecursiveLock lock(mLock);
        if (!isAuthorized()) {
          ++mTotalDroppedEvents;
          ZS_LOG_WARNING(Insane, log("ignoring event as not in authorized connection state (event dropped)"));
          return;
        }

        mEventDataInOutgoingQueue += currentSize;
        message->TransferTo(mOutgoingQueue);

        if (mWriteReady) {
          sendOutgoingData();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark RemoteEventing => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      Log::Params RemoteEventing::slog(const char *message)
      {
        return Log::Params(message, "eventing::RemoteEventing");
      }

      //-----------------------------------------------------------------------
      Log::Params RemoteEventing::log(const char *message)
      {
        ElementPtr objectEl = Element::create("eventing::RemoteEventing");
        objectEl->adoptAsLastChild(UseEventingHelper::createElementWithNumber("id", string(mID)));
        return Log::Params(message, objectEl);
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::disconnect()
      {
        auto pThis = mThisWeak.lock();
        if (!pThis) {
          ZS_LOG_WARNING(Detail, log("disconnect as pThis is null (shutting down)"));
          cancel();
          return;
        }
        
        IRemoteEventingAsyncDelegateProxy::create(pThis)->onRemoteEventingUnsubscribeLogger();
        
        if (!isListeningMode()) {
          ZS_LOG_DEBUG(log("disconnecting forwarding to cancel"));
          cancel();
          return;
        }

        if (isShuttingDown()) {
          ZS_LOG_TRACE(log("disconnect forwarding to shutdown"));
          cancel();
          return;
        }

        if (mAcceptedSocket) {
          if (MessageType_Welcome == mHandshakeState) {
            sendData(MessageType_Goodbye, SecureByteBlock());
            mHandshakeState = MessageType_Goodbye;
          }

          ZS_LOG_TRACE(log("disconnecting accepted socket"));
          try {
            mAcceptedSocket->close();
          } catch (const Socket::Exceptions::Unspecified &) {
            ZS_LOG_WARNING(Debug, log("could not read active socket"));
          }
          mAcceptedSocket.reset();
        }

        prepareNewConnection();
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::cancel()
      {
        ZS_LOG_TRACE(log("cancel called"));
        
        if (isShutdown()) {
          ZS_LOG_TRACE(log("already shutdown"));
          return;
        }
        
        setState(State_ShuttingDown);
        
        for (auto iter = mRemoteRegisteredProvidersByUUID.begin(); iter != mRemoteRegisteredProvidersByUUID.end(); ++iter) {
          auto provider = (*iter).second;
          Log::setEventingLogging(provider->mHandle, mID, false);
          Log::unregisterEventingWriter(provider->mHandle);
        }
        
        mRemoteRegisteredProvidersByUUID.clear();
        mRemoteRegisteredProvidersByRemoteHandle.clear();

        auto pThis = mThisWeak.lock();
        mGracefulShutdownReference = pThis;

        if (mGracefulShutdownReference) {
          IRemoteEventingAsyncDelegateProxy::create(pThis)->onRemoteEventingUnsubscribeLogger();

          if (MessageType_Welcome == mHandshakeState) {
            sendData(MessageType_Goodbye, SecureByteBlock());
            mHandshakeState = MessageType_Goodbye;
          }

          if (mOutgoingQueue.AnyRetrievable()) {
            auto activeSocket = getActiveSocket();
            if (activeSocket) {
              ZS_LOG_TRACE(log("waiting until shutdown"));
              return;
            }
          }
        }
        
        resetConnection();

        try {
          if (mAcceptedSocket) mAcceptedSocket->close();
        } catch (const Socket::Exceptions::Unspecified &) {
          ZS_LOG_WARNING(Debug, log("failed to close accepted socket"));
        }
        try {
          if (mBindSocket) mBindSocket->close();
        } catch (const Socket::Exceptions::Unspecified &) {
          ZS_LOG_WARNING(Debug, log("failed to close bind socket"));
        }
        try {
          if (mConnectSocket) mConnectSocket->close();
        } catch (const Socket::Exceptions::Unspecified &) {
          ZS_LOG_WARNING(Debug, log("failed to close connect socket"));
        }
        
        mAcceptedSocket.reset();
        mBindSocket.reset();
        mConnectSocket.reset();

        if (mRebindTimer) {
          mRebindTimer->cancel();
          mRebindTimer.reset();
        }
        
        setState(State_Shutdown);
        
        mGracefulShutdownReference.reset();
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::step()
      {
        ZS_LOG_DEBUG(log("step"));
        
        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_TRACE(log("step forwarding to cancel"));
          return;
        }
        
        {
          if (isListeningMode()) {
            if (!stepSocketBind()) return;
            if (!stepWaitForAccept()) return;
          } else {
            if (!stepSocketConnect()) return;
            if (!stepWaitConnected()) return;
            if (!stepHello()) return;
          }
          if (!stepNotifyTimer()) return;
          if (!stepAuthorized()) return;
        }

        setState(State_Connected);
      }

      //-----------------------------------------------------------------------
      bool RemoteEventing::stepSocketBind()
      {
        if (mBindSocket) {
          ZS_LOG_TRACE(log("step - already bound"));
          return true;
        }
        
        if (mRebindTimer) {
          ZS_LOG_TRACE(log("step - waiting for rebind timer"));
          return true;
        }
        
        ZS_LOG_DEBUG(log("step - attempting to bind"));
        
        IPAddress bindIP(mUseIPv6 ? IPAddress::anyV6() : IPAddress::anyV4(), mListenPort);

        if (Time() == mBindFailureTime) {
          mBindFailureTime = zsLib::now() + mMaxWaitToBindTime;
        }

        try {
          mBindSocket = Socket::createTCP(mUseIPv6 ? Socket::Create::Family::IPv6 : Socket::Create::Family::IPv4);
          mBindSocket->setBlocking(false);
          mBindSocket->setDelegate(mThisWeak.lock());
          mBindSocket->bind(bindIP);
          mBindSocket->listen();
        } catch (const Socket::Exceptions::Unspecified &) {
          auto tick = zsLib::now();
          if (tick > mBindFailureTime) {
            ZS_LOG_ERROR(Detail, log("failed to bind to socket") + ZS_PARAM("port", string(mListenPort)));
            cancel();
            return false;
          }
          
          ZS_LOG_WARNING(Debug, log("failed to bind (will attempt again in 1 second)"));
          mRebindTimer = ITimer::create(mThisWeak.lock(), Seconds(1), false);
          try {
            if (mBindSocket) mBindSocket->close();
          } catch (const Socket::Exceptions::Unspecified &) {
            ZS_LOG_WARNING(Debug, log("failed to close listen socket"));
          }
          mBindSocket.reset();
          return false;
        }

        mBindFailureTime = Time();

        setState(State_Listening);
        return true;
      }
      
      //-----------------------------------------------------------------------
      bool RemoteEventing::stepWaitForAccept()
      {
        if (!mAcceptedSocket) {
          ZS_LOG_TRACE(log("step - waiting for socket to accept"));
          return false;
        }
        
        ZS_LOG_TRACE(log("step - have accepted socket"));
        return true;
      }
      
      //-----------------------------------------------------------------------
      bool RemoteEventing::stepSocketConnect()
      {
        if (mConnectSocket) {
          ZS_LOG_TRACE(log("step - already have a socket connecting"));
          return true;
        }
        
        ZS_LOG_DEBUG(log("conencting"));

        try {
          mConnectSocket = Socket::createTCP(mServerIP.isIPv4() ? Socket::Create::Family::IPv4 : Socket::Create::Family::IPv6);
          mConnectSocket->setBlocking(false);
          mConnectSocket->setDelegate(mThisWeak.lock());
          bool wouldBlock = false;
          mConnectSocket->connect(mServerIP, &wouldBlock);
        } catch (const Socket::Exceptions::Unspecified &) {
          ZS_LOG_WARNING(Detail, log("failed to connect (shutting down)"));
          disconnect();
          return false;
        }
        
        setState(State_Connecting);

        return true;
      }

      //-----------------------------------------------------------------------
      bool RemoteEventing::stepWaitConnected()
      {
        if (!mConnected) {
          ZS_LOG_TRACE(log("step waiting to connect"));
          return false;
        }
        
        ZS_LOG_TRACE(log("step - connected"));
        return true;
      }
      
      //-----------------------------------------------------------------------
      bool RemoteEventing::stepHello()
      {
        if (MessageType_Hello != mHandshakeState) {
          ZS_LOG_TRACE(log("step - skipping hello"));
          return true;
        }

        ElementPtr rootEl = Element::create("hello");

        mHelloSalt = IHelper::randomString(IHasher::sha256DigestSize()*8/5);
        rootEl->adoptAsFirstChild(IHelper::createElementWithNumber("version", "1"));
        rootEl->adoptAsFirstChild(IHelper::createElementWithText("salt", mHelloSalt));

        String helloProof = IHasher::hashAsString("hello:proof:" + mSharedSecret + ":" + mHelloSalt, IHasher::sha256());
        rootEl->adoptAsFirstChild(IHelper::createElementWithText("proof", helloProof));

        mExpectingHelloProofInChallenge = IHasher::hashAsString("hello:expecting:" + mSharedSecret + ":" + mHelloSalt, IHasher::sha256());
        mHandshakeState = MessageType_Challenge;
        sendData(MessageType_Hello, rootEl);
        return false;
      }
      
      //-----------------------------------------------------------------------
      bool RemoteEventing::stepNotifyTimer()
      {
        if (mNotifyTimer) {
          ZS_LOG_TRACE(log("step - already have notify timer"));
          return true;
        }
        
        Seconds notifyTimeout(ISettings::getUInt(ZSLIB_EVENTING_SETTING_REMOTE_EVENTING_NOTIFY_TIMER));
        if (notifyTimeout < Seconds(1)) {
          notifyTimeout = Seconds(1);
        }

        mNotifyTimer = ITimer::create(mThisWeak.lock(), notifyTimeout);
        return true;
      }

      //-----------------------------------------------------------------------
      bool RemoteEventing::stepAuthorized()
      {
        if (!isAuthorized()) {
          ZS_LOG_TRACE(log("step - not authorized"));
          return false;
        }

        ZS_LOG_TRACE(log("step - authorized"));
        return true;
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::setState(States state)
      {
        if (state == mState) return;
        
        ZS_LOG_DEBUG(log("state changed") + ZS_PARAM("old state", IRemoteEventingTypes::toString(mState)) + ZS_PARAM("new state", state));
        
        mState = state;

        auto pThis = mThisWeak.lock();
        if (pThis) {
          if (mDelegate) {
            try {
              mDelegate->onRemoteEventingStateChanged(pThis, mState);
            } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone (probably okay)"));
              mDelegate.reset();
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::resetConnection()
      {
        if (mNotifyTimer) {
          mNotifyTimer->cancel();
          mNotifyTimer.reset();
        }
        for (auto iter = mRequestedRemoteProviderKeywordLevel.begin(); iter != mRequestedRemoteProviderKeywordLevel.end(); ++iter)
        {
          auto provider = (*iter).second;
          Log::setEventingLogging(provider->mHandle, mID, false);
        }
        mRequestedRemoteProviderKeywordLevel.clear();
        mRequestRemoteProviderKeywordLevel.clear();

        mAnnouncedLocalDropped = 0;
        mAnnouncedRemoteDropped = 0;
        mTotalDroppedEvents = 0;
        mIncomingQueue.Clear();
        mOutgoingQueue.Clear();
        mEventDataInOutgoingQueue = 0;

        mHelloSalt.clear();
        mExpectingHelloProofInChallenge.clear();
        mChallengeSalt.clear();
        mExpectingChallengeProofInReply.clear();
        
        mFlipEndianInt = false;
        mFlipEndianFloat = false;
        
        mRemoteSubsystems.clear();
        for (auto iter = mRemoteRegisteredProvidersByUUID.begin(); iter != mRemoteRegisteredProvidersByUUID.end(); ++iter) {
          auto provider = (*iter).second;
          Log::setEventingLogging(provider->mHandle, mID, false);
          Log::unregisterEventingWriter(provider->mHandle);
        }
        mRemoteRegisteredProvidersByUUID.clear();
        mRemoteRegisteredProvidersByRemoteHandle.clear();
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::prepareNewConnection()
      {
        resetConnection();
        mHandshakeState = MessageType_First;
        setState(State_Listening);
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::readIncomingMessage()
      {
        while ((mIncomingQueue.AnyRetrievable()) &&
               (MessageType_Goodbye != mHandshakeState))
        {
          auto available = mIncomingQueue.CurrentSize();

          CryptoPP::word32 messageSize{};
          if (available < sizeof(messageSize)) {
            ZS_LOG_INSANE(log("insufficient read size for next message") + ZS_PARAM("size", available));
            break;
          }

          // message size does include the size of the message type
          mIncomingQueue.PeekWord32(messageSize);
          if (available < sizeof(messageSize) + messageSize) {
            ZS_LOG_INSANE(log("insufficient read size for next message") + ZS_PARAM("available", available) + ZS_PARAM("size", messageSize));
            break;
          }

          mIncomingQueue.Skip(sizeof(messageSize));

          CryptoPP::word32 messageType {};
          if (messageSize < sizeof(messageType)) {
            ZS_LOG_WARNING(Detail, log("illegal message size (disconnecting)"));
            disconnect();
            return;
          }

          mIncomingQueue.GetWord32(messageType);
          messageSize -= sizeof(messageType);

          SecureByteBlock buffer(messageSize);
          mIncomingQueue.Get(buffer, messageSize);

          if (MessageType_Welcome == mHandshakeState) {
            handleAuthorizedMessage(static_cast<MessageTypes>(messageType), buffer);
          } else {
            handleHandshakeMessage(static_cast<MessageTypes>(messageType), buffer);
          }
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::sendOutgoingData()
      {
        if (!mWriteReady) {
          ZS_LOG_INSANE(log("waiting for write ready to be able to send"));
          return;
        }
        
        if (!mOutgoingQueue.AnyRetrievable()) {
          ZS_LOG_INSANE(log("no data available to send"));
          return;
        }
        
        auto activeSocket = getActiveSocket();
        if (!activeSocket) {
          ZS_LOG_INSANE(log("no socket available to send"));
          return;
        }
        
        try {
          while (mWriteReady) {
            BYTE buffer[4096];

            size_t availeable = mEventDataInOutgoingQueue;
            if (availeable < 1) break;
#ifdef _DEBUG
            ZS_THROW_BAD_STATE_IF(availeable != mOutgoingQueue.CurrentSize())
#endif //_DEBUG
            if (availeable > sizeof(buffer)) {
              availeable = sizeof(buffer);
            }

            mOutgoingQueue.Peek(&(buffer[0]), availeable);

            bool wouldBlock = false;
            auto written = activeSocket->send(&(buffer[0]), availeable, &wouldBlock);

            mOutgoingQueue.Skip(written);
            mEventDataInOutgoingQueue -= static_cast<size_t>(written);
            if (wouldBlock) mWriteReady = false;
          }
        } catch (const Socket::Exceptions::Unspecified &) {
          ZS_LOG_WARNING(Debug, log("could not write to active socket"));
        }

        if (isShuttingDown()) {
          ZS_LOG_TRACE(log("step after write ready"));
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::sendData(
                                    MessageTypes messageType,
                                    const SecureByteBlock &buffer
                                    )
      {
        CryptoPP::word32 type = static_cast<CryptoPP::word32>(messageType);
        CryptoPP::word32 size = static_cast<CryptoPP::word32>(sizeof(type) + buffer.SizeInBytes());
        
        mOutgoingQueue.PutWord32(size);
        mOutgoingQueue.PutWord32(type);
        mOutgoingQueue.Put(buffer, buffer.SizeInBytes());
        
        mEventDataInOutgoingQueue += static_cast<size_t>((sizeof(uint32_t)*2) + buffer.SizeInBytes());
        
        if (mWriteReady) {
          sendOutgoingData();
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::sendData(
                                    MessageTypes messageType,
                                    const ElementPtr &rootEl
                                    )
      {
        auto message = IHelper::toString(rootEl);
        sendData(messageType, message);
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::sendData(
                                    MessageTypes messageType,
                                    const std::string &message
                                    )
      {
        CryptoPP::word32 type = static_cast<CryptoPP::word32>(messageType);
        CryptoPP::word32 size = static_cast<CryptoPP::word32>(sizeof(type) + message.length());
        
        mOutgoingQueue.PutWord32(size);
        mOutgoingQueue.PutWord32(type);
        mOutgoingQueue.Put(reinterpret_cast<const BYTE *>(message.c_str()), message.length());

        mEventDataInOutgoingQueue += static_cast<size_t>((sizeof(uint32_t)*2) + message.length());

        if (mWriteReady) {
          sendOutgoingData();
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::sendAck(
                                   const String &requestID,
                                   int errorNumber,
                                   const char *reason
                                   )
      {
        auto rootEl = Element::create("ack");
        
        rootEl->adoptAsLastChild(IHelper::createElementWithText("id", requestID));
        if (0 != errorNumber) {
          rootEl->adoptAsLastChild(IHelper::createElementWithNumber("error", string(errorNumber)));
          String reasonStr(reason);
          if (reasonStr.hasData()) {
            rootEl->adoptAsLastChild(IHelper::createElementWithText("reason", reasonStr));
          }
        }

        sendData(MessageType_RequestAck, rootEl);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleHandshakeMessage(
                                                  MessageTypes messageType,
                                                  SecureByteBlock &buffer
                                                  )
      {
        if (MessageType_Goodbye == messageType) {
          ZS_LOG_DEBUG(log("received goodbye during handshake"));
          disconnect();
          return;
        }
        
        if (buffer.SizeInBytes() < 1) {
          ZS_LOG_WARNING(Detail, log("message size not legal (shutting down)") + ZS_PARAM("type", string(messageType)));
          disconnect();
          return;
        }

        const char *message = reinterpret_cast<const char *>(buffer.BytePtr());
        ElementPtr rootEl = IHelper::toJSON(message);
        if (!rootEl) {
          ZS_LOG_WARNING(Detail, log("message not legal (disconnecting)") + ZS_PARAM("type", string(messageType)));
          disconnect();
          return;
        }

        switch (messageType) {
          case MessageType_Hello:           handleHello(rootEl); break;
          case MessageType_Challenge:       handleChallenge(rootEl); break;
          case MessageType_ChallengeReply:  handleChallengeReply(rootEl); break;
          case MessageType_Welcome:         handleWelcome(rootEl); break;
          default: {
            ZS_LOG_WARNING(Detail, log("message type not understood (disconnecting)") + ZS_PARAM("type", string(messageType)));
            disconnect();
            break;
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleAuthorizedMessage(
                                                   MessageTypes messageType,
                                                   SecureByteBlock &buffer
                                                   )
      {
        switch (messageType) {
          case MessageType_TraceEvent: {
            handleEvent(buffer);
            return;
          }
          case MessageType_Goodbye: {
            ZS_LOG_DEBUG(log("received goodbye"));
            disconnect();
            return;
          }
          default: {
            break;
          }
        }
        
        if (buffer.SizeInBytes() < 1) {
          ZS_LOG_WARNING(Detail, log("message size not legal (disconnecting)") + ZS_PARAM("type", string(messageType)));
          disconnect();
          return;
        }
        
        const char *message = reinterpret_cast<const char *>(buffer.BytePtr());
        ElementPtr rootEl = IHelper::toJSON(message);
        if (!rootEl) {
          ZS_LOG_WARNING(Detail, log("message not legal (disconnecting)") + ZS_PARAM("type", string(messageType)));
          disconnect();
          return;
        }

        switch (messageType) {
          case MessageType_Notify:      handleNotify(rootEl); break;
          case MessageType_Request:     handleRequest(rootEl); break;
          case MessageType_RequestAck:  handleRequestAck(rootEl); break;
          default: {
            ZS_LOG_WARNING(Detail, log("message type not understood (disconnecting)") + ZS_PARAM("type", string(messageType)));
            disconnect();
            break;
          }
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleHello(const ElementPtr &rootEl)
      {
        if ((MessageType_Hello != mHandshakeState) ||
            (isConnectingMode())) {
          ZS_LOG_WARNING(Detail, log("received hello but not waiting for hello"));
          disconnect();
          return;
        }

        mHelloSalt = IHelper::getElementText(rootEl->findFirstChildElement("salt"));
        if (mHelloSalt.isEmpty()) {
          ZS_LOG_WARNING(Detail, log("received hello but missing salt"));
          disconnect();
          return;
        }
        
        String helloProof = IHelper::getElementText(rootEl->findFirstChildElement("proof"));
        String expectingHelloProof = IHasher::hashAsString("hello:proof:" + mSharedSecret + ":" + mHelloSalt, IHasher::sha256());
        
        if (helloProof != expectingHelloProof) {
          ZS_LOG_WARNING(Detail, log("hello proof does not match expected value") + ZS_PARAMIZE(helloProof) + ZS_PARAMIZE(expectingHelloProof));
          disconnect();
          return;
        }

        mHandshakeState = MessageType_Challenge;
        
        mExpectingHelloProofInChallenge = IHasher::hashAsString("hello:expecting:" + mSharedSecret + ":" + mHelloSalt, IHasher::sha256());

        ElementPtr challengeEl = Element::create("challenge");

        mChallengeSalt = IHelper::randomString(IHasher::sha256DigestSize() * 8 / 5);
        challengeEl->adoptAsFirstChild(IHelper::createElementWithNumber("version", "1"));
        challengeEl->adoptAsFirstChild(IHelper::createElementWithText("salt", mChallengeSalt));
        challengeEl->adoptAsFirstChild(IHelper::createElementWithText("proof", mExpectingHelloProofInChallenge));

        mExpectingChallengeProofInReply = IHasher::hashAsString("challenge:expecting:" + mSharedSecret + ":" + mHelloSalt + ":" + mChallengeSalt, IHasher::sha256());
        sendData(MessageType_Challenge, challengeEl);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleChallenge(const ElementPtr &rootEl)
      {
        if ((MessageType_Challenge != mHandshakeState) ||
            (mChallengeSalt.hasData())) {
          ZS_LOG_WARNING(Detail, log("received challenge but not waiting for challenge"));
          disconnect();
          return;
        }

        mChallengeSalt = IHelper::getElementText(rootEl->findFirstChildElement("salt"));
        String proof = IHelper::getElementText(rootEl->findFirstChildElement("proof"));
        
        if (proof != mExpectingHelloProofInChallenge) {
          ZS_LOG_WARNING(Detail, log("received challenge but proof does not match") + ZS_PARAMIZE(proof) + ZS_PARAMIZE(mExpectingHelloProofInChallenge));
          disconnect();
          return;
        }

        ElementPtr challengeReplyEl = Element::create("challengeReply");

        mExpectingChallengeProofInReply = IHasher::hashAsString("challenge:expecting:" + mSharedSecret + ":" + mHelloSalt + ":" + mChallengeSalt, IHasher::sha256());
        challengeReplyEl->adoptAsFirstChild(IHelper::createElementWithText("proof", mExpectingChallengeProofInReply));

        sendData(MessageType_ChallengeReply, challengeReplyEl);
        mHandshakeState = MessageType_ChallengeReply;
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleChallengeReply(const ElementPtr &rootEl)
      {
        if ((MessageType_Challenge != mHandshakeState) ||
            (!mChallengeSalt.hasData())) {
          ZS_LOG_WARNING(Detail, log("received challenge reply but not waiting for challenge reply"));
          disconnect();
          return;
        }
        
        String proof = IHelper::getElementText(rootEl->findFirstChildElement("proof"));
        if (proof != mExpectingChallengeProofInReply) {
          ZS_LOG_WARNING(Detail, log("received challenge reply but proof does not match") + ZS_PARAMIZE(proof) + ZS_PARAMIZE(mExpectingChallengeProofInReply));
          disconnect();
          return;
        }

        mHandshakeState = MessageType_ChallengeReply;
        sendWelcome();
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleWelcome(const ElementPtr &rootEl)
      {
        if (MessageType_ChallengeReply != mHandshakeState) {
          ZS_LOG_WARNING(Detail, log("received welcome but not waiting for welcome"));
          disconnect();
          return;
        }

        mHandshakeState = MessageType_Welcome;
        if (isConnectingMode()) {
          sendWelcome();
        }
        
        String valueStr = IHelper::getElementText(rootEl->findFirstChildElement("value32"));
        String valueBytesStr = IHelper::getElementText(rootEl->findFirstChildElement("value32Bytes"));
        
        try {
          uint32_t value32 = Numeric<uint32_t>(valueStr);
          
          auto valueBuffer = IHelper::convertFromHex(valueBytesStr);
          if ((!valueBuffer) ||
              (valueBuffer->SizeInBytes() < sizeof(value32))) {
            ZS_LOG_WARNING(Detail, log("value byes does not match size of value32"));
            disconnect();
            return;
          }
          
          if (0 != memcmp(valueBuffer->BytePtr(), &value32, sizeof(value32))) mFlipEndianInt = true;
        } catch (const Numeric<uint32_t>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, log("received welcome but missing value32"));
          disconnect();
          return;
        }

        String valueFloatStr = IHelper::getElementText(rootEl->findFirstChildElement("valueFloat"));
        String valueFloatBytesStr = IHelper::getElementText(rootEl->findFirstChildElement("valueFloatBytes"));

        try {
          float valueFloat = Numeric<float>(valueFloatStr);
          
          auto valueBuffer = IHelper::convertFromHex(valueFloatBytesStr);
          if ((!valueBuffer) ||
              (valueBuffer->SizeInBytes() < sizeof(valueFloat))) {
            ZS_LOG_WARNING(Detail, log("value byes does not match size of value float"));
            disconnect();
            return;
          }

          if (0 != memcmp(valueBuffer->BytePtr(), &valueFloat, sizeof(valueFloat))) mFlipEndianFloat = true;
        } catch (const Numeric<float>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, log("received welcome but missing value32"));
          disconnect();
          return;
        }

        IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
        IRemoteEventingAsyncDelegateProxy::create(mThisWeak.lock())->onRemoteEventingSubscribeLogger();
      }
      
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleNotify(const ElementPtr &rootEl)
      {
        String typeStr = IHelper::getElementText(rootEl->findFirstChildElement("type"));
        if (ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_GENERAL_INFO == typeStr) {
          handleNotifyGeneralInfo(rootEl);
          return;
        }
        if (ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_SUBSYSTEM == typeStr) {
          handleNotifyRemoteSubsystem(rootEl);
          return;
        }
        if (ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_PROVIDER == typeStr) {
          handleNotifyRemoteProvider(rootEl);
          return;
        }
        if (ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_PROVIDER_KEYWORD_LOGGING == typeStr) {
          handleNotifyRemoteProviderKeywordLogging(rootEl);
          return;
        }
        ZS_LOG_WARNING(Detail, log("remote notify is not understood (ignored)") + ZS_PARAMIZE(typeStr));
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleNotifyGeneralInfo(const ElementPtr &rootEl)
      {
        String subsystemStr = IHelper::getElementText(rootEl->findFirstChildElement("subsystem"));
        String droppedStr = IHelper::getElementText(rootEl->findFirstChildElement("dropped"));
        
        size_t totalDropped = 0;
        try {
          totalDropped = Numeric<decltype(totalDropped)>(droppedStr);
        } catch (const Numeric<decltype(totalDropped)>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, log("dropped value is not valid") + ZS_PARAMIZE(droppedStr));
          return;
        }

        if (mAnnouncedRemoteDropped != totalDropped) {
          mAnnouncedRemoteDropped = totalDropped;
          if (mDelegate) {
            try {
              mDelegate->onRemoteEventingRemoteDroppedEvents(mThisWeak.lock(), totalDropped);
            } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone (probably okay)"));
              mDelegate.reset();
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleNotifyRemoteSubsystem(const ElementPtr &rootEl)
      {
        String subsystemStr = IHelper::getElementText(rootEl->findFirstChildElement("name"));
        
        auto found = mRemoteSubsystems.find(subsystemStr);
        if (found != mRemoteSubsystems.end()) {
          ZS_LOG_WARNING(Debug, log("already notified about subsystem") + ZS_PARAMIZE(subsystemStr));
          return;
        }
        
        auto info = make_shared<SubsystemInfo>();
        info->mName = subsystemStr;
        mRemoteSubsystems[subsystemStr] = info;
        
        if (mDelegate) {
          try {
            mDelegate->onRemoteEventingRemoteSubsystem(mThisWeak.lock(), info->mName);
          } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate gone (probably okay)"));
            mDelegate.reset();
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleNotifyRemoteProvider(const ElementPtr &rootEl)
      {
        String remoteHandleStr = IHelper::getElementText(rootEl->findLastChildElement("handle"));
        String goneStr = IHelper::getElementText(rootEl->findLastChildElement("gone"));
        
        ProviderHandle remoteHandle = 0;
        try {
          remoteHandle = Numeric<ProviderHandle>(remoteHandleStr);
        } catch (const Numeric<ProviderHandle>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, log("remote handle is not valid") + ZS_PARAMIZE(remoteHandleStr));
          return;
        }
        
        if (goneStr.hasData()) {
          try {
            bool gone = Numeric<bool>(goneStr);
            if (gone) {
              auto found = mRemoteRegisteredProvidersByRemoteHandle.find(remoteHandle);
              if (found == mRemoteRegisteredProvidersByRemoteHandle.end()) {
                ZS_LOG_WARNING(Trace, log("notified remote provider is gone but provider was never announced"));
                return;
              }
              
              auto provider = (*found).second;

              {
                auto foundUUDI = mRemoteRegisteredProvidersByUUID.find(provider->mProviderID);
                if (foundUUDI != mRemoteRegisteredProvidersByUUID.end()) {
                  mRemoteRegisteredProvidersByUUID.erase(foundUUDI);
                } else {
                  ZS_LOG_WARNING(Trace, log("notified remote provider is gone but provider UUID was not found"));
                }
              }

              Log::unregisterEventingWriter(provider->mHandle);
              
              if (mDelegate) {
                try {
                  mDelegate->onRemoteEventingRemoteProviderGone(provider->mProviderName);
                } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
                  ZS_LOG_WARNING(Debug, log("delegate gone (probably okay)"));
                  mDelegate.reset();
                }
              }
              return;
            }
            
            // provider is not gone...
          } catch (const Numeric<bool>::ValueOutOfRange &) {
            ZS_LOG_WARNING(Detail, log("gone text was not understood") + ZS_PARAMIZE(goneStr));
            return;
          }
        }
        
        String providerIDStr = IHelper::getElementText(rootEl->findLastChildElement("id"));
        String providerNameStr = IHelper::getElementText(rootEl->findLastChildElement("name"));
        String providerHashStr = IHelper::getElementText(rootEl->findLastChildElement("hash"));
        
        ProviderInfo *provider {};
        
        try {
          UUID providerID = Numeric<UUID>(providerIDStr);
          
          auto found = mRemoteRegisteredProvidersByUUID.find(providerID);
          if (found != mRemoteRegisteredProvidersByUUID.end()) {
            ZS_LOG_WARNING(Debug, log("remote provider announced but alrady know about provider ID") + ZS_PARAMIZE(providerIDStr));
            return;
          }

          provider = new ProviderInfo;
          mCleanUpProviderInfos.insert(provider);

          provider->mRelatedToRemoteEventingObjectID = mID;
          provider->mProviderID = providerID;
          provider->mRemoteHandle = remoteHandle;
          provider->mProviderName = providerNameStr;
          provider->mProviderHash = providerHashStr;
          provider->mSelfRegistered = true;
        } catch (const Numeric<UUID>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Debug, log("remote provider announced by provider ID is not recognized") + ZS_PARAMIZE(providerIDStr));
          return;
        }

        mRemoteRegisteredProvidersByUUID[provider->mProviderID] = provider;
        mRemoteRegisteredProvidersByRemoteHandle[provider->mRemoteHandle] = provider;

        provider->mHandle = Log::registerEventingWriter(provider->mProviderID, provider->mProviderName, provider->mProviderHash);

        EventingAtomDataArray atomArray {};
        if (!Log::getEventingWriterInfo(provider->mHandle, provider->mProviderID, provider->mProviderName, provider->mProviderHash, &atomArray)) {
          ZS_LOG_WARNING(Detail, log("registered eventing writer but no information can be found") + ZS_PARAM("provider name", provider->mProviderName));
          return;
        }

        ProviderInfo *existingProvider = reinterpret_cast<ProviderInfo *>(atomArray[mEventingAtomIndex]);
        if (existingProvider) {
          if (mID != existingProvider->mRelatedToRemoteEventingObjectID) {
            ZS_LOG_WARNING(Detail, log("existing provider belongs to unrelated remote eventing") + ZS_PARAM("id", existingProvider->mRelatedToRemoteEventingObjectID));
            return;
          }
          existingProvider->mRemoteHandle = remoteHandle;
        } else {
          atomArray[mEventingAtomIndex] = reinterpret_cast<Log::EventingAtomData>(provider);
        }
        
        if (mDelegate) {
          try {
            mDelegate->onRemoteEventingRemoteProvider(provider->mProviderID, provider->mProviderName, provider->mProviderHash);
          } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Debug, log("delegate gone (probably okay)"));
            mDelegate.reset();
          }
        }

        for (auto iter_doNotUse = mRequestRemoteProviderKeywordLevel.begin(); iter_doNotUse != mRequestRemoteProviderKeywordLevel.end(); )
        {
          auto current = iter_doNotUse;
          ++iter_doNotUse;
          
          auto checkProviderName = (*current).first;
          auto bitmask = (*current).second;

          if (checkProviderName == providerNameStr) {
            requestSetRemoteEventProviderLogging(checkProviderName, bitmask);
            mRequestRemoteProviderKeywordLevel.erase(current);
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleNotifyRemoteProviderKeywordLogging(const ElementPtr &rootEl)
      {
        String remoteHandleStr = IHelper::getElementText(rootEl->findLastChildElement("handle"));
        String bitmaskStr = IHelper::getElementText(rootEl->findLastChildElement("bitmask"));
        
        ProviderHandle remoteHandle = 0;
        try {
          remoteHandle = Numeric<ProviderHandle>(remoteHandleStr);
        } catch (const Numeric<ProviderHandle>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Detail, log("remote handle is not valid") + ZS_PARAMIZE(remoteHandleStr));
          return;
        }
        
        auto found = mRemoteRegisteredProvidersByRemoteHandle.find(remoteHandle);
        if (found == mRemoteRegisteredProvidersByRemoteHandle.end()) {
          ZS_LOG_WARNING(Debug, log("told keyword logging information about unknown provider") + ZS_PARAMIZE(remoteHandle));
          return;
        }
        
        auto provider = (*found).second;

        try {
          KeywordBitmaskType bitmask = Numeric<KeywordBitmaskType>(bitmaskStr);
          if (mDelegate) {
            mDelegate->onRemoteEventingRemoteProviderStateChange(provider->mProviderName, bitmask);
          }
        } catch (const Numeric<KeywordBitmaskType>::ValueOutOfRange &) {
          ZS_LOG_WARNING(Debug, log("remote bitmask is not valid"));
        } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Debug, log("delegate gone (probably okay)"));
          mDelegate.reset();
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleRequest(const ElementPtr &rootEl)
      {
        int error = 0;
        String reason;
        String requestID = IHelper::getElementText(rootEl->findFirstChildElement("id"));
        String typeStr = IHelper::getElementText(rootEl->findFirstChildElement("type"));
        if (ZSLIB_EVENTING_REMOTE_EVENTING_REQUEST_SET_SUBSYSTEM_LEVEL == typeStr) {
          String subsystemStr = IHelper::getElementText(rootEl->findFirstChildElement("subsystem"));
          String levelStr = IHelper::getElementText(rootEl->findFirstChildElement("level"));

          try {
            auto level = Log::toLevel(levelStr);
            Log::setEventingLevelByName(subsystemStr, level);
          } catch (const InvalidArgument &) {
            ZS_LOG_WARNING(Detail, log("remote set subsystem request is not understood (ignored)") + ZS_PARAMIZE(subsystemStr) + ZS_PARAMIZE(subsystemStr));
            error = -1;
            reason = "Level was not understood: " + levelStr;
          }
          sendAck(requestID, error, reason);
          return;
        }
        if (ZSLIB_EVENTING_REMOTE_EVENTING_REQUEST_SET_EVENT_PROVIDER_LOGGING == typeStr) {
          String providerStr = IHelper::getElementText(rootEl->findFirstChildElement("provider"));
          String keywordStr = IHelper::getElementText(rootEl->findFirstChildElement("keywords"));
          KeywordBitmaskType bitmask = 0;
          try {
            bitmask = Numeric<KeywordBitmaskType>(keywordStr);
            for (auto iter = mLocalAnnouncedProviders.begin(); iter != mLocalAnnouncedProviders.end(); ++iter) {
              auto providerInfo = (*iter).second;
              if (providerInfo->mProviderName == providerStr) {
                if (0 == bitmask) {
                  auto found = mRequestedRemoteProviderKeywordLevel.find(providerInfo->mHandle);
                  if (found != mRequestedRemoteProviderKeywordLevel.end()) mRequestedRemoteProviderKeywordLevel.erase(found);
                } else {
                  mRequestedRemoteProviderKeywordLevel[providerInfo->mHandle] = providerInfo;
                }
                Log::setEventingLogging(providerInfo->mHandle, mID, 0 != bitmask, bitmask);
              }
            }
          } catch (const Numeric<KeywordBitmaskType>::ValueOutOfRange &) {
            ZS_LOG_WARNING(Detail, log("remote set event provider logging request is not understood (ignored)") + ZS_PARAMIZE(providerStr) + ZS_PARAMIZE(keywordStr));
            error = -1;
            reason = "Keywords value was not understood: " + keywordStr;
          }
          sendAck(requestID, error, reason);
          return;
        }
        
        ZS_LOG_WARNING(Detail, log("remote request is not understood (ignored)") + ZS_PARAMIZE(typeStr));
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::handleRequestAck(const ElementPtr &rootEl)
      {
        // ignored for now...
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::handleEvent(SecureByteBlock &buffer)
      {
        size_t expectingBasicSize = (sizeof(CryptoPP::word16)*5) +
                                    (sizeof(uint8_t)*4) +
                                    (sizeof(uint64_t)*2);
        
        if (buffer.SizeInBytes() < expectingBasicSize) {
          ZS_LOG_WARNING(Debug, log("event message did not contain enough header data") + ZS_PARAMIZE(expectingBasicSize) + ZS_PARAM("actual size", buffer.SizeInBytes()));
          return;
        }
        
        const BYTE *pos = buffer.BytePtr();
        
        uint64_t remoteHandle = IHelper::getBE64(pos);
        pos += sizeof(remoteHandle);
        
        auto found = mRemoteRegisteredProvidersByRemoteHandle.find(remoteHandle);
        if (found == mRemoteRegisteredProvidersByRemoteHandle.end()) {
          ZS_LOG_WARNING(Trace, log("event about provider that was never announced") + ZS_PARAMIZE(remoteHandle));
          return;
        }
        
        auto provider = (*found).second;
        if (!provider->mSelfRegistered) {
          ZS_LOG_ERROR(Debug, log("event about provider that was not registered from remote party") + ZS_PARAMIZE(remoteHandle));
          return;
        }
        
        Log::Severity severity = static_cast<Log::Severity>(IHelper::getBE16(pos));
        pos += sizeof(uint16_t);
        Log::Level level = static_cast<Log::Level>(IHelper::getBE16(pos));
        pos += sizeof(uint16_t);
        
        if ((severity < Log::Severity_First) ||
            (severity > Log::Severity_Last)) {
          ZS_LOG_WARNING(Debug, log("illegal severity") + ZS_PARAMIZE(severity));
          return;
        }
        if ((level < Log::Level_First) ||
            (level > Log::Level_Last)) {
          ZS_LOG_WARNING(Debug, log("illegal level") + ZS_PARAMIZE(level));
          return;
        }
        
        USE_EVENT_DESCRIPTOR descriptor {};
        descriptor.Id = IHelper::getBE16(pos);
        pos += sizeof(uint16_t);
        descriptor.Version = *pos;
        pos += sizeof(uint8_t);
        descriptor.Channel = *pos;
        pos += sizeof(uint8_t);
        descriptor.Level = *pos;
        pos += sizeof(uint8_t);
        descriptor.Opcode = *pos;
        pos += sizeof(uint8_t);
        descriptor.Task = IHelper::getBE16(pos);
        pos += sizeof(uint16_t);
        descriptor.Keyword = IHelper::getBE64(pos);
        pos += sizeof(uint64_t);

        size_t descriptorCount = IHelper::getBE16(pos);
        pos += sizeof(uint16_t);
        
        if (descriptorCount > ZSLIB_EVENTING_REMOTE_EVENTING_MAX_DATA_DESCRIPTORS) {
          ZS_LOG_WARNING(Debug, log("remote event contains too many data descriptors") + ZS_PARAMIZE(descriptorCount));
          return;
        }

        USE_EVENT_DATA_DESCRIPTOR dataDescriptors[ZSLIB_EVENTING_REMOTE_EVENTING_MAX_DATA_DESCRIPTORS];
        USE_EVENT_PARAMETER_DESCRIPTOR paramDescriptors[ZSLIB_EVENTING_REMOTE_EVENTING_MAX_DATA_DESCRIPTORS];

        size_t remaining = buffer.SizeInBytes() - expectingBasicSize;
        
        size_t expecting = (sizeof(uint16_t)*descriptorCount);
        if (remaining < expecting) {
          ZS_LOG_WARNING(Debug, log("event message did not contain enough data") + ZS_PARAMIZE(expecting) + ZS_PARAMIZE(remaining) + ZS_PARAM("actual size", buffer.SizeInBytes()));
          return;
        }

        for (size_t index = 0; index < descriptorCount; ++index) {
          paramDescriptors[index].Type = static_cast<EventParameterTypes>(IHelper::getBE16(pos));
          pos += sizeof(uint16_t);
          remaining -= sizeof(uint16_t);
        }

        for (size_t index = 0; index < descriptorCount; ++index) {
          
          {
            expecting = sizeof(uint32_t);
            if (remaining < expecting) goto not_enough_data;

            uint32_t dataTypeSize = IHelper::getBE32(pos);
            pos += sizeof(dataTypeSize);
            remaining -= sizeof(dataTypeSize);
            
            bool endianFlip {false};
            if (0 != (dataTypeSize & (1 << 31))) {
              endianFlip = true;
              dataTypeSize = dataTypeSize & (0x7FFFFFFF);
            }
            
            expecting = dataTypeSize;
            if (remaining < expecting) goto not_enough_data;
            
            dataDescriptors[index].Size = dataTypeSize;
            if (0 != dataDescriptors[index].Size) {
              dataDescriptors[index].Ptr = reinterpret_cast<uintptr_t>(pos);
            }

            if (endianFlip) {
              switch (dataTypeSize) {
                case 2: {
                  uint16_t value = IHelper::getBE16(pos);
                  memcpy(const_cast<BYTE *>(pos), &value, sizeof(value));
                  break;
                }
                case 4: {
                  uint32_t value = IHelper::getBE32(pos);
                  memcpy(const_cast<BYTE *>(pos), &value, sizeof(value));
                  break;
                }
                case 8: {
                  uint64_t value = IHelper::getBE64(pos);
                  memcpy(const_cast<BYTE *>(pos), &value, sizeof(value));
                  break;
                }
                default:  {
                  // just leave in original format
                  break;
                }
              }
            }

            pos += dataTypeSize;
            remaining -= dataTypeSize;
            
            continue;
          }
          
        not_enough_data:
          {
            ZS_LOG_WARNING(Debug, log("event message did not contain enough data") + ZS_PARAMIZE(index) + ZS_PARAMIZE(expecting) + ZS_PARAMIZE(remaining) + ZS_PARAM("actual size", buffer.SizeInBytes()));
            return;
          }
        }

        // write the remote event as if it was generated locally
        Log::writeEvent(
                        provider->mHandle,
                        severity,
                        level,
                        (&descriptor),
                        (&(paramDescriptors[0])),
                        (&(dataDescriptors[0])),
                        descriptorCount
                        );
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::sendWelcome()
      {
        ElementPtr welcomeEl = Element::create("welcome");
        
        uint32_t endian32 = 1;
        BYTE endian32Bytes[sizeof(endian32)] {};
        memcpy(&(endian32Bytes[0]), &endian32, sizeof(endian32));
        
        float endianFloat = 3.14f;
        BYTE endianFloatBytes[sizeof(endianFloat)] {};
        memcpy(&(endianFloatBytes[0]), &endianFloat, sizeof(endianFloat));
        
        welcomeEl->adoptAsFirstChild(IHelper::createElementWithNumber("value32", string(endian32)));
        welcomeEl->adoptAsFirstChild(IHelper::createElementWithText("value32Bytes", IHelper::convertToHex(&(endian32Bytes[0]), sizeof(endian32Bytes))));
        welcomeEl->adoptAsFirstChild(IHelper::createElementWithNumber("valueFloat", string(endianFloat)));
        welcomeEl->adoptAsFirstChild(IHelper::createElementWithText("valueFloatBytes", IHelper::convertToHex(&(endianFloatBytes[0]), sizeof(endianFloatBytes))));
        
        sendData(MessageType_Welcome, welcomeEl);
        
        for (auto iter = mLocalSubsystems.begin(); iter != mLocalSubsystems.end(); ++iter) {
          auto &info = (*iter).second;
          announceSubsystemToRemote(info);
        }
        
        for (auto iter = mLocalAnnouncedProviders.begin(); iter != mLocalAnnouncedProviders.end(); ++iter) {
          auto &info = (*iter).second;
          announceProviderToRemote(info);
        }
        
        for (auto iter = mSetRemoteSubsystemsLevels.begin(); iter != mSetRemoteSubsystemsLevels.end(); ++iter) {
          auto &info = (*iter).second;
          requestSetRemoteSubsystemLevel(info);
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::sendNotify()
      {
        if (!isAuthorized()) {
          ZS_LOG_TRACE(log("skipping notify as not authorized yet"));
          return;
        }

        if (mAnnouncedLocalDropped != mTotalDroppedEvents) {
          mAnnouncedLocalDropped = mTotalDroppedEvents;
          ElementPtr rootEl = Element::create("notify");
          rootEl->adoptAsLastChild(IHelper::createElementWithText("type", ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_GENERAL_INFO));
          rootEl->adoptAsLastChild(IHelper::createElementWithNumber("dropped", string(mTotalDroppedEvents)));
          sendData(MessageType_Notify, rootEl);

          if (mDelegate) {
            try {
              mDelegate->onRemoteEventingLocalDroppedEvents(mThisWeak.lock(), mAnnouncedLocalDropped);
            } catch (const IRemoteEventingDelegateProxy::Exceptions::DelegateGone &) {
              ZS_LOG_WARNING(Detail, log("delegate gone (probably okay)"));
              mDelegate.reset();
            }
          }
        }
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::requestSetRemoteSubsystemLevel(SubsystemInfoPtr info)
      {
        ElementPtr rootEl = Element::create("request");
        
        rootEl->adoptAsLastChild(IHelper::createElementWithText("type", ZSLIB_EVENTING_REMOTE_EVENTING_REQUEST_SET_SUBSYSTEM_LEVEL));
        rootEl->adoptAsLastChild(IHelper::createElementWithText("subsystem", info->mName));
        rootEl->adoptAsLastChild(IHelper::createElementWithText("level", zsLib::Log::toString(info->mLevel)));

        sendData(MessageType_Request, rootEl);
      }

      //-----------------------------------------------------------------------
      void RemoteEventing::requestSetRemoteEventProviderLogging(
                                                                const String &providerName,
                                                                KeywordBitmaskType bitmask
                                                                )
      {
        ElementPtr rootEl = Element::create("request");
        
        rootEl->adoptAsLastChild(IHelper::createElementWithText("type", ZSLIB_EVENTING_REMOTE_EVENTING_REQUEST_SET_EVENT_PROVIDER_LOGGING));
        rootEl->adoptAsLastChild(IHelper::createElementWithText("provider", providerName));
        rootEl->adoptAsLastChild(IHelper::createElementWithNumber("keywords", string(bitmask)));

        sendData(MessageType_Request, rootEl);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::announceProviderToRemote(
                                                    ProviderInfo *provider,
                                                    bool announceNew
                                                    )
      {
        ElementPtr rootEl = Element::create("notify");
        
        rootEl->adoptAsLastChild(IHelper::createElementWithText("type", ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_PROVIDER));
        rootEl->adoptAsLastChild(IHelper::createElementWithNumber("handle", string(static_cast<uint64_t>(provider->mHandle))));
        if (announceNew) {
          rootEl->adoptAsLastChild(IHelper::createElementWithText("id", string(provider->mProviderID)));
          rootEl->adoptAsLastChild(IHelper::createElementWithText("name", provider->mProviderName));
          rootEl->adoptAsLastChild(IHelper::createElementWithText("hash", provider->mProviderHash));
        } else {
          rootEl->adoptAsLastChild(IHelper::createElementWithNumber("gone", (!announceNew) ? "true" : "false"));
        }

        sendData(MessageType_Notify, rootEl);
        
        if (0 != provider->mBitmask) {
          announceProviderLoggingStateChangedToRemote(provider, provider->mBitmask);
        }
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::announceProviderLoggingStateChangedToRemote(
                                                                       ProviderInfo *provider,
                                                                       KeywordBitmaskType bitmask
                                                                       )
      {
        ElementPtr rootEl = Element::create("notify");
        
        rootEl->adoptAsLastChild(IHelper::createElementWithText("type", ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_PROVIDER_KEYWORD_LOGGING));
        
        rootEl->adoptAsLastChild(IHelper::createElementWithNumber("handle", string(static_cast<uint64_t>(provider->mHandle))));
        rootEl->adoptAsLastChild(IHelper::createElementWithNumber("bitmask", string(bitmask)));
        
        sendData(MessageType_Notify, rootEl);
      }
      
      //-----------------------------------------------------------------------
      void RemoteEventing::announceSubsystemToRemote(SubsystemInfoPtr info)
      {
        ElementPtr rootEl = Element::create("notify");
        
        rootEl->adoptAsLastChild(IHelper::createElementWithText("type", ZSLIB_EVENTING_REMOTE_EVENTING_NOTIFY_SUBSYSTEM));
        rootEl->adoptAsLastChild(IHelper::createElementWithText("name", info->mName));
        
        sendData(MessageType_Notify, rootEl);
      }
      
    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRemoteEventingTypes
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IRemoteEventingTypes::toString(States state)
    {
      switch (state)
      {
        case State_Pending:               return "Pending";
        case State_Listening:             return "Listening";
        case State_Connecting:            return "Connecting";
        case State_Connected:             return "Connected";
        case State_ShuttingDown:          return "Shutting down";
        case State_Shutdown:              return "Shutdown";
      }

      return "unknown";
    }
    
    //-------------------------------------------------------------------------
    IRemoteEventingTypes::States IRemoteEventingTypes::toState(const char *state) throw (InvalidArgument)
    {
      String str(state);
      for (IRemoteEventingTypes::States index = IRemoteEventingTypes::State_First; index <= IRemoteEventingTypes::State_Last; index = static_cast<IRemoteEventingTypes::States>(static_cast<std::underlying_type<IRemoteEventingTypes::States>::type>(index) + 1)) {
        if (0 == str.compareNoCase(IRemoteEventingTypes::toString(index))) return index;
      }

      ZS_THROW_INVALID_ARGUMENT(String("Not a state: ") + str);
      return State_First;
    }
    
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IRemoteEventing
    #pragma mark

    //-------------------------------------------------------------------------
    IRemoteEventingPtr IRemoteEventing::connectToRemote(
                                                        IRemoteEventingDelegatePtr connectionDelegate,
                                                        const IPAddress &serverIP,
                                                        const char *connectionSharedSecret
                                                        )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(serverIP.isEmpty());
      return internal::RemoteEventing::connectToRemote(connectionDelegate, serverIP, connectionSharedSecret);
    }

    //-------------------------------------------------------------------------
    IRemoteEventingPtr IRemoteEventing::listenForRemote(
                                                        IRemoteEventingDelegatePtr connectionDelegate,
                                                        WORD localPort,
                                                        const char *connectionSharedSecret,
                                                        Seconds maxWaitToBindTimeInSeconds
                                                        )
    {
      return internal::RemoteEventing::listenForRemote(connectionDelegate, localPort, connectionSharedSecret, maxWaitToBindTimeInSeconds);
    }

  } // namespace eventing
} // namespace zsLib
