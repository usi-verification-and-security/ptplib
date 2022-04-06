//
// Created by Antti on 14.03.22.
//

#include <PTPLib/common/Printer.hpp>

int main() {
    PTPLib::common::synced_stream stream;
    stream.print(PTPLib::common::Color::FG_BrightYellow, "foo");
    stream.println(PTPLib::common::Color::FG_BrightCyan, "bar");
    stream.print(PTPLib::common::Color::BG_BrightCyan, "   ");
    stream.println(PTPLib::common::Color::BG_BrightYellow, "   ");
    return 0;
}