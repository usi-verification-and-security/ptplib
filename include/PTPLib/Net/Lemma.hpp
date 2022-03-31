/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_NET_LEMMA_H
#define PTPLIB_NET_LEMMA_H

#include <vector>
#include <sstream>


namespace PTPLib::Net {
    class Lemma {
    public:
        friend std::ostream &operator<<(std::ostream &stream, const Lemma &lemma) {
            return stream << std::to_string(lemma.level) << " " << lemma.clause;
        }

        friend std::istream &operator>>(std::istream &stream, Lemma &lemma) {
            stream >> lemma.level;
            lemma.clause = std::string(std::istreambuf_iterator<char>(stream), {});
            return stream;
        }

        Lemma(std::string const & c, const int l) : clause(c), level(l) {}

        Lemma() : Lemma(std::string() , 0) {}

        std::string clause;
        int level;
    };
}

#endif //PTPLIB_NET_LEMMA_H
