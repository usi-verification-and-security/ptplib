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
        std::string_view Partition = "partition";
        std::string_view Stop = "stop";
        std::string_view ClauseInjection = "inject";
        std::string_view Incremental = "incremental";
        std::string_view CnfClauses = "cnf-clauses";
        std::string_view Cnflearnts = "cnf-learnts";
        std::string_view Solve = "solve";
        std::string_view Lemmas = "lemmas";
        std::string_view Terminate = "terminate";
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
        Communication, ClausePull, ClausePush, MemCheck
    };

    enum Status
    {
        unknown, sat, unsat
    };

}

#endif // PTPLIB_PARTITION_CONSTANT_HPP