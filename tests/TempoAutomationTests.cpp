#include "TestSupport.h"

#include "core/TempoAutomation.h"

#include <cmath>
#include <vector>

int main()
{
    using namespace triumph;

    const std::vector<tempo::Point> ramp {
        { 0.0, 120.0, tempo::SegmentCurve::linear },
        { 4.0, 60.0, tempo::SegmentCurve::step }
    };
    const auto rampSeconds = tempo::beatToSeconds (4.0, ramp);
    REQUIRE (rampSeconds > 2.7 && rampSeconds < 2.8);
    REQUIRE (std::abs (tempo::secondsToBeat (rampSeconds, ramp) - 4.0) < 1.0e-8);
    REQUIRE (std::abs (tempo::bpmAtBeat (2.0, ramp) - 90.0) < 1.0e-8);

    const std::vector<automation::Point> smooth {
        { 0, 0.0f, automation::Curve::smooth },
        { 100, 1.0f, automation::Curve::linear }
    };
    REQUIRE (std::abs (automation::valueAtSample (50, smooth) - 0.5f) < 0.0001f);
    const std::vector<automation::Point> hold {
        { 0, 0.25f, automation::Curve::hold },
        { 100, 1.0f, automation::Curve::linear }
    };
    REQUIRE (std::abs (automation::valueAtSample (99, hold) - 0.25f) < 0.0001f);
    REQUIRE (! automation::shouldWrite (automation::Mode::read, true, true));
    REQUIRE (automation::shouldWrite (automation::Mode::touch, true, false));
    REQUIRE (! automation::shouldWrite (automation::Mode::touch, false, true));
    REQUIRE (automation::shouldWrite (automation::Mode::latch, false, true));
    REQUIRE (automation::shouldWrite (automation::Mode::write, false, false));

    struct CapturedParameterEvent
    {
        std::uint32_t sampleOffset = 0;
        float value = 0.0f;
    };
    std::vector<CapturedParameterEvent> parameterEvents;
    const std::vector<automation::Point> plugInAutomation {
        { 960, 0.20f, automation::Curve::hold },
        { 1008, 1.20f, automation::Curve::linear },
        { 1032, -0.20f, automation::Curve::linear },
        { 1100, 0.75f, automation::Curve::linear }
    };
    REQUIRE (automation::visitParameterEventsForBlock (
        1000, 64, plugInAutomation, 0.5f, -1.0f,
        [&] (std::uint32_t offset, float value)
    {
        parameterEvents.push_back ({ offset, value });
        return true;
    }));
    REQUIRE (parameterEvents.size() == 3);
    REQUIRE (parameterEvents[0].sampleOffset == 0);
    REQUIRE (std::abs (parameterEvents[0].value - 0.20f) < 0.0001f);
    REQUIRE (parameterEvents[1].sampleOffset == 8);
    REQUIRE (parameterEvents[1].value == 1.0f);
    REQUIRE (parameterEvents[2].sampleOffset == 32);
    REQUIRE (parameterEvents[2].value == 0.0f);

    parameterEvents.clear();
    REQUIRE (automation::visitParameterEventsForBlock (
        1000, 64, plugInAutomation, 0.5f, 0.20f,
        [&] (std::uint32_t offset, float value)
    {
        parameterEvents.push_back ({ offset, value });
        return true;
    }));
    REQUIRE (parameterEvents.size() == 2);
    REQUIRE (parameterEvents.front().sampleOffset == 8);

    const std::vector<automation::Signature> signatures {
        { 0.0, 4, 4 }, { 8.0, 3, 4 }
    };
    REQUIRE (automation::signatureAtBeat (7.0, signatures).numerator == 4);
    REQUIRE (automation::signatureAtBeat (8.0, signatures).numerator == 3);
    REQUIRE (automation::isBarAccent (11.0,
                                      automation::signatureAtBeat (11.0, signatures)));

    const std::vector<tempo::Point> fixedTempo {
        { 0.0, 120.0, tempo::SegmentCurve::step }
    };
    const auto events = automation::metronomeEventsForBlock (
        0, 48001, 48000.0, fixedTempo, { { 0.0, 4, 4 } });
    REQUIRE (events.size() == 3);
    REQUIRE (events[0].sampleOffset == 0);
    REQUIRE (events[0].accent);
    REQUIRE (events[1].sampleOffset == 24000);
    REQUIRE (! events[1].accent);
    REQUIRE (events[2].sampleOffset == 48000);

    return 0;
}
