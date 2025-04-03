/*
 * Copyright 2021, Barak Shoshany, doi:10.5281/zenodo.4742687, arXiv:2105.00613
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#ifndef PTPLIB_THREADS_THREADPOOl_HPP
#define PTPLIB_THREADS_THREADPOOl_HPP

#include "PTPLib/common/Printer.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <cassert>

namespace PTPLib::threads {

    class ThreadPool {
        typedef std::uint_fast32_t ui32;

        std::string pool_name;

        mutable std::mutex queue_mutex = {};

        std::atomic<bool> running = true;

        std::queue<std::pair<std::function<void()>, std::string>> tasks = {};

        std::unique_ptr<std::vector<std::thread>> threads;

        std::atomic<ui32> tasks_total = 0;

        PTPLib::common::synced_stream * syncedStream = nullptr;
    public:
        ThreadPool(std::string _pool_name = std::string(), const ui32 _thread_count = 0)
        : pool_name    (_pool_name) {
            threads = std::make_unique<std::vector<std::thread>>();
            if (_thread_count != 0)
                create_threads(_thread_count);
            else create_threads(std::thread::hardware_concurrency() - 1);
        }

        ~ThreadPool() {
            wait_for_tasks();
            running = false;
            destroy_threads();
            if (syncedStream)
                syncedStream->println(PTPLib::common::Color::FG_BrightRed, pool_name, " destroyed!");
        }

        void set_syncedStream(PTPLib::common::synced_stream & ss) { syncedStream = &ss; }

        size_t get_tasks_queued() const {
            const std::scoped_lock lock(queue_mutex);
            return tasks.size();
        }


        ui32 get_tasks_running() const {
            return tasks_total - (ui32) get_tasks_queued();
        }


        ui32 get_tasks_total() const {
            return tasks_total;
        }

        std::size_t get_thread_count() const {
            return threads->size();
        }


        template<typename T, typename F>
        void parallelize_loop(T first_index, T last_index, const F & loop, ui32 num_tasks = 0) {
            if (num_tasks == 0)
                num_tasks = get_thread_count();
            if (last_index < first_index)
                std::swap(last_index, first_index);
            size_t total_size = last_index - first_index + 1;
            size_t block_size = total_size / num_tasks;
            if (block_size == 0) {
                block_size = 1;
                num_tasks = (ui32) total_size > 1 ? (ui32) total_size : 1;
            }
            std::atomic<ui32> blocks_running = 0;
            for (ui32 t = 0; t < num_tasks; t++) {
                T start = (T) (t * block_size + first_index);
                T end = (t == num_tasks - 1) ? last_index : (T) ((t + 1) * block_size + first_index - 1);
                blocks_running++;
                push_task([start, end, &loop, &blocks_running] {
                    for (T i = start; i <= end; i++)
                        loop(i);
                    blocks_running--;
                });
            }
            while (blocks_running != 0) {
                sleep_or_yield();
            }
        }

        template<typename F, typename R = std::invoke_result_t<std::decay_t<F>>>
        std::future<R> submit_task(const F & task, std::string task_name=std::string()) {
            std::shared_ptr<std::promise<R>> task_promise(new std::promise<R>);
            std::future<R> future = task_promise->get_future();
            push_task([task, task_promise] {
                try {
                    task_promise->set_value(task());
                }
                catch (...) {
                    try {
                        task_promise->set_exception(std::current_exception());
                    }
                    catch (...) {
                    }
                }
            }, task_name);
            return future;
        }

        template<typename F>
        void push_task(const F & task, std::string task_name = std::string()) {
            tasks_total++;
            {
                const std::scoped_lock lock(queue_mutex);
                tasks.push(std::make_pair(std::function<void()>(task), task_name));
            }
        }

        void reset(const ui32 & _thread_count = std::thread::hardware_concurrency() - 1) {
            bool was_paused = paused;
            paused = true;
            wait_for_tasks();
            running = false;
            destroy_threads();
            threads->clear();
            paused = was_paused;
            create_threads(_thread_count);
            running = true;
        }


        template<typename F, typename... A, typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>>>
        std::future<bool> submit(const F & task, const A & ...args, std::string task_name=std::string()) {
            std::shared_ptr<std::promise<bool>> task_promise(new std::promise<bool>);
            std::future<bool> future = task_promise->get_future();
            push_task([task, args..., task_promise] {
                try {
                    task(args...);
                    task_promise->set_value(true);
                }
                catch (...) {
                    try {
                        task_promise->set_exception(std::current_exception());
                    }
                    catch (...) {
                    }
                }
            }, task_name);
            return future;
        }


        template<typename F, typename... A, typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>,
                typename = std::enable_if_t<!std::is_void_v<R>>>
        std::future<R> submit(const F & task, const A & ...args, std::string task_name=std::string()) {
            std::shared_ptr<std::promise<R>> task_promise(new std::promise<R>);
            std::future<R> future = task_promise->get_future();
            push_task([task, args..., task_promise] {
                try {
                    task_promise->set_value(task(args...));
                }
                catch (...) {
                    try {
                        task_promise->set_exception(std::current_exception());
                    }
                    catch (...) {
                    }
                }
            }, task_name);
            return future;
        }

        void wait_for_tasks()
        {
            while (true)
            {
                if (!paused)
                {
                    if (tasks_total == 0)
                        break;
                }
                else
                {
                    if (get_tasks_running() == 0)
                        break;
                }
                sleep_or_yield();
            }
        }

        void destroy_threads() {
            for (std::size_t i = 0; i < get_thread_count(); ++i) {
                threads->at(i).join();
            }
        }

        void increase(ui32 tc) {
            assert(get_thread_count() < std::thread::hardware_concurrency());
            for (ui32 i = 0; i < tc; ++i) {
                threads->push_back(std::thread(&ThreadPool::worker, this));
            }
        }

        std::atomic<bool> paused = false;

        ui32 sleep_duration = 1000;

    private:
        void create_threads(const ui32 _thread_count) {
            assert(_thread_count < std::thread::hardware_concurrency());
            for (ui32 i = 0; i < _thread_count; ++i) {
                threads->push_back(std::thread(&ThreadPool::worker, this));
            }
        }

        bool pop_task(std::pair<std::function<void()>, std::string> & task) {
            const std::scoped_lock lock(queue_mutex);
            if (tasks.empty())
                return false;
            else {
                task = std::move(tasks.front());
                tasks.pop();
                return true;
            }
        }

        void sleep_or_yield() {
            if (sleep_duration)
                std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration));
            else
                std::this_thread::yield();
        }

        void worker() {
            while (running) {
                std::pair<std::function<void()>, std::string> task;
                if (!paused && pop_task(task)) {
                    if (syncedStream)
                        syncedStream->println(PTPLib::common::Color::FG_Yellow, "THREAD_POOL -> TASK STARTED : ", task.second);

                    task.first();
                    tasks_total--;

                    if (syncedStream)
                        syncedStream->println(PTPLib::common::Color::FG_Yellow, "THREAD_POOL -> TASK ENDED : ", task.second, " REMAINED TASKS: ", tasks_total);
                }
                else {
                    sleep_or_yield();
                }
            }
        }
    };
}
#endif // PTPLIB_THREADS_THREADPOOl_HPP
