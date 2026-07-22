#include "core/RecordingStateMachine.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph::recording;

    StateMachine normal;
    REQUIRE (normal.get() == State::idle);
    REQUIRE (normal.arm());
    REQUIRE (normal.beginPreRoll());
    REQUIRE (normal.beginCapture());
    REQUIRE (normal.isCapturing());
    REQUIRE (normal.beginFinalize());
    REQUIRE (normal.get() == State::finalizing);
    REQUIRE (normal.finishFinalize (true));
    REQUIRE (normal.get() == State::finalized);
    REQUIRE (! normal.isActive());

    StateMachine punch;
    REQUIRE (punch.arm());
    REQUIRE (punch.beginCapture (CaptureMode::punch));
    REQUIRE (punch.get() == State::punch);
    REQUIRE (! punch.beginNextLoopPass());
    REQUIRE (punch.beginFinalize());
    REQUIRE (punch.finishFinalize (false));
    REQUIRE (punch.get() == State::aborted);
    REQUIRE (punch.markRecovered());
    REQUIRE (punch.get() == State::recovered);

    StateMachine loop;
    REQUIRE (loop.arm());
    REQUIRE (loop.beginCapture (CaptureMode::loop));
    REQUIRE (loop.getLoopPass() == 1);
    REQUIRE (loop.beginNextLoopPass());
    REQUIRE (loop.beginNextLoopPass());
    REQUIRE (loop.getLoopPass() == 3);
    REQUIRE (loop.beginFinalize());
    REQUIRE (loop.finishFinalize (true));
    loop.reset();
    REQUIRE (loop.get() == State::idle);
    REQUIRE (! loop.isActive());

    return 0;
}
