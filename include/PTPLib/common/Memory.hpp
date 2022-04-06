/*
 * Copyright (c) Matteo Marescotti <Matteo.marescotti@usi.ch>
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_MEMORY_HPP
#define PTPLIB_MEMORY_HPP

#include "Exception.hpp"

namespace PTPLib {

    #if defined(__unix__) || defined(__linux__) || defined(__APPLE__)

        #define PTPLIB_MEMORY_SUPPORTED
        #include <sys/time.h>
        #include <sys/resource.h>
    
    #else
    #warning "measuring memory is not supported for the current OS"
    #endif

    size_t current_memory() {
        #ifdef PTPLIB_MEMORY_SUPPORTED
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) != 0) {
                throw Exception(__FILE__, __LINE__, "getrusage() error");
            }

        #ifdef __linux__
            return usage.ru_maxrss * 1024;
        #else
            return usage.ru_maxrss;
        #endif

        #else
            return 0;
        #endif
    }
}
#endif // PTPLIB_MEMORY_HPP