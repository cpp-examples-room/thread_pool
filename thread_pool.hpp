#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace tds {
    class function_wrapper {
    private:
        struct impl_base {
            virtual void call() = 0;
            virtual ~impl_base() {}
        };

        template <typename F>
        struct impl_type: impl_base {
            F f_;

            impl_type(F&& f): f_(std::move(f)) {}

            void call() override {
                f_();
            }
        };

        std::unique_ptr<impl_base> impl;

    public:
        template <typename F>
        function_wrapper(F&& f):
            impl(new impl_type<F>(std::move(f))) {}

        void operator()() {
            impl->call();
        }

        function_wrapper() = delete;
        function_wrapper(function_wrapper&& rhs): impl(std::move(rhs.impl)) {}

        function_wrapper& operator=(function_wrapper&& rhs) {
            impl = std::move(rhs.impl);
            return *this;
        }

        function_wrapper(function_wrapper const&)=delete;
        function_wrapper& operator=(function_wrapper const&)=delete;
    };

    class thread_pool {
    public:
        thread_pool(unsigned int thread_count = std::thread::hardware_concurrency() + 1) {
            try {
                for(unsigned int i = 0; i < thread_count; ++i) {
                    threads_.reserve(thread_count);
                    std::thread t{&thread_pool::worker_thread, this};
                    threads_.push_back(std::move(t));
                }
            } catch (...) {
                close();
                throw;
            }
        }

        thread_pool(thread_pool const& rhs) = delete;
        thread_pool(thread_pool&& rhs) = default;
        thread_pool& operator=(thread_pool const& rhs) = delete;
        thread_pool& operator=(thread_pool&& rhs) = default;

        ~thread_pool() {
            close();
            join();
        }

        template <typename F, typename... Args>
        auto submit(F&& f, Args&&... args) {
            std::packaged_task<typename std::result_of<F(Args...)>::type()> task(
                std::bind(std::forward<F>(f), std::forward<Args...>(args)...));
            auto future = task.get_future();
            std::lock_guard<std::mutex> lock_g(mutex_);
            work_queue_.push_back(std::move(task));
            return future;
        }

    private:
        void worker_thread() {
            try {
                while(true) {
                    std::unique_lock<std::mutex> u_lock(mutex_);
                    if (closed_) { return; }
                    if (!work_queue_.empty()) {
                        function_wrapper task{std::move(work_queue_.front())};
                        work_queue_.pop_front();
                        u_lock.unlock();
                        task();

                    } else {
                        u_lock.unlock();
                        std::this_thread::yield();
                    }
                }
            } catch (...) {
                std::terminate();
            }
        }

        void join() {
            for(auto&& t: threads_) {
                t.join();
            }
        }

        void close() {
            std::lock_guard<std::mutex> lock_g(mutex_);
            closed_ = true;
        }

        bool closed() const {
            std::lock_guard<std::mutex> lock_g(mutex_);
            return closed_;
        }

        std::vector<std::thread>        threads_;
        mutable std::mutex              mutex_;
        std::deque<function_wrapper>    work_queue_; 
        bool                            closed_;
    };
}

#endif
