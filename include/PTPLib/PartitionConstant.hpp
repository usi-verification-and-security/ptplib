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
        const std::string PARTITION = "partition";
        const std::string STOP = "stop";
        const std::string CLAUSEINJECTION = "inject";
        const std::string INCREMENTAL = "incremental";
        const std::string CNFCLAUSES = "cnf-clauses";
        const std::string CNFLEARNTS = "cnf-learnts";
        const std::string SOLVE = "solve";
        const std::string LEMMAS = "lemmas";
        const std::string TERMINATE = "terminate";
    } Command;

    static struct
    {
        const std::string NODE = "node";
        const std::string NODE_ = "node_";
        const std::string COMMAND = "command";
        const std::string QUERY = "query";
        const std::string NAME = "name";
        const std::string SEED = "seed";
        const std::string SPLIT_TYPE = "split-type";
        const std::string SPLIT_PREFERENCE = "split-preference";
        const std::string PARTITIONS = "partitions";
    } Param;


    enum TASK
    {
        COMMUNICATION, SOLVER, CLAUSEPULL, CLAUSEPUSH, MEMORYCHECK, CLAUSELEARN
    };
}

#endif // PTPLIB_PARTITION_CONSTANT_HPP