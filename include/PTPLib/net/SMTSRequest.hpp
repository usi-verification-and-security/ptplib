/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_NET_SMTSREQUEST_H
#define PTPLIB_NET_SMTSREQUEST_H

#include "PTPLib/net/Header.hpp"

#include <vector>
#include <sstream>


namespace PTPLib::net {
    struct SMTSRequest : public std::pair<PTPLib::net::Header, std::string> {
        using std::pair<PTPLib::net::Header, std::string>::pair;
        PTPLib::net::Header & header = this->first;
        std::string & body = this->second;
    };
}

#endif //PTPLIB_NET_SMTSREQUEST_H
