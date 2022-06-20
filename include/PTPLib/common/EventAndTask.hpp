/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_COMMON_EVENTANDTASK_H
#define PTPLIB_COMMON_EVENTANDTASK_H

namespace PTPLib::common {
    template<typename MOVABLE, typename LAMBDA>
    class EventAndTask_move {
        MOVABLE x;
        LAMBDA f;
    public:
        EventAndTask_move(MOVABLE && x, LAMBDA && f)
        : x{ std::forward<MOVABLE>(x) }, f{ std::forward<LAMBDA>(f) }
        {}

        template<typename ...Ts>
        auto operator()(Ts && ...args)
        -> decltype(f(x, std::forward<Ts>(args)...)) {
            return f(x, std::forward<Ts>(args)...);
        }

        template<typename ...Ts>
        auto operator()(Ts && ...args) const
        -> decltype(f(x, std::forward<Ts>(args)...)) {
            return f(x, std::forward<Ts>(args)...);
        }
    };

    template<typename MOVABLE, typename LAMBDA>
    EventAndTask_move<MOVABLE, LAMBDA> EventAndTask(MOVABLE && x, LAMBDA && f) {
        return EventAndTask_move<MOVABLE, LAMBDA>(std::forward<MOVABLE>(x), std::forward<LAMBDA>(f));
    }
}

#endif //PTPLIB_COMMON_EVENTANDTASK_H
