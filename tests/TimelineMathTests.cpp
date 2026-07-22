#include "core/TimelineMath.h"
#include "core/TimeStretch.h"
#include "core/TransientDetector.h"
#include "core/OfflineRenderMath.h"
#include "core/WarpMap.h"
#include "TestSupport.h"
#include <cmath>

namespace
{
bool closeEnough (double a, double b)
{
    return std::abs (a - b) < 0.00001;
}
}

int main()
{
    using namespace triumph::timeline;

    REQUIRE (closeEnough (secondsToPixels (30.0, 60.0, 1000.0), 500.0));
    REQUIRE (closeEnough (secondsToPixels (90.0, 60.0, 1000.0), 1000.0));
    REQUIRE (closeEnough (pixelsToSeconds (250.0, 1000.0, 60.0), 15.0));
    REQUIRE (closeEnough (pixelsToSeconds (-50.0, 1000.0, 60.0), 0.0));
    REQUIRE (secondsToSamples (1.5, 48000.0) == 72000);
    REQUIRE (closeEnough (samplesToSeconds (72000, 48000.0), 1.5));
    REQUIRE (closeEnough (samplesToMilliseconds (480, 48000.0), 10.0));
    REQUIRE (closeEnough (samplesToMilliseconds (480, 0.0), 0.0));
    REQUIRE (convertSampleCount (44100, 44100.0, 48000.0) == 48000);
    REQUIRE (compensateInputLatency (48000, 480, 48000.0, 48000.0) == 47520);
    REQUIRE (compensateInputLatency (100, 480, 48000.0, 48000.0) == 0);
    REQUIRE (compensateInputLatency (48000, 441, 44100.0, 48000.0) == 47520);
    REQUIRE (compensateInputLatency (48000, 480, 48000.0, 48000.0, 120)
             == 47640);
    REQUIRE (compensateInputLatency (48000, 480, 48000.0, 48000.0, -120)
             == 47400);
    REQUIRE (snapSamples (23999, 12000) == 24000);
    REQUIRE (snapSamples (5000, 0) == 5000);
    REQUIRE (rangesOverlap (0, 100, 99, 20));
    REQUIRE (! rangesOverlap (0, 100, 100, 20));

    for (long long position = 0; position <= 480; position += 24)
    {
        const auto fadeIn = equalPowerFadeIn (position, 480);
        const auto fadeOut = equalPowerFadeOut (position, 480);
        REQUIRE (std::abs (fadeIn * fadeIn + fadeOut * fadeOut - 1.0f)
                 < 0.0001f);
    }
    REQUIRE (equalPowerFadeIn (0, 480) == 0.0f);
    REQUIRE (equalPowerFadeOut (480, 480) == 0.0f);

    REQUIRE (closeEnough (triumph::stretch::clampRatio (0.1), 0.25));
    REQUIRE (closeEnough (triumph::stretch::clampRatio (8.0), 4.0));
    REQUIRE (closeEnough (triumph::stretch::playbackRate (2.0), 0.5));
    REQUIRE (triumph::stretch::timelineToNativeSamples (96000, 2.0) == 48000);
    REQUIRE (triumph::stretch::nativeToTimelineSamples (48000, 2.0) == 96000);

    const auto anchors = triumph::warp::normalise ({ { 96000, 96000 },
                                                      { 0, 0 },
                                                      { 24000, 48000 } });
    REQUIRE (triumph::warp::sourceAt (anchors, 12000) == 24000);
    REQUIRE (triumph::warp::sourceAt (anchors, 60000) == 72000);
    REQUIRE (closeEnough (triumph::warp::playbackRate (
                              anchors, 0, 24000, 48000.0, 48000.0), 2.0));
    REQUIRE (triumph::warp::timelineAt (anchors, 72000) == 60000);

    std::vector<float> onsetSignal (12000, 0.0f);
    for (const auto onset : { 2048, 6144, 10240 })
        for (int index = 0; index < 256; ++index)
            onsetSignal[static_cast<std::size_t> (onset + index)] = 0.9f;
    triumph::transient::Settings detectorSettings;
    detectorSettings.minimumSpacingSamples = 1000;
    const auto transients = triumph::transient::detect (
        onsetSignal.data(), static_cast<std::int64_t> (onsetSignal.size()),
        detectorSettings);
    REQUIRE (transients.size() == 3);
    REQUIRE (std::abs (transients.front().sample - 2048) <= 256);

    triumph::offline::DeterministicDither ditherA;
    triumph::offline::DeterministicDither ditherB;
    for (int sample = 0; sample < 128; ++sample)
    {
        const auto first = ditherA.next24BitTpdf();
        REQUIRE (first == ditherB.next24BitTpdf());
        REQUIRE (std::abs (first) <= 1.0f / 8388608.0f);
    }
    triumph::offline::DeterministicDither dither16;
    for (int sample = 0; sample < 128; ++sample)
        REQUIRE (std::abs (dither16.nextTpdf (16)) <= 1.0f / 32768.0f);
    REQUIRE (dither16.nextTpdf (32) == 0.0f);
    REQUIRE (closeEnough (triumph::offline::linearSample (0.0f, 1.0f, 0.25),
                          0.25));
    REQUIRE (triumph::offline::renderLengthSamples (
                 48000, 48000.0, 48000.0, 0.1) == 52800);
    REQUIRE (triumph::offline::renderLengthSamples (
                 44100, 44100.0, 48000.0, 2.0) == 144000);
    REQUIRE (triumph::offline::renderLengthSamples (
                 48000, 48000.0, 48000.0, -1.0) == 48000);

    const auto left = balanceForPan (-1.0f);
    REQUIRE (closeEnough (left.left, 1.0));
    REQUIRE (closeEnough (left.right, 0.0));

    const auto centre = balanceForPan (0.0f);
    REQUIRE (closeEnough (centre.left, 1.0));
    REQUIRE (closeEnough (centre.right, 1.0));

    const auto right = balanceForPan (1.0f);
    REQUIRE (closeEnough (right.left, 0.0));
    REQUIRE (closeEnough (right.right, 1.0));

    return 0;
}
