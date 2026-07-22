#include "TestSupport.h"

#include "core/ResponsiveLayout.h"

int main()
{
    using namespace triumph::layout;
    REQUIRE (useCompactToolbars (1280));
    REQUIRE (useCompactToolbars (1379));
    REQUIRE (useCompactToolbars (1380));
    REQUIRE (useCompactToolbars (1499));
    REQUIRE (! useCompactToolbars (1500));
    REQUIRE (! useCompactToolbars (1600));
    REQUIRE (fitsTargetWidth (1280));
    REQUIRE (fitsTargetWidth (1365));
    REQUIRE (fitsTargetWidth (1440));
    REQUIRE (fitsTargetWidth (1600));
    REQUIRE (requiredTopToolbarWidth (true)
             < requiredTopToolbarWidth (false));
    REQUIRE (requiredTransportToolbarWidth (true)
             < requiredTransportToolbarWidth (false));
    REQUIRE (minimumWindowWidth == 1280);
    REQUIRE (minimumWindowHeight == 720);
    return 0;
}
