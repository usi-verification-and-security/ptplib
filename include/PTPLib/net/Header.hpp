/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_NET_HEADER_HPP
#define PTPLIB_NET_HEADER_HPP

#include "PTPLib/common/Lib.hpp"

#include <array>
#include <iomanip>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sstream>


namespace PTPLib::net {

    typedef std::string header_prefix;
    const header_prefix parameter = "parameter";
    const header_prefix statistic = "statistic";

    class Header : public std::map<std::string, std::string> {

        friend std::istream & operator>>(std::istream & stream, Header & header) {
            char c;
            do {
                stream.get(c);
            } while (c && c != '{');
            if (!c)
                return stream;
            std::pair <std::string, std::string> pair;
            std::string * s = &pair.first;
            bool escape = false;
            while (stream.get(c)) {
                if (!escape && s->size() == 0 && c == ' ')
                    continue;
                if (!escape && s->size() == 0) {
                    if (c == '}' && s == &pair.first)
                        break;
                    if (c == '"') {
                        if (!stream.get(c))
                            throw PTPLib::common::Exception(__FILE__, __LINE__, "unexpected end");
                    } else
                        throw PTPLib::common::Exception(__FILE__, __LINE__, "double quotes expected");
                }
                if (!escape) {
                    switch (c) {
                        case '\\':
                            escape = true;
                            continue;
                        case '"':
                            while (stream.get(c) && c == ' ') {
                            }
                            if (s == &pair.first) {
                                if (c != ':')
                                    throw PTPLib::common::Exception(__FILE__, __LINE__, "colon expected");
                                s = &pair.second;
                                continue;
                            } else {
                                header.insert(pair);
                                if (c == '}')
                                    stream.unget();
                                else if (c != ',')
                                    throw PTPLib::common::Exception(__FILE__, __LINE__, "comma expected");
                                pair.first.clear();
                                pair.second.clear();
                                s = &pair.first;
                            }
                            break;
                        default:
                            if ('\x00' <= c && c <= '\x1f')
                                throw PTPLib::common::Exception(__FILE__, __LINE__, "control char not allowed");
                            *s += c;
                    }
                } else {
                    escape = false;
                    char i = 0;
                    switch (c) {
                        case '"':
                        case '\\':
                        case 'b':
                        case 'f':
                        case 'n':
                        case 'r':
                        case 't':
                            *s += c;
                            break;
                        case 'u':
                            if (!(stream.get(c) && c == '0' && stream.get(c) && c == '0'))
                                throw PTPLib::common::Exception(__FILE__, __LINE__, "unicode not supported");
                            for (uint8_t _ = 0; _ < 2; _++) {
                                if (!stream.get(c))
                                    throw PTPLib::common::Exception(__FILE__, __LINE__, "unexpected end");
                                c = (char) toupper(c);
                                if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A')))
                                    throw PTPLib::common::Exception(__FILE__, __LINE__, "bad hex string");
                                c -= '0';
                                if (c > 9)
                                    c -= 7;
                                i = (i << 4) + c;
                            }
                            *s += i;
                            break;
                        default:
                            throw PTPLib::common::Exception(__FILE__, __LINE__, "bad char after escape");
                    }
                }
            }
            return stream;
        }

        friend std::ostream & operator<<(std::ostream & stream, const Header & header) {
            std::vector <std::string> pairs;
            for (auto & pair:header) {
                std::ostringstream ss;
                for (auto value:std::array<const std::string *, 2>({{&pair.first, &pair.second}})) {
                    ss << "\"";
                    for (auto & c:*value) {
                        switch (c) {
                            case '"':
                                ss << "\\\"";
                                break;
                            case '\\':
                                ss << "\\\\";
                                break;
                            case '\b':
                                ss << "\\b";
                                break;
                            case '\f':
                                ss << "\\f";
                                break;
                            case '\n':
                                ss << "\\n";
                                break;
                            case '\r':
                                ss << "\\r";
                                break;
                            case '\t':
                                ss << "\\t";
                                break;
                            default:
                                if ('\x00' <= c && c <= '\x1f') {
                                    ss << "\\u"
                                       << std::hex << std::setw(4) << std::setfill('0') << (int) c;
                                } else {
                                    ss << c;
                                }
                        }
                    }
                    ss << "\"";
                    if (value == &pair.first)
                        ss << ":";
                }
                pairs.push_back(ss.str());
            }
            return join(stream << "{", ",", pairs) << "}";
        }

    public:
        uint8_t level() const {
            std::string node;
            try {
                node = this->at(PTPLib::common::Param.NODE);
            } catch (std::out_of_range &) {
            }
            node  % std::make_pair("[", "") % std::make_pair("]", "") % std::make_pair(" ", "");
            auto v = split(node, ",");
            return (uint8_t)(v.size() / 2);
        }

        PTPLib::net::Header copy(const std::vector <std::string> & keys) const {
            auto header = PTPLib::net::Header();
            for (auto & key:keys) {
                try {
                    header[key] = this->at(key);
                } catch (std::out_of_range &) {
                }
            }
            return header;
        }

        PTPLib::net::Header copy(const header_prefix & prefix, const std::vector <std::string> & keys) const {
            auto header = Header();
            for (auto & key:keys) {
                header.set(prefix, key, this->get(prefix, key));
            }
            return header;
        }

        const std::vector <std::string> keys(const header_prefix & prefix ) const {
            std::vector <std::string> st;
            for (auto & pair:*this) {
                if (pair.first.substr(0, prefix.size() + 1) == prefix + ".") {
                    st.push_back(pair.first.substr(prefix.size() + 1));
                }
            }
            return st;
        }

        const std::vector <std::string> keys() const {
            std::vector <std::string> st;
            for (auto & pair:*this) {
                st.push_back(pair.first);
            }
            return st;
        }

        const std::string & get(const header_prefix & prefix, const std::string & key) const {
            static const std::string empty = "";
            try {
                return this->at(prefix + "." + key);
            } catch (std::out_of_range &) {
                return empty;
            }
        }

        void set(const header_prefix & prefix, const std::string & key, const std::string & value) {
            (*this)[prefix + "." + key] = value;
        }

        void remove(const header_prefix & prefix, const std::string & key) {
            this->erase(prefix + "." + key);
        }
    };
    
}

#endif // PTPLIB_NET_HEADER_HPP