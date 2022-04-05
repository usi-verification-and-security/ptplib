/*
 * Copyright 2021, Barak Shoshany, doi:10.5281/zenodo.4742687, arXiv:2105.00613
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#ifndef PTPLIB_ThreadPool_HPP
#define PTPLIB_ThreadPool_HPP

#include "Printer.hpp"
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

namespace PTPLib {

    class ThreadPool {
        typedef std::uint_fast32_t ui32;

        std::string pool_name;

        mutable std::mutex queue_mutex = {};

        std::atomic<bool> running = true;

        std::queue<std::pair<std::function<void()>, std::string>> tasks = {};

        ui32 thread_count;

        std::unique_ptr<std::thread[]> threads;

        std::atomic<ui32> tasks_total = 0;

        PTPLib::synced_stream & stream;

    public:
        ThreadPool(PTPLib::synced_stream & ss,
                std::string _pool_name = std::string(), const ui32 & _thread_count = std::thread::hardware_concurrency())
        :
            pool_name    (_pool_name),
            thread_count (_thread_count),
            threads      (new std::thread[_thread_count]),
            stream  (ss)
        {
           create_threads();
        }

        ~ThreadPool() {
            wait_for_tasks();
            running = false;
            destroy_threads();
            std::cout << pool_name <<" destroyed!" << std::endl;
        }

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

        ui32 get_thread_count() const {
            return thread_count;
        }


        template<typename T, typename F>
        void parallelize_loop(T first_index, T last_index, const F & loop, ui32 num_tasks = 0) {
            if (num_tasks == 0)
                num_tasks = thread_count;
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


        template<typename F>
        void push_task(const F & task, std::string task_name = std::string()) {
            tasks_total++;
            {
                const std::scoped_lock lock(queue_mutex);
                tasks.push(std::make_pair(std::function<void()>(task), task_name));
            }
        }


        template<typename F, typename... A>
        void push_task(const F & task, const A & ...args) {
            push_task([task, args...]
                      { task(args...); });
        }


        void reset(const ui32 & _thread_count = std::thread::hardware_concurrency()) {
            bool was_paused = paused;
            paused = true;
            wait_for_tasks();
            running = false;
            destroy_threads();
            thread_count = _thread_count ? _thread_count : std::thread::hardware_concurrency();
            threads.reset(new std::thread[thread_count]);
            paused = was_paused;
            create_threads();
            running = true;
        }


        template<typename F, typename... A, typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>>>
        std::future<bool> submit(const F & task, const A & ...args) {
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
            });
            return future;
        }


        template<typename F, typename... A, typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>,
                typename = std::enable_if_t<!std::is_void_v<R>>>
        std::future<R> submit(const F & task, const A & ...args, std::string task_name=std::string()) {
            std::shared_ptr<std::promise<R>> task_promise(new std::promise<R>);
            std::future<R> future = task_promise->get_future();
            push_task([task, args..., task_promise, task_name] {
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
            for (ui32 i = 0; i < thread_count; i++) {
                threads[i].join();
            }
        }

        std::atomic<bool> paused = false;

        ui32 sleep_duration = 1000;

    private:
        void create_threads() {
            for (ui32 i = 0; i < thread_count; ++i) {
                threads[i] = std::thread(&ThreadPool::worker, this);
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
                    assert(task.second != "");
                    stream.println(PTPLib::Color::FG_BrightRed, "task id : ",task.second);
                    task.first();
                    tasks_total--;
                }
                else {
                    sleep_or_yield();
                }
            }
        }
    };
}
#endif // PTPLIB_ThreadPool_HPP