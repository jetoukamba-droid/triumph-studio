#include "core/AudioDeviceRecovery.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph::device;

    RecoveryTracker tracker;
    auto status = tracker.snapshot();
    REQUIRE (status.state == RecoveryState::stable);
    REQUIRE (status.interruptions == 0);

    tracker.noteUnavailable (true);
    tracker.noteUnavailable (false);
    status = tracker.snapshot();
    REQUIRE (status.state == RecoveryState::interrupted);
    REQUIRE (status.interruptions == 1);
    REQUIRE (status.resumePending);

    tracker.cancelResume();
    REQUIRE (! tracker.snapshot().resumePending);
    tracker.noteUnavailable (true);
    REQUIRE (tracker.snapshot().resumePending);

    REQUIRE (tracker.beginRecovery());
    REQUIRE (! tracker.beginRecovery());
    status = tracker.snapshot();
    REQUIRE (status.state == RecoveryState::reopening);
    REQUIRE (status.attempts == 1);

    tracker.markFailed();
    tracker.markFailed();
    status = tracker.snapshot();
    REQUIRE (status.state == RecoveryState::failed);
    REQUIRE (status.failures == 1);
    REQUIRE (status.resumePending);

    REQUIRE (tracker.beginRecovery());
    REQUIRE (tracker.markReady());
    REQUIRE (! tracker.markReady());
    status = tracker.snapshot();
    REQUIRE (status.state == RecoveryState::recovered);
    REQUIRE (status.attempts == 2);
    REQUIRE (status.recoveries == 1);
    REQUIRE (! status.resumePending);

    tracker.settleRecovered();
    REQUIRE (tracker.snapshot().state == RecoveryState::stable);

    tracker.noteUnavailable (false);
    REQUIRE (tracker.beginRecovery());
    REQUIRE (! tracker.markReady());
    status = tracker.snapshot();
    REQUIRE (status.interruptions == 2);
    REQUIRE (status.recoveries == 2);
    REQUIRE (! status.resumePending);
    return 0;
}
