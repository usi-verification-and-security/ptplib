/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_PARTITION_CONSTANT_HPP
#define PTPLIB_PARTITION_CONSTANT_HPP

#include <iostream>

namespace PTPLib
{
    static struct
    {
        const std::string Partition = "partition";
        const std::string Stop = "stop";
        const std::string ClauseInjection = "inject";
        const std::string Incremental = "incremental";
        const std::string CnfClauses = "cnf-clauses";
        const std::string Cnflearnts = "cnf-learnts";
        const std::string Solve = "solve";
        const std::string Lemmas = "lemmas";
        const std::string Terminate = "terminate";
    } Command;

    struct Task
    {
        const enum {
            incremental, resume, partition, stop
        } command;

        std::string smtlib;
    };

    enum Threads
    {
        Search, ClausePull, ClausePush, MemCheck
    };
}

#endif // PTPLIB_PARTITION_CONSTANT_HPP