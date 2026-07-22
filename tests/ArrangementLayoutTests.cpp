#include "core/ArrangementLayout.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph::arrangement;

    REQUIRE (eventDisplayOriginX() == trackHeaderWidth + eventDisplayInset);
    REQUIRE (rulerOriginX() == rowContentInsetX + eventDisplayOriginX());
    REQUIRE (clampTrackHeight (32) == preferredTrackHeight);
    REQUIRE (clampTrackHeight (120) == 120);
    REQUIRE (clampTrackHeight (320) == maximumTrackHeight);
    REQUIRE (! rulerUsesPanelLabel());
    REQUIRE (! newInstrumentTrackCreatesClip());

    return 0;
}
