#include "TestSupport.h"

#include "core/AutomationWrite.h"

int main()
{
    using namespace triumph::automation_write;

    Pass touch (Mode::touch, 100, 8);
    touch.update (100, 0.1f);
    touch.beginGesture (110, 0.2f);
    touch.update (120, 0.4f);
    touch.endGesture (130, 0.6f);
    touch.update (140, 0.9f);
    const auto touched = touch.finish (150, 0.0f);
    REQUIRE (touched.front().sample == 110);
    REQUIRE (touched.back().sample == 130);

    Pass latch (Mode::latch, 0, 1);
    latch.beginGesture (10, 0.25f);
    latch.endGesture (20, 0.75f);
    latch.update (30, 0.8f);
    const auto latched = latch.finish (100, 0.0f);
    REQUIRE (latched.back().sample == 100);
    REQUIRE (latched.back().value == 0.8f);

    const std::vector<Point> line {
        { 0, 0.0f }, { 10, 0.1f }, { 20, 0.2f },
        { 30, 0.8f }, { 40, 0.4f }
    };
    const auto reduced = simplify (line, 0.01f);
    REQUIRE (reduced.size() == 4);
    REQUIRE (reduced[2].sample == 30);
    return 0;
}
