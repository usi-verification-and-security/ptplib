/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_COMMON_LIB_HPP
#define PTPLIB_COMMON_LIB_HPP

#include "Exception.hpp"
#include "PartitionConstant.hpp"

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <functional>

void split(const std::string & string, const std::string & delimiter,
           std::function<void(const std::string &)> callback, uint32_t limit = 0) {
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

std::vector<std::string> split(const std::string & string, const std::string & delimiter, uint32_t limit = 0) {
    std::vector<std::string> vector;
    split(string, delimiter, [&vector](const std::string & sub) {
        vector.push_back(sub);
    }, limit);
    return vector;
}

std::istream &
split(std::istream & stream, const char delimiter, std::function<void(const std::string &)> callback) {
    std::string sub;
    while (std::getline(stream, sub, delimiter)) {
        callback(sub);
    }
    return stream;
}


std::istream & split(std::istream & stream, const char delimiter, std::vector<std::string> & vector) {
    return split(stream, delimiter, [&vector](const std::string & sub) {
        vector.push_back(sub);
    });
}


std::string & replace(std::string & string, const std::string & from, const std::string & to, size_t n = 0) {
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

std::string & operator%(std::string & string, const std::pair<std::string, std::string> & pair) {
    return replace(string, pair.first, pair.second);
}


template<typename T>
std::ostream & join(std::ostream & stream, const std::string & delimiter, const std::vector<T> & vector) {
    for (auto it = vector.begin(); it != vector.end(); ++it) {
        stream << *it;
        if (it + 1 != vector.end())
            stream << delimiter;
    }
    return stream;
}


template<typename T>
std::ostream & operator<<(std::ostream & stream, const std::vector<T> & v) {
    for (auto & i:v) {
        stream << i << '\0';
    }
    return stream;
}

template<typename T>
std::istream & operator>>(std::istream & stream, std::vector<T> & v) {
    return split(stream, '\0', [&](std::string sub) {
        if (sub.size() == 0)
            return;
        T t;
        std::istringstream(sub) >> t;
        v.push_back(t);
    });
}

template<typename T>
const std::string to_string(const T & obj) {
    std::ostringstream ss;
    ss << obj;
    return ss.str();
}

template<>
const std::string to_string<bool>(const bool & b) {
    return b ? "true" : "false";
}

bool to_bool(const std::string & str) {
    return (str == "true" or str == "TRUE") ? true : false;
}

std::string get_task_name(int index) {
    auto input = static_cast<PTPLib::common::TASK>(index);
    switch (input) {
        case PTPLib::common::TASK::MEMORYCHECK:
            return PTPLib::common::TASK_STR[PTPLib::common::TASK::MEMORYCHECK];

        case PTPLib::common::TASK::COMMUNICATION:
            return PTPLib::common::TASK_STR[PTPLib::common::TASK::COMMUNICATION];

        case PTPLib::common::TASK::CLAUSEPUSH:
            return PTPLib::common::TASK_STR[PTPLib::common::TASK::CLAUSEPUSH];

        case PTPLib::common::TASK::CLAUSEPULL:
            return PTPLib::common::TASK_STR[PTPLib::common::TASK::CLAUSEPULL];

        case PTPLib::common::TASK::SOLVER:
            return PTPLib::common::TASK_STR[PTPLib::common::TASK::SOLVER];

        case PTPLib::common::TASK::CLAUSELEARN:
            return PTPLib::common::TASK_STR[PTPLib::common::TASK::CLAUSELEARN];
    }
    return "";
}
#endif // PTPLIB_COMMON_LIB_HPP