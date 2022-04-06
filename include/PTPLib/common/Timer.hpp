/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_COMMON_TIMER_HPP
#define PTPLIB_COMMON_TIMER_HPP

#include <string>
#include <iostream>
#include <chrono>

namespace PTPLib::common {

// A c++ wrapper for manual time checking
    class StoppableWatch {

    public:
        StoppableWatch(const StoppableWatch & other) = default;

        StoppableWatch(StoppableWatch && other) = default;

        virtual ~StoppableWatch() = default;

        StoppableWatch & operator=(const StoppableWatch & other) = default;

        StoppableWatch & operator=(StoppableWatch && other) = default;

        inline StoppableWatch( bool start = false) :
                started_(false), paused_(false),
                reference_( std::chrono::steady_clock::now()),
                accumulated_(std::chrono::duration<long double>(0)) {
            if (start)
                this->start();
        }

        inline void start() {
            if (not started_) {
                started_ = true;
                paused_ = false;
                accumulated_ = std::chrono::duration<long double>(0);
                reference_ = std::chrono::steady_clock::now();
            } else if (paused_) {
                reference_ = std::chrono::steady_clock::now();
                paused_ = false;
            }
        }

        inline void stop() {
            if (started_ && not paused_) {
                auto now = std::chrono::steady_clock::now();
                accumulated_ = accumulated_ +
                               std::chrono::duration_cast<std::chrono::duration<long double> >(now - reference_);
                paused_ = true;
            }
        }

        inline void reset() {
            if (started_) {
                started_ = false;
                paused_ = false;
                reference_ = std::chrono::steady_clock::now();
                accumulated_ = std::chrono::duration<long double>(0);
            }
        }

        inline std::uint_fast16_t elapsed_time_microseconds() {
            return count_elapsed<std::chrono::microseconds>();
        }

        inline std::uint_fast16_t elapsed_time_milliseconds() {
            return count_elapsed<std::chrono::milliseconds>();
        }

        inline std::uint_fast8_t elapsed_time_second() {
            return count_elapsed<std::chrono::seconds>();
        }

        template<class duration_t>
        typename duration_t::rep count_elapsed() const {
            if (started_) {
                if (paused_) {
                    return std::chrono::duration_cast<duration_t>(accumulated_).count();
                } else {
                    return std::chrono::duration_cast<duration_t>(
                            accumulated_ + (std::chrono::steady_clock::now() - reference_)).count();
                }
            } else {
                return duration_t(0).count();
            }
        }

    private:
        bool started_;
        bool paused_;
        std::chrono::steady_clock::time_point reference_;
        std::chrono::duration<long double> accumulated_;
    };
}
#endif // PTPLIB_COMMON_TIMER_HPP