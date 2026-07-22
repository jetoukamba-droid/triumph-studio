#include "core/ReleaseIdentity.h"
#include "TestSupport.h"

#include <string_view>

int main()
{
    using namespace triumph::release;

    REQUIRE (std::string_view (applicationName) == "Triumph Studio");
    REQUIRE (std::string_view (versionLabel) == "0.22.0-beta.1");
    REQUIRE (std::string_view (releaseChannel) == "Closed Beta 1");
    REQUIRE (isPrerelease);
    REQUIRE (hasBetaVersion());
    return 0;
}
