//
// Created by Antti on 14.03.22.
//

#include <PTPLib/Printer.hpp>

int main() {
    partitionChannel::synced_stream stream;
    stream.print(partitionChannel::Color::FG_BrightYellow, "foo");
    stream.println(partitionChannel::Color::FG_BrightCyan, "bar");
    stream.print(partitionChannel::Color::BG_BrightCyan, "   ");
    stream.println(partitionChannel::Color::BG_BrightYellow, "   ");
    return 0;
}