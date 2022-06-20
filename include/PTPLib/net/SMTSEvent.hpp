/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_NET_SMTSEVENT_H
#define PTPLIB_NET_SMTSEVENT_H

#include "PTPLib/net/Header.hpp"

#include <vector>
#include <sstream>


namespace PTPLib::net {
    struct SMTS_Event {
        PTPLib::net::Header header;
        std::string body;

        SMTS_Event() {}

        template <typename HD, typename PL>
        SMTS_Event(HD && hd, PL && str) {
            this->header = std::forward<HD>(hd);
            this->body = std::forward<PL>(str);
        }

        template <typename HD, typename PL>
        SMTS_Event(HD & hd, PL && str) {
            this->header = hd;
            this->body = std::forward<PL>(str);
        }

        SMTS_Event(PTPLib::net::Header & hd) {
            this->header = hd;
            this->body = std::string();
        }

        SMTS_Event(PTPLib::net::Header && hd) {
            this->header = std::move(hd);
            this->body = std::string();
        }

        bool empty() const { return header.empty(); }
    };
}

#endif //PTPLIB_NET_SMTSEVENT_H
