/*

 Copyright (c) 2014, Robin Raymond
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

#include <zsLib/date.h>
#include <zsLib/Event.h>
#include <zsLib/Exception.h>
#include <zsLib/helpers.h>
#include <zsLib/IFactory.h>
#include <zsLib/IHelper.h>
#include <zsLib/IMessageQueue.h>
#include <zsLib/IMessageQueueThread.h>
#include <zsLib/IMessageQueueThreadPool.h>
#include <zsLib/IMessageQueueManager.h>
#include <zsLib/IPAddress.h>
#include <zsLib/ISettings.h>
#include <zsLib/ITimer.h>
#include <zsLib/IWakeDelegate.h>
#include <zsLib/Log.h>
#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Numeric.h>
#include <zsLib/Promise.h>
#include <zsLib/Proxy.h>
#include <zsLib/ProxySubscriptions.h>
#include <zsLib/Singleton.h>
#include <zsLib/Socket.h>
#include <zsLib/String.h>
#include <zsLib/Stringize.h>
#include <zsLib/TearAway.h>
#include <zsLib/WeightedMovingAverage.h>
#include <zsLib/XML.h>
//#include <zsLib/zsLib.h>
#include <zsLib/types.h>

#include <zsLib/SafeInt.h>
