/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and
 * confidential information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified, published,
 * uploaded, posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 */

#ifndef YamiVideoCompositor_h
#define YamiVideoCompositor_h

#include "VideoFrameMixer.h"
#include "VideoLayout.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <JobTimer.h>
#include <logger.h>
#include <MediaFramePipeline.h>
#include <vector>
#include <VideoCommonDefs.h>
#include <va/va.h>

struct VideoRect;

namespace YamiMediaCodec {
class IVideoPostProcess;
}

namespace woogeen_base {
class PooledFrameAllocator;
class YamiVideoInputManager;
}

namespace mcu {
class BackgroundCleaner;

/**
 * composite a sequence of frames into one frame based on current layout config,
 * there is a question of how many streams to be composited if there are 16 participants
 * but only 6 regions are configed. current implementation will only show 6 participants but
 * still 16 audios will be mixed. In the future, we may enable the video rotation based on VAD history.
 */
class YamiVideoCompositor : public VideoFrameCompositor,
                            public woogeen_base::JobTimerListener {
    DECLARE_LOGGER();
    enum LayoutSolutionState{UN_INITIALIZED = 0, CHANGING, IN_WORK};
public:
    YamiVideoCompositor(uint32_t maxInput, woogeen_base::VideoSize rootSize, woogeen_base::YUVColor bgColor);
    ~YamiVideoCompositor();

    bool activateInput(int input);
    void deActivateInput(int input);
    void pushInput(int input, const woogeen_base::Frame& frame);
    void updateRootSize(woogeen_base::VideoSize& rootSize);
    void updateBackgroundColor(woogeen_base::YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

    void onTimeout();

private:
    SharedPtr<VideoFrame> layout();
    SharedPtr<VideoFrame> customLayout();
    void generateFrame();
    void clearBackgroud(const SharedPtr<VideoFrame>& dest);

    // Delta used for translating between NTP and internal timestamps.
    int64_t m_ntpDelta;

    boost::shared_mutex m_mutex;
    woogeen_base::VideoSize m_compositeSize;
    uint32_t m_bgColor;
    LayoutSolution m_currentLayout;

    boost::shared_ptr<VADisplay> m_display;
    SharedPtr<woogeen_base::PooledFrameAllocator> m_allocator;
    SharedPtr<YamiMediaCodec::IVideoPostProcess> m_vpp;
    std::vector<boost::shared_ptr<woogeen_base::YamiVideoInputManager> > m_inputs;
    std::vector<VideoRect> m_inputRects;

    boost::shared_ptr<BackgroundCleaner> m_backgroundCleaner;

    // LayoutSolution m_newLayout;
    // LayoutSolutionState m_solutionState;
    boost::scoped_ptr<woogeen_base::JobTimer> m_jobTimer;
};

}
#endif /* YamiVideoCompositor_h*/