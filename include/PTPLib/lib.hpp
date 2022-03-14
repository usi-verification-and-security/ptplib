/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_LIB_HPP
#define PTPLIB_LIB_HPP

#include "PTPLib/Exception.h"
#include "PTPLib/ChannelConfig.h"

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <functional>


void split(std::string_view string, std::string_view delimiter, std::function<void(std::string_view)> callback, uint32_t limit = 0)
{
    size_t b = 0;
    size_t e;
    while (true) {
        if (limit != 0 && --limit == 0)
            e = std::string::npos;
        else
            e = string.find(delimiter, b);
        callback(string.substr(b, e - b));
        if (e == std::string::npos)
            return;
        b = e + delimiter.size();
    }
}

std::vector<std::string> split(std::string_view string, std::string_view delimiter, uint32_t limit = 0)
{
    std::vector<std::string> vector;
    split(string, delimiter, [&vector](std::string_view sub)
    {
        vector.push_back(sub);
    }, limit);
    return vector;
}

std::istream & split(std::istream & stream, const char delimiter, std::function<void(std::string_view)> callback)
{
    std::string sub;
    while (std::getline(stream, sub, delimiter))
    {
        callback(sub);
    }
    return stream;
}


std::istream & split(std::istream & stream, const char delimiter, std::vector<std::string> & vector)
{
    return split(stream, delimiter, [&vector](std::string_view sub)
    {
        vector.push_back(sub);
    });
}


std::string & replace(std::string & string, std::string_view from, std::string_view to, size_t n = 0)
{
    if (from.empty())
        return string;
    size_t start_pos = 0;
    while ((start_pos = string.find(from, start_pos)) != std::string::npos) {
        string.replace(start_pos, from.length(), to);
        start_pos += to.length();
        if (n > 0) {
            if (n == 1)
                break;
            n--;
        }
    }
    return string;
}

std::string & operator%(std::string & string, const std::pair<std::string, std::string> & pair)
{
    return ::replace(string, pair.first, pair.second);
}


template<typename T>
std::ostream &join(std::ostream &stream, std::string_view delimiter, const std::vector<T> &vector)
{
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        stream << *it;
        if (it + 1 != vector.end())
            stream << delimiter;
    }
    return stream;
}


template<typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &v)
{
    for (auto &i:v) {
        stream << i << '\0';
    }
    return stream;
}

template<typename T>
std::istream &operator>>(std::istream &stream, std::vector<T> &v)
{
    return ::split(stream, '\0', [&](std::string_view sub) {
        if (sub.size() == 0)
            return;
        T t;
        std::istringstream(sub) >> t;
        v.push_back(t);
    });
}

template<typename T>
const std::string to_string(const T &obj)
{
    std::ostringstream ss;
    ss << obj;
    return ss.str();
}

template<>
const std::string to_string<bool>(const bool & b)
{
    return b ? "true" : "false";
}

#endif // PTPLIB_LIB_HPP