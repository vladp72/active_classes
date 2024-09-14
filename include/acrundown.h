#ifndef _AC_HELPERS_WIN32_LIBRARY_RUNDOWN_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_RUNDOWN_HEADER_

#pragma once

#include "accommon.h"
#include "acwaitonaddress.h"
#include "ackernelobject.h"
#include "acresourceowner.h"

namespace ac {

    class rundown_exception: public std::system_error {
    public:
        rundown_exception()
            : std::system_error(std::error_code{static_cast<int>(ERROR_INVALID_STATE),
                                                std::system_category()},
                                "Rundown is running down") {
        }
    };

    class counter_overflow_exception: public std::system_error {
    public:
        counter_overflow_exception()
            : std::system_error(std::error_code{static_cast<int>(ERROR_ARITHMETIC_OVERFLOW),
                                                std::system_category()},
                                "Rundown counter value overrun") {
        }
    };

    namespace details {

        struct noop_rundown_base {
            bool try_start(bool restart) {
                restart;
                return true;
            }

            void has_work() {
            }

            void no_work() {
            }

            void rundown_complete() {
            }
        };

        template<typename T>
        struct crtp_rundown_base {
            bool try_start(bool restart) {
                return static_cast<T *>(this)->try_start_impl(restart);
            }

            void has_work() {
                static_cast<T *>(this)->has_work_impl();
            }

            void no_work() {
                static_cast<T *>(this)->no_work_impl();
            }

            void rundown_complete() {
                static_cast<T *>(this)->rundown_complete_impl();
            }
        };

        template<typename T>
        struct rundown_container {
            explicit rundown_container(T *container)
                : container_(container) {
            }

            bool try_start(bool restart) {
                return container_->try_start_impl(restart);
            }

            void has_work() {
                container_->has_work_impl();
            }

            void no_work() {
                container_->no_work_impl();
            }

            void rundown_complete() {
                container_->rundown_complete_impl();
            }

        private:
            T *container_;
        };

        template<typename T>
        struct master_rundown {
            explicit master_rundown(T *master)
                : master_(master) {
            }

            bool try_start(bool restart) {
                master_acquired_ = master_->try_acquire();
                return master_acquired_;
            }

            void has_work() {
            }

            void no_work() {
            }

            void rundown_complete() {
                if (master_acquired_) {
                    master_->release();
                }
            }

        private:
            T *master_{nullptr};
            bool master_acquired_{false};
        };
    } // namespace details

    template<typename T = details::noop_rundown_base>
    class rundown_counter: public T {
    protected:
        using base_t = T;

    public:
#ifdef _WIN64
        using counter_t = uint64_t;
        static counter_t const COUNTER_MAX_VALUE = 0x7FFFFFFFFFFFFFFFLL;
#else
        using counter_t = uint32_t;
        static counter_t const COUNTER_MAX_VALUE = 0x7FFFFFFFL;
#endif

    protected:
        using atomic_counter_t = std::atomic<counter_t>;

        static constexpr counter_t COUNTER_VALUE_BITS = COUNTER_MAX_VALUE;
        static constexpr counter_t INIT_VALUE = 0;
        static constexpr counter_t INCR = 1;
        static constexpr counter_t CANCEL_BIT = ~COUNTER_MAX_VALUE;

        constexpr static bool is_canceled(counter_t value) {
            return (value & CANCEL_BIT);
        }

        constexpr static counter_t decoded_count(counter_t value) {
            return (value & COUNTER_VALUE_BITS);
        }

        constexpr static bool is_idle(counter_t value) {
            return 0 == decoded_count(value);
        }

        counter_t counter_value(std::memory_order const order = std::memory_order_relaxed) const {
            return counter_.load(order);
        }

    public:
        rundown_counter(rundown_counter const &) = delete;
        rundown_counter &operator=(rundown_counter const &) = delete;
        rundown_counter(rundown_counter &&) = delete;
        rundown_counter &operator=(rundown_counter &&) = delete;

        template<typename... T>
        explicit rundown_counter(T &&...Args) {
            AC_CODDING_ERROR_IF_NOT(this->try_start(false, std::forward<T>(Args)...));
        }

        ~rundown_counter() {
            AC_CODDING_ERROR_IF_NOT(is_rundown_complete());
        }

        //
        // Set cancelation bit as release to make sure any modifications
        // that this thread did to the object before starting rundown
        // are visible by the new threads that perform acquire.
        //
        // If we see there are no active thread then rundown is complete.
        // Do acquire barrier to make sure we see any changes done by
        // last threads that released rundown, and thread that started
        // rundown last time.
        //
        bool start_rundown() {
            counter_t value = counter_.fetch_or(CANCEL_BIT, std::memory_order_release);
            bool idle = is_idle(value);
            if (idle) {
                std::atomic_thread_fence(std::memory_order_acquire);
            }
            return idle;
        }

        //
        // use memory_order_release to establish happen
        // before with any changes sequenced before restart
        // it will synchronise with memory_order_release in
        // acquire.
        // Controlling thread can make modification to the object
        // that is protected by rundown (1), then open the rundown with
        // memory_order_release.
        // Threads that do acquire with memory_rder_acquire will see
        // all changes done in (1)
        //
        template<typename... T>
        std::pair<bool, bool> restart(T &&...Args) {
            bool result = this->try_start(true, std::forward<T>(Args)...);
            if (result) {
                counter_t value = counter_.exchange(INIT_VALUE, std::memory_order_release);
                AC_CODDING_ERROR_IF_NOT(is_canceled(value) && is_idle(value));
            }
            return std::make_pair(result, false /*is_idle*/);
        }

        explicit operator bool() const {
            return is_running();
        }
        //
        // when used as a cancelation flag then if caller
        // need to make sure he observes any modification done
        // before start rundown he should explicitely require
        // memory_order_acquire or insert acquire barrier when this
        // method returns false.
        //
        bool is_running(std::memory_order order = std::memory_order_relaxed) const {
            return !is_canceled(counter_.load(order));
        }
        //
        // when used as a cancelation flag then if caller
        // need to make sure he observes any modification done
        // before start rundown he should explicitely require
        // memory_order_acquire or insert acquire barrier when this
        // method returns false.
        //
        bool is_running_down(std::memory_order order = std::memory_order_relaxed) const {
            counter_t value = counter_.load(order);
            return (is_canceled(value) && !is_idle(value));
        }
        //
        // Only for logging
        //
        bool is_rundown_complete(std::memory_order order = std::memory_order_relaxed) const {
            counter_t value = counter_.load(order);
            return (is_canceled(value) && is_idle(value));
        }
        //
        // Only for logging
        //
        counter_t count(std::memory_order const order = std::memory_order_relaxed) const {
            counter_t value = counter_.load(order);
            return decoded_count(value);
        }
        //
        // Uses memory order acquire to make sure we will see any changes
        // done by the thread that reset rundown.
        //
        bool try_acquire(counter_t max_count = COUNTER_MAX_VALUE) {
            bool aborted = false;
            bool acquired = false;
            //
            // Since we are loading current counter value relaxed
            // we might have a stale value. If it is below max then
            // we will try to run iteration assuming it is correct,
            // but if it is above max then we will assign some wrong
            // value (INCR would do just fine), retry CAS and let
            // loop figure out what to do next.
            //
            counter_t old_value = decoded_count(counter_.load(std::memory_order_relaxed));
            counter_t new_value;
            if (old_value < max_count) {
                new_value = old_value + INCR;
            } else {
                new_value = INCR;
            }
            while (!counter_.compare_exchange_weak(
                old_value, new_value, std::memory_order_acquire, std::memory_order_relaxed)) {
                if (is_canceled(old_value)) {
                    aborted = true;
                    break;
                }
                if (old_value >= max_count) {
                    aborted = true;
                    break;
                }
                new_value = old_value + INCR;
            }
            if (!aborted) {
                acquired = true;
                if (new_value == INCR) {
                    this->has_work();
                }
            }
            return acquired;
        }
        //
        // Uses memory order acquire to make sure we will see any changes
        // done by the thread that reset rundown.
        //
        void acquire(counter_t max_count = COUNTER_MAX_VALUE) {
            //
            // Since we are loading current counter value relaxed
            // we might have a stale value. If it is below max then
            // we will try to run iteration assuming it is correct,
            // but if it is above max then we will assign some wrong
            // value (INCR would do just fine), retry CAS and let
            // loop figure out what to do next.
            //
            counter_t old_value = decoded_count(counter_.load(std::memory_order_relaxed));
            counter_t new_value;
            if (old_value < max_count) {
                new_value = old_value + INCR;
            } else {
                new_value = INCR;
            }
            while (!counter_.compare_exchange_weak(
                old_value, new_value, std::memory_order_acquire, std::memory_order_relaxed)) {
                if (is_canceled(old_value)) {
                    throw rundown_exception();
                }
                if (old_value >= max_count) {
                    throw counter_overflow_exception();
                }
                new_value = old_value + INCR;
            }
            if (new_value == INCR) {
                this->has_work();
            }
        }
        //
        // Use relaxed to decrement counter, and only if we
        // are running down do acquire fence so method that
        // is handling rundown complete is synchronised with
        // the thread that started rundown.
        //
        void release() {
            counter_t old_value = counter_.fetch_sub(INCR, std::memory_order_relaxed);
            counter_t decoded_value = decoded_count(old_value);
            bool canceled = is_canceled(old_value);
            if (1 == decoded_value) {
                this->no_work();
                if (canceled) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    this->rundown_complete();
                }
            }
        }

    protected:
        atomic_counter_t counter_{INIT_VALUE};
    };

    class rundown: public rundown_counter<ac::details::crtp_rundown_base<rundown>> {
        using base_t = rundown_counter<ac::details::crtp_rundown_base<rundown>>;

    public:
        rundown() {
        }

        ~rundown() {
            join();
        }

        bool start_rundown() {
            bool just_stopped = base_t::start_rundown();
            if (just_stopped) {
                e_.set();
            }
            return just_stopped;
        }

        void restart() {
            e_.reset();
            bool result = false;
            bool is_idle = false;
            std::tie(result, is_idle) = base_t::restart();
            if (!result) {
                e_.set();
            }
        }

        void join() {
            start_rundown();
            AC_CODDING_ERROR_IF_NOT(ERROR_SUCCESS == e_.wait());
        }

        bool try_start_impl(bool restart) {
            if (restart) {
                e_.reset();
            }
            return true;
        }

        void has_work_impl() {
        }

        void no_work_impl() {
        }

        void rundown_complete_impl() {
            e_.set();
        }

    private:
        ac::event e_{event::manuel, event::unsignaled};
    };

    class slim_rundown
        : public rundown_counter<ac::details::crtp_rundown_base<slim_rundown>> {
        using base_t = rundown_counter<ac::details::crtp_rundown_base<slim_rundown>>;

    public:
        slim_rundown() {
        }

        ~slim_rundown() {
            join();
        }

        bool start_rundown() {
            bool just_stopped = base_t::start_rundown();
            return just_stopped;
        }

        void restart() {
            bool result = false;
            bool is_idle = false;
            std::tie(result, is_idle) = base_t::restart();
            AC_CODDING_ERROR_IF_NOT(result);
        }

        void join() {
            if (!start_rundown()) {
                counter_t const volatile *const volatile counter =
                    reinterpret_cast<counter_t *>(&counter_);
                for (;;) {
                    counter_t current_value = counter_value(std::memory_order_acquire);
                    AC_CODDING_ERROR_IF_NOT(is_canceled(current_value));
                    if (is_idle(current_value)) {
                        break;
                    }
                    wait_on_address::wait(counter, current_value);
                }
            }
        }

        bool try_start_impl(bool restart) {
            restart;
            return true;
        }

        void has_work_impl() {
        }

        void no_work_impl() {
        }

        void rundown_complete_impl() {
            counter_t const volatile *const volatile counter =
                reinterpret_cast<counter_t *>(&counter_);
            wait_on_address::wake_all(counter);
        }
    };

    template< typename T>
    class join_guard {
    public:

        explicit join_guard(T *rundown) noexcept 
        : rundown_(rundown) {
        }

        join_guard(join_guard const &) = delete;
        join_guard &operator=(join_guard const &) = delete;

        join_guard(join_guard &&other) noexcept 
        : rundown_(other.rundown_) {
            other.rundown_ = nullptr;
        }

        join_guard &operator=(join_guard &&other) noexcept {
            if (&other != this) {
                join();
                rundown_ = other.rundown_;
                other.rundown_ = nullptr;
            }
            return *this;
        }

        ~join_guard() noexcept {
            join();
        }

        [[nodiscard]] bool is_armed() const noexcept {
            return rundown_ != nullptr;
        }

        operator bool() const noexcept {
            return is_armed();
        }

        void join() noexcept {
            if (rundown_) {
                rundown_->join();
                rundown_ = nullptr;
            }
        }

        void disarm() {
            rundown_ = nullptr;
        }

        void attach(T *rundown) noexcept {
            if (rundown_ != rundown) {
                join();
                rundown_ = rundown;
            }
        }

        [[nodiscard]] T *detach() noexcept {
            T *rundown{rundown_};
            rundown_ = nullptr;
            return rundown;
        }

    private:
        T *rundown_;
    };

    using rundown_lock = resource_owner<rundown>;
    using rundown_join = join_guard<rundown>;

    using slim_rundown_lock = resource_owner<slim_rundown>;
    using slim_rundown_join = join_guard<slim_rundown>;


} // namespace ac

#endif //_AC_HELPERS_WIN32_LIBRARY_RUNDOWN_HEADER_
