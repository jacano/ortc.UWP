/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_BASE_VIDEOBROADCASTER_H_
#define WEBRTC_MEDIA_BASE_VIDEOBROADCASTER_H_

#include <memory>
#include <utility>
#include <vector>

#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread_checker.h"
#include "webrtc/media/base/videoframe.h"
#include "webrtc/media/base/videosinkinterface.h"
#include "webrtc/media/base/videosourcebase.h"
#include "webrtc/media/engine/webrtcvideoframe.h"

namespace rtc {

// VideoBroadcaster broadcast video frames to sinks and combines
// VideoSinkWants from its sinks. It does that by implementing
// rtc::VideoSourceInterface and rtc::VideoSinkInterface.
// Sinks must be added and removed on one and only one thread.
// Video frames can be broadcasted on any thread. I.e VideoBroadcaster::OnFrame
// can be called on any thread.
class VideoBroadcaster : public VideoSourceBase,
                         public VideoSinkInterface<cricket::VideoFrame> {
 public:
  VideoBroadcaster();
  bool Suspend() override { return false; }
  bool Resume() override { return false; }
  bool IsSuspended() override { return false; }
  void SetIsH264Source(bool isH264) override { }
  bool IsH264Source() override { return false; }
  void AddOrUpdateSink(VideoSinkInterface<cricket::VideoFrame>* sink,
                       const VideoSinkWants& wants) override;
  void RemoveSink(VideoSinkInterface<cricket::VideoFrame>* sink) override;

  // Returns true if the next frame will be delivered to at least one sink.
  bool frame_wanted() const;

  // Returns VideoSinkWants a source is requested to fulfill. They are
  // aggregated by all VideoSinkWants from all sinks.
  VideoSinkWants wants() const;

  void OnFrame(const cricket::VideoFrame& frame) override;

 protected:
  void UpdateWants() EXCLUSIVE_LOCKS_REQUIRED(sinks_and_wants_lock_);
  const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& GetBlackFrameBuffer(
      int width, int height)
      EXCLUSIVE_LOCKS_REQUIRED(sinks_and_wants_lock_);

  ThreadChecker thread_checker_;
  rtc::CriticalSection sinks_and_wants_lock_;

  VideoSinkWants current_wants_ GUARDED_BY(sinks_and_wants_lock_);
  rtc::scoped_refptr<webrtc::VideoFrameBuffer> black_frame_buffer_;
};

}  // namespace rtc

#endif  // WEBRTC_MEDIA_BASE_VIDEOBROADCASTER_H_