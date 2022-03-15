/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_EXCEPTION_HPP
#define PTPLIB_EXCEPTION_HPP

#include <exception>
#include <iostream>

class Exception : public std::exception {
private:
    const std::string msg;

public:
    explicit Exception(const std::string & message) :
            msg(message) {}

    explicit Exception(const char * file, unsigned line, const std::string & message) :
            msg(message + " at " + file + ":" + std::to_string(line)) {}

    const char * what() const throw() { return this->msg.c_str(); }
};

#endif // PTPLIB_EXCEPTION_HPP