/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <exception>
#include <iostream>

class Exception : public std::exception {
private:
    const std::string msg;

public:
    explicit Exception(std::string_view message) :
            msg(message) {}

    explicit Exception(const char * file, unsigned line, const std::string & message) :
            msg(message + " at " + file + ":" + std::to_string(line)) {}

    const char * what() const throw() { return this->msg.c_str(); }
};

