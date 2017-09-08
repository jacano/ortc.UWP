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

#include <zsLib/eventing/tool/internal/zsLib_eventing_tool_Monitor.h>

#include <zsLib/eventing/tool/OutputStream.h>
#include <zsLib/eventing/IHelper.h>
#include <zsLib/eventing/Log.h>

#include <zsLib/IMessageQueueManager.h>
#include <zsLib/Numeric.h>

namespace zsLib { namespace eventing { namespace tool { ZS_DECLARE_SUBSYSTEM(zsLib_eventing_tool) } } }

namespace zsLib
{
  namespace eventing
  {
    namespace tool
    {
      ZS_DECLARE_USING_PTR(zsLib::eventing, IHelper);
      
      typedef eventing::USE_EVENT_DESCRIPTOR USE_EVENT_DESCRIPTOR;
      typedef eventing::USE_EVENT_PARAMETER_DESCRIPTOR USE_EVENT_PARAMETER_DESCRIPTOR;
      typedef eventing::USE_EVENT_DATA_DESCRIPTOR USE_EVENT_DATA_DESCRIPTOR;
      
      namespace internal
      {
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark (helpers)
        #pragma mark
        
        //---------------------------------------------------------------------
        static bool hasSingleton(bool nowHaveSingle = false)
        {
          static bool hasSingleton {};
          if (nowHaveSingle) hasSingleton = true;
          return hasSingleton;
        }
        
        //---------------------------------------------------------------------
        static uint64_t getUnsignedValue(const USE_EVENT_DATA_DESCRIPTOR &data)
        {
          if (!data.Ptr) return 0;
          
          switch (data.Size) {
            case 1: {
              uint8_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(uint8_t));
              return value;
            }
            case 2: {
              uint16_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(uint16_t));
              return value;
            }
            case 4: {
              uint32_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(uint32_t));
              return value;
            }
            case 8: {
              uint64_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(uint64_t));
              return value;
            }
            default: {
              uint64_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(uint64_t) > data.Size ? data.Size : sizeof(uint64_t));
              return value;
            }
          }
          return 0;
        }
        
        //---------------------------------------------------------------------
        static int64_t getSignedValue(const USE_EVENT_DATA_DESCRIPTOR &data)
        {
          if (!data.Ptr) return 0;
          
          switch (data.Size) {
            case 1: {
              int8_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(int8_t));
              return value;
            }
            case 2: {
              int16_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(int16_t));
              return value;
            }
            case 4: {
              int32_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(int32_t));
              return value;
            }
            case 8: {
              int64_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(int64_t));
              return value;
            }
            default: {
              int64_t value = 0;
              memcpy(&value, (const void *)(data.Ptr), sizeof(int64_t) > data.Size ? data.Size : sizeof(int64_t));
              return value;
            }
          }
          return 0;
        }
        
        //---------------------------------------------------------------------
        static double getFloatValue(const USE_EVENT_DATA_DESCRIPTOR &data)
        {
          if (!data.Ptr) return 0.0f;
          
          if (sizeof(float) == data.Size) {
            float value {};
            memcpy(&value, (const void *)(data.Ptr), sizeof(value));
            return value;
          }
          if (sizeof(double) == data.Size) {
            double value ={};
            memcpy(&value, (const void *)(data.Ptr), sizeof(value));
            return value;
          }
          
          double value = 0;
          memcpy(&value, (const void *)(data.Ptr), sizeof(double) > data.Size ? data.Size : sizeof(double));
          return value;
        }
        
        //---------------------------------------------------------------------
        static String valueAsString(
                                    const USE_EVENT_PARAMETER_DESCRIPTOR &param,
                                    const USE_EVENT_DATA_DESCRIPTOR &data,
                                    bool &outIsNumber
                                    )
        {
          outIsNumber = true;
          
          switch (param.Type) {
            case EventParameterType_Boolean:          {
              outIsNumber = false;
              if (0 != getUnsignedValue(data)) return "true";
              return "false";
            }
            case EventParameterType_UnsignedInteger:  return string(getUnsignedValue(data));
            case EventParameterType_SignedInteger:    return string(getSignedValue(data));
            
            case EventParameterType_FloatingPoint:    return string(getFloatValue(data));
            case EventParameterType_Pointer:          return string(getUnsignedValue(data));
            case EventParameterType_AString:          {
              outIsNumber = false;
              if (!data.Ptr) return String();
              if (0 == data.Size) return String();
              auto temp = IHelper::convertToBuffer(reinterpret_cast<const BYTE *>(data.Ptr), data.Size);
              return String(reinterpret_cast<const char *>(temp->BytePtr()));
            }
            case EventParameterType_WString:          {
              outIsNumber = false;
              if (!data.Ptr) return String();
              if (0 == data.Size) return String();
              size_t total = data.Size / sizeof(wchar_t);
              
              if (0 == total) return String();
              
              wchar_t *temp = new wchar_t[total+1] {};
              memcpy(temp, (const void *)(data.Ptr), data.Size);
              
              String result(&(temp[0]));
              
              delete [] temp;
              temp = NULL;
              
              return result;
            }
            case EventParameterType_Binary:
            default:
            {
              break;
            }
          }

          outIsNumber = false;
          if (!data.Ptr) return String();
          if (0 == data.Size) return String();
          return IHelper::convertToHex(reinterpret_cast<const BYTE *>(data.Ptr), data.Size);
        }
        
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor
        #pragma mark
        
        //---------------------------------------------------------------------
        Monitor::Monitor(
                         const make_private &,
                         IMessageQueuePtr queue,
                         const ICommandLineTypes::MonitorInfo &monitorInfo
                         ) :
          MessageQueueAssociator(queue),
          mMonitorInfo(monitorInfo),
          mEventingAtom(zsLib::Log::registerEventingAtom("org.zsLib.eventing.tool.Monitor"))
        {
          if (mMonitorInfo.mOutputJSON) {
            tool::output() << "{ \"events\": { \"event\": [\n";
          }
        }
        
        //---------------------------------------------------------------------
        Monitor::~Monitor()
        {
          mThisWeak.reset();
          
          for (auto iter = mCleanProviderInfos.begin(); iter != mCleanProviderInfos.end(); ++iter) {
            auto info = (*iter);
            
            EventingAtomDataArray eventingArray = NULL;
            if (Log::getEventingWriterInfo(info->mHandle, info->mProviderID, info->mProviderName, info->mProviderUniqueHash, &eventingArray)) {
              eventingArray[mEventingAtom] = 0;
            }

            delete (*iter);
          }
          mCleanProviderInfos.clear();
        }
        
        //---------------------------------------------------------------------
        void Monitor::init()
        {
          IWakeDelegateProxy::create(mThisWeak.lock())->onWake();
        }

        //---------------------------------------------------------------------
        MonitorPtr Monitor::create(const ICommandLineTypes::MonitorInfo &monitorInfo)
        {
          auto queue = IMessageQueueManager::getMessageQueue("org.zsLib.eventing.tool.Monitor");
          auto pThis(make_shared<Monitor>(make_private{}, queue, monitorInfo));
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        MonitorPtr Monitor::singleton(const ICommandLineTypes::MonitorInfo *monitorInfo)
        {
          AutoRecursiveLock lock(*IHelper::getGlobalLock());
          hasSingleton(true);
          static SingletonLazySharedPtr<Monitor> pThis(Monitor::create(*monitorInfo));
          static SingletonManager::Register reg("org.zsLib.eventing.tool.Monitor", pThis.singleton());
          return pThis.singleton();
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor => (friends)
        #pragma mark
        
        //---------------------------------------------------------------------
        void Monitor::monitor(const ICommandLineTypes::MonitorInfo &monitorInfo)
        {
          auto pThis = Monitor::singleton(&monitorInfo);
          if (!pThis) return;
          
          while (!pThis->shouldQuit())
          {
            std::this_thread::sleep_for(Seconds(1));
          }
        }

        //---------------------------------------------------------------------
        void Monitor::interrupt()
        {
          {
            AutoRecursiveLock lock(*IHelper::getGlobalLock());
            if (!hasSingleton()) return;
          }

          auto pThis = Monitor::singleton();
          if (!pThis) return;

          pThis->internalInterrupt();

          while (!pThis->shouldQuit())
          {
            std::this_thread::sleep_for(Seconds(1));
          }
        }
 
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor::ISingletonManagerDelegate
        #pragma mark
        
        //---------------------------------------------------------------------
        void Monitor::notifySingletonCleanup()
        {
          internalInterrupt();
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor::ITimerDelegate
        #pragma mark

        //---------------------------------------------------------------------
        void Monitor::onTimer(ITimerPtr timer)
        {
          AutoRecursiveLock lock(mLock);
          if (timer == mAutoQuitTimer) {
            if (!mMonitorInfo.mQuietMode) {
              tool::output() << "[Info] Auto-quit timer fired.\n";
            }
            cancel();
            return;
          }
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor::ITimerDelegate
        #pragma mark

        //---------------------------------------------------------------------
        void Monitor::onWake()
        {
          {
            AutoRecursiveLock lock(mLock);
            step();
          }

          if (mMonitorInfo.mOutputJSON) {
            Log::addEventingProviderListener(mThisWeak.lock());
            Log::addEventingListener(mThisWeak.lock());
          }
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor::IRemoteEventingDelegate
        #pragma mark
        
        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingStateChanged(
                                                   IRemoteEventingPtr connection,
                                                   States state
                                                   )
        {
          if (!mMonitorInfo.mQuietMode) {
            tool::output() << "[Info] Remoting eventing state: " << IRemoteEventing::toString(state) << "\n";
          }

          switch (state) {
            case IRemoteEventingTypes::State_ShuttingDown:
            case IRemoteEventingTypes::State_Shutdown:
            {
              AutoRecursiveLock lock(mLock);
              cancel();
              break;
            }
            default: {
              break;
            }
          }
        }
        
        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingRemoteSubsystem(
                                                      IRemoteEventingPtr connection,
                                                      const char *subsystemName
                                                      )
        {
          if (!mMonitorInfo.mQuietMode) {
            tool::output() << "[Info] Remoting eventing subsystem: " << String(subsystemName) << "\n";
          }
        }
        
        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingRemoteProvider(
                                                     UUID providerID,
                                                     const char *providerName,
                                                     const char *providerUniqueHash
                                                     )
        {
          if (!mMonitorInfo.mQuietMode) {
            tool::output() << "[Info] Remote provider \"" << String(providerName) << "\", \"" << string(providerID) << ", \"" << providerUniqueHash << "\" found.\n";
          }
        }
        
        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingRemoteProviderGone(const char *providerName)
        {
          if (!mMonitorInfo.mQuietMode) {
            tool::output() << "[Info] Remote provider \"" << String(providerName) << " gone.\n";
          }
        }
        
        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingRemoteProviderStateChange(
                                                                const char *providerName,
                                                                KeywordBitmaskType keywords
                                                                )
        {
          if (!mMonitorInfo.mQuietMode) {
            tool::output() << "[Info] Remote provider \"" << String(providerName) << " logging state change with bitmask " << IHelper::convertToHex((BYTE *)(&keywords), sizeof(keywords)) << "\n";
          }
        }
        
        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingLocalDroppedEvents(
                                                         IRemoteEventingPtr connection,
                                                         size_t totalDropped
                                                         )
        {
          // ignored
        }

        //---------------------------------------------------------------------
        void Monitor::onRemoteEventingRemoteDroppedEvents(
                                                          IRemoteEventingPtr connection,
                                                          size_t totalDropped
                                                          )
        {
          mTotalEventsDropped = totalDropped;
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor::ILogEventingDelegate
        #pragma mark

        //---------------------------------------------------------------------
        void Monitor::notifyEventingProviderRegistered(
                                                       ProviderHandle handle,
                                                       EventingAtomDataArray eventingAtomDataArray
                                                       )
        {
          ProviderInfo *provider = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtom]);

          bool subscribeLogging = false;

          // process event
          {
            AutoRecursiveLock lock(mLock);
            if (!mRemote) return;

            if (NULL == provider) {
              provider = new ProviderInfo;
              mCleanProviderInfos.insert(provider);

              provider->mHandle = handle;

              eventingAtomDataArray[mEventingAtom] = reinterpret_cast<EventingAtomData>(provider);

              if (Log::getEventingWriterInfo(handle, provider->mProviderID, provider->mProviderName, provider->mProviderUniqueHash)) {
                auto found = mProviders.find(provider->mProviderID);
                if (found != mProviders.end()) {

                  auto existingProvider = (*found).second;

                  if (existingProvider->mUniqueHash == provider->mProviderUniqueHash) {
                    provider->mExistingProvider = (*found).second;

                    // process all events into a quick lookup map
                    for (auto iter = provider->mExistingProvider->mEvents.begin(); iter != provider->mExistingProvider->mEvents.end(); ++iter)
                    {
                      auto event = (*iter).second;
                      provider->mEvents[event->mValue] = event;
                    }
                  } else {
                    if (!mMonitorInfo.mQuietMode) {
                      tool::output() << "[Warning] Provider \"" << provider->mProviderName << "\" hashes do not match: X=" << existingProvider->mUniqueHash << " Y=" << provider->mProviderUniqueHash << "\n";
                    }
                  }
                }
              }
            }

            for (auto iter = mMonitorInfo.mSubscribeProviders.begin(); iter != mMonitorInfo.mSubscribeProviders.end(); ++iter)
            {
              auto &name = (*iter);
              if (0 == name.compareNoCase(provider->mProviderName)) {
                if (!mMonitorInfo.mQuietMode) {
                  tool::output() << "[Info] Provider \"" << provider->mProviderName << "\" is subscribed.\n";
                }
                subscribeLogging = true;
              }
            }
          }

          if (subscribeLogging) {
            Log::setEventingLogging(handle, mID, true);
          }
        }

        //---------------------------------------------------------------------
        void Monitor::notifyEventingProviderUnregistered(
                                                         ProviderHandle handle,
                                                         EventingAtomDataArray eventingAtomDataArray
                                                         )
        {
          ProviderInfo *provider = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtom]);
          if (!provider) return;
          Log::setEventingLogging(handle, mID, false);
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor::ILogEventingDelegate
        #pragma mark
        
        //---------------------------------------------------------------------
        void Monitor::notifyWriteEvent(
                                      ProviderHandle handle,
                                      EventingAtomDataArray eventingAtomDataArray,
                                      Severity severity,
                                      Level level,
                                      EVENT_DESCRIPTOR_HANDLE descriptor,
                                      EVENT_PARAMETER_DESCRIPTOR_HANDLE paramDescriptor,
                                      EVENT_DATA_DESCRIPTOR_HANDLE dataDescriptor,
                                      size_t dataDescriptorCount
                                      )
        {
          static const size_t skipStartLength = strlen("{\"event\":");
          static const size_t skipEndLength = strlen("}");
          
          ProviderInfo *provider = reinterpret_cast<ProviderInfo *>(eventingAtomDataArray[mEventingAtom]);
          if (!provider) return;

          ++mTotalEvents;

          String output;

          if (provider->mEvents.size() > 0) {
            auto found = provider->mEvents.find(descriptor->Id);
            if (found != provider->mEvents.end()) {
              auto event = (*found).second;

              ElementPtr rootEl = Element::create("event");
              rootEl->adoptAsLastChild(IHelper::createElementWithText("severity", Log::toString(severity)));
              rootEl->adoptAsLastChild(IHelper::createElementWithText("level", Log::toString(level)));
              rootEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode("name", event->mName));
              if (event->mChannel) {
                rootEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode("channel", event->mChannel->mID));
              }
              if (event->mTask) {
                rootEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode("task", event->mTask->mName));
              }
              if (event->mOpCode) {
                rootEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode("opCode", event->mOpCode->mName));
              }
              
              ElementPtr valuesEl = Element::create("values");
              rootEl->adoptAsLastChild(valuesEl);

              size_t totalDataParams = ZS_EVENTING_TOTAL_BUILT_IN_EVENT_DATA;

              if (event->mDataTemplate) {
                totalDataParams += event->mDataTemplate->mDataTypes.size();
              }

              if (totalDataParams != dataDescriptorCount) {
                // reduce total params by number of buffer sizes not included
                for (auto iter = event->mDataTemplate->mDataTypes.begin(); iter != event->mDataTemplate->mDataTypes.end(); ++iter) {
                  auto &dataType = (*iter);
                  if (dataType->mType == IEventingTypes::PredefinedTypedef_size) {
                    --totalDataParams;
                  }
                }
                if (totalDataParams != dataDescriptorCount) {
                  if (!mMonitorInfo.mQuietMode) {
                    tool::output() << "[Warning] Event \"" << event->mName << "\" parameter count does not match: X=" << string(totalDataParams) << " Y=" << string(dataDescriptorCount) << "\n";
                    return;
                  }
                }
              }

              for (size_t index = 0; index < ZS_EVENTING_TOTAL_BUILT_IN_EVENT_DATA; ++index)
              {
                bool isNumber = false;
                String valueName("unknown");
                String value = valueAsString(paramDescriptor[index], dataDescriptor[index], isNumber);
                switch (index) {
                  case 0: valueName = "_subsystemName"; break;
                  case 1: valueName = "_function"; break;
                  case 2: valueName = "_line"; break;
                }
                
                if (isNumber) {
                  valuesEl->adoptAsLastChild(IHelper::createElementWithNumber(valueName, value));
                } else {
                  valuesEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode(valueName, value));
                }
              }

              if (event->mDataTemplate) {
                auto iterDataType = event->mDataTemplate->mDataTypes.begin();
                for (int index = ZS_EVENTING_TOTAL_BUILT_IN_EVENT_DATA; static_cast<size_t>(index) < dataDescriptorCount; ++iterDataType)
                {
                  {
                    if (iterDataType == event->mDataTemplate->mDataTypes.end()) break;
                    
                    auto &dataType = (*iterDataType);

                    bool isNumber = false;
                    String valueName = dataType->mValueName;

                    int offset = 0;
                    if (dataType->mType == IEventingTypes::PredefinedTypedef_binary) {
                      if (static_cast<size_t>(index + 1) >= dataDescriptorCount) {
                        if (!mMonitorInfo.mQuietMode) {
                          tool::output() << "[Warning] Event \"" << event->mName << "\" parameter count buffer missing space for buffer: COUNT=" << string(dataDescriptorCount) << " INDEX=" << string(index) << "\n";
                        }
                        return;
                      }

                      // the actual type and value of the buffer is stored just after its size in the array
                      offset = 1;
                    } else if (dataType->mType == IEventingTypes::PredefinedTypedef_size) {
                      if (index < 1) {
                        if (!mMonitorInfo.mQuietMode) {
                          tool::output() << "[Warning] Event \"" << event->mName << "\" parameter count buffer missing space for buffer size: COUNT=" << string(dataDescriptorCount) << " INDEX=" << string(index) << "\n";
                        }
                        return;
                      }
                      offset = -1;
                    }

                    String value = valueAsString(paramDescriptor[index + offset], dataDescriptor[index + offset], isNumber);

                    if (isNumber) {
                      valuesEl->adoptAsLastChild(IHelper::createElementWithNumber(valueName, value));
                    } else {
                      valuesEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode(valueName, value));
                    }
                    goto next_index;
                  }
                  
                next_index:
                  {
                    ++index;
                  }
                }
              }

              output = IHelper::toString(rootEl);
            }
          }
          
          if (!output.hasData()) {
            ElementPtr rootEl = Element::create("event");
            rootEl->adoptAsLastChild(IHelper::createElementWithText("severity", Log::toString(severity)));
            rootEl->adoptAsLastChild(IHelper::createElementWithText("level", Log::toString(level)));
            rootEl->adoptAsLastChild(IHelper::createElementWithNumber("name", string(descriptor->Id)));
            rootEl->adoptAsLastChild(IHelper::createElementWithNumber("channel", string(descriptor->Channel)));
            rootEl->adoptAsLastChild(IHelper::createElementWithNumber("task", string(descriptor->Task)));
            rootEl->adoptAsLastChild(IHelper::createElementWithNumber("opCode", string(descriptor->Opcode)));

            ElementPtr valuesEl = Element::create("values");
            rootEl->adoptAsLastChild(valuesEl);

            for (size_t index = 0; index < ZS_EVENTING_TOTAL_BUILT_IN_EVENT_DATA; ++index)
            {
              bool isNumber = false;
              String valueName("unknown");
              String value = valueAsString(paramDescriptor[index], dataDescriptor[index], isNumber);
              switch (index) {
                case 0: valueName = "_subsystemName"; break;
                case 1: valueName = "_function"; break;
                case 2: valueName = "_line"; break;
              }
              
              if (isNumber) {
                valuesEl->adoptAsLastChild(IHelper::createElementWithNumber(valueName, value));
              } else {
                valuesEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode(valueName, value));
              }
            }
            
            for (size_t index = ZS_EVENTING_TOTAL_BUILT_IN_EVENT_DATA; index < dataDescriptorCount; ++index)
            {
              bool isNumber = false;
              String valueName = string(index-ZS_EVENTING_TOTAL_BUILT_IN_EVENT_DATA);
              String value = valueAsString(paramDescriptor[index], dataDescriptor[index], isNumber);
              
              if (isNumber) {
                valuesEl->adoptAsLastChild(IHelper::createElementWithNumber(valueName, value));
              } else {
                valuesEl->adoptAsLastChild(IHelper::createElementWithTextAndJSONEncode(valueName, value));
              }
            }

            output = IHelper::toString(rootEl);
          }

          if (output.hasData()) {
            AutoRecursiveLock lock(mLock);
            if (!mFirstOutputEvent) {
              tool::output() << ",";
            } else {
              mFirstOutputEvent = false;
            }
            tool::output().write(output.c_str() + skipStartLength, output.length() - skipStartLength - skipEndLength);
            tool::output() << "\n";
          }
        }

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Monitor => (internal)
        #pragma mark

        //---------------------------------------------------------------------
        void Monitor::internalInterrupt()
        {
          AutoRecursiveLock lock(mLock);
          cancel();
        }

        //---------------------------------------------------------------------
        void Monitor::cancel()
        {
          if (mShouldQuit) return;
          
          auto pThis = mThisWeak.lock();
          mGracefulShutdownReference = pThis;

          if (mRemote) {
            mRemote->shutdown();
          }

          if (mAutoQuitTimer) {
            mAutoQuitTimer->cancel();
            mAutoQuitTimer.reset();
          }

          if (mGracefulShutdownReference) {
            if (mRemote) {
              auto state = mRemote->getState();
              if (IRemoteEventingTypes::State_Shutdown != state) return;
            }
          }
          
          if (mMonitorInfo.mOutputJSON) {
            Log::removeEventingListener(pThis);
            Log::removeEventingProviderListener(pThis);
          }

          mRemote.reset();

          if (mMonitorInfo.mOutputJSON) {
            tool::output() << "\n] } }\n";
          }
          if (!mMonitorInfo.mQuietMode) {
            tool::output() << "\n";
            tool::output() << "[Info] Total events dropped: " << string(mTotalEventsDropped) << "\n";
            tool::output() << "[Info] Total events received: " << string(mTotalEvents) << "\n";
          }
          mShouldQuit = true;

          mGracefulShutdownReference.reset();
        }

        //---------------------------------------------------------------------
        void Monitor::step()
        {
          for (auto iter = mMonitorInfo.mJMANFiles.begin(); iter != mMonitorInfo.mJMANFiles.end(); ++iter) {
            auto fileName = (*iter);
            
            ProviderPtr provider;
            SecureByteBlockPtr jmanRaw;
            
            try {
              jmanRaw = IHelper::loadFile(fileName);
            } catch (const StdError &e) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load jman file: ") + fileName + ", error=" + string(e.result()) + ", reason=" + e.message());
            }
            if (!jmanRaw) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, String("Failed to load jman file: ") + fileName);
            }
            
            auto rootEl = IHelper::read(jmanRaw);
            
            try {
              provider = Provider::create(rootEl);
            } catch (const InvalidContent &e) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Failed to parse jman file: " + e.message());
            }
            if (!provider) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_FILE_FAILED_TO_LOAD, "Failed to parse jman file: " + fileName);
            }
            
            auto found = mProviders.find(provider->mID);
            if (found != mProviders.end()) {
              ZS_THROW_CUSTOM_PROPERTIES_1(Failure, ZS_EVENTING_TOOL_INVALID_CONTENT, "Duplicate provider found in jman file: " + fileName);
            }
            
            mProviders[provider->mID] = provider;
          }
          
          if (Seconds() != mMonitorInfo.mTimeout) {
            mAutoQuitTimer = ITimer::create(mThisWeak.lock(), zsLib::now() + mMonitorInfo.mTimeout);
          }
          
          if (mMonitorInfo.mIPAddress.isAddressEmpty()) {
            mRemote = IRemoteEventing::listenForRemote(mThisWeak.lock(), mMonitorInfo.mPort, mMonitorInfo.mSecret);
            if (!mMonitorInfo.mQuietMode) {
              tool::output() << "[Info] Listening for remote connection: " << string(mMonitorInfo.mPort) << "\n";
            }
          } else {
            mRemote = IRemoteEventing::connectToRemote(mThisWeak.lock(), mMonitorInfo.mIPAddress, mMonitorInfo.mSecret);
            if (!mMonitorInfo.mQuietMode) {
              tool::output() << "[Info] Connecting to remote process: " << mMonitorInfo.mIPAddress.string() << "\n";
            }
          }
          
          if (!mRemote) {
            cancel();
            return;
          }
          
          for (auto iter = mProviders.begin(); iter != mProviders.end(); ++iter) {
            auto provider = (*iter).second;
            for (auto iterSubsystem = provider->mSubsystems.begin(); iterSubsystem != provider->mSubsystems.end(); ++iterSubsystem) {
              auto subsystem = (*iterSubsystem).second;
              mRemote->setRemoteLevel(subsystem->mName, subsystem->mLevel);
            }
          }
        }
        

      }
    }
  }
}
