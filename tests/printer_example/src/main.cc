//
// Created by Antti on 14.03.22.
//

#include <PTPLib/Printer.hpp>

int main() {
    PTPLib::synced_stream stream;
    stream.print(PTPLib::Color::FG_BrightYellow, "foo");
    stream.println(PTPLib::Color::FG_BrightCyan, "bar");
    stream.print(PTPLib::Color::BG_BrightCyan, "   ");
    stream.println(PTPLib::Color::BG_BrightYellow, "   ");
    return 0;
}