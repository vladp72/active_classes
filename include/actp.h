
#ifndef _AC_HELPERS_WIN32_LIBRARY_THREAD_POOL_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_THREAD_POOL_HEADER_

#pragma once

#include "accommon.h"
#include "acresourceowner.h"
#include "acrundown.h"

namespace ac::tp {

    enum class callback_runs_long : bool { no = false, yes = true };

    enum class callback_persistent : bool { no = false, yes = true };

    using nano = std::ratio<1LL, 1'000'000'000LL>;
    using nanoseconds = std::chrono::duration<long long, nano>;

    using nano100 = std::ratio<1LL, 10'000'000LL>;
    using nanoseconds100 = std::chrono::duration<long long, nano100>;

    using micro = std::ratio<1LL, 1'000'000LL>;
    using microseconds = std::chrono::duration<long long, micro>;

    using mili = std::ratio<1LL, 1'000LL>;
    using miliseconds = std::chrono::duration<long long, mili>;

    using seconds = std::chrono::duration<long long, std::ratio<1LL, 1LL>>;

    using minutes = std::chrono::duration<long long, std::ratio<60LL, 1LL>>;

    using hours = std::chrono::duration<long long, std::ratio<60LL*60LL, 1LL>>;

    using days = std::chrono::duration<long long, std::ratio<24LL*60LL*60LL, 1LL>>;

    using weeks = std::chrono::duration<long long, std::ratio<7LL * 24LL * 60LL * 60LL, 1LL>>;

    using clock = std::chrono::system_clock;
    using duration = clock::duration;
    using time_point = clock::time_point;
    using period = clock::period;

    extern __declspec(selectany) duration const infinite_duration = duration{-1};

    [[nodiscard]] inline std::chrono::duration<long long, std::ratio<1, 10000000>> operator"" _ns100(
        unsigned long long v) {
        return (std::chrono::duration<long long, std::ratio<1, 10000000>>(v));
    }

    [[nodiscard]] inline std::chrono::duration<double, std::ratio<1, 10000000>> operator"" _ns100(
        long double _Val) {
        return (std::chrono::duration<double, std::ratio<1, 10000000>>(_Val));
    }

    typedef std::shared_ptr<slim_rundown> slim_rundown_ptr;

    class thread_pool;
    class callback_instance;
    class work_item;
    class timer_work_item;
    class wait_work_item;
    class io_handler;

    using work_item_ptr = std::unique_ptr<work_item>;
    using timer_work_item_ptr = std::unique_ptr<timer_work_item>;
    using wait_work_item_ptr = std::unique_ptr<wait_work_item>;
    using io_handler_ptr = std::unique_ptr<io_handler>;

    typedef std::move_only_function<void(callback_instance &)> work_item_callback; // see help for the CreateThreadpoolWork
    typedef std::move_only_function<void(callback_instance &)> timer_work_item_callback; // see help for the CreateThreadpoolTimer
    typedef std::move_only_function<void(callback_instance &, TP_WAIT_RESULT)> wait_work_item_callback; // see help for the CreateThreadpoolWait
    typedef std::move_only_function<void(callback_instance &, OVERLAPPED *, ULONG, ULONG_PTR)> io_callback; // see help for the CreateThreadpoolIo

    struct optional_callback_parameters {
        std::optional<TP_CALLBACK_PRIORITY> priority;
        std::optional<callback_runs_long> runs_long;
        std::optional<void *> module;
    };

    namespace details {
        template<typename C>
        inline VOID CALLBACK submit_work_worker(PTP_CALLBACK_INSTANCE instance,
                                                PVOID context) noexcept {
            std::unique_ptr<C> cb{reinterpret_cast<C *>(context)};
            callback_instance inst{instance};
            (*cb)(inst);
        }
    } // namespace details

    //
    // A helper class that should not be used directly.
    // The concept of environment is incapsulated inside the cancelation
    // group.
    //
    class callback_environment final {
    public:
        callback_environment() noexcept {
            initialize();
        }

        explicit callback_environment(optional_callback_parameters const &optional_parameters) noexcept
            : callback_environment() {
            set_callback_optional_parameters(optional_parameters);
        }

        explicit callback_environment(optional_callback_parameters const *optional_parameters) noexcept
            : callback_environment() {
            if (optional_parameters) {
                set_callback_optional_parameters(optional_parameters);
            }
        }

        callback_environment(callback_environment &) = delete;
        callback_environment(callback_environment &&) = delete;
        callback_environment &operator=(callback_environment &) = delete;
        callback_environment &operator=(callback_environment &&) = delete;

        ~callback_environment() noexcept {
            destroy();
        }

        void set_callback_runs_long() noexcept {
            SetThreadpoolCallbackRunsLong(&environment_);
        }

        void set_callback_persistent() noexcept {
            SetThreadpoolCallbackPersistent(&environment_);
        }

        void set_callback_priority(TP_CALLBACK_PRIORITY priority) noexcept {
            SetThreadpoolCallbackPriority(&environment_, priority);
        }

        void set_thread_pool(PTP_POOL threadPool = nullptr) noexcept {
            SetThreadpoolCallbackPool(&environment_, threadPool);
        }

        void set_library(void *module) noexcept {
            SetThreadpoolCallbackLibrary(&environment_, module);
        }

        void set_cleanup_group(PTP_CLEANUP_GROUP cleanup_group,
                               PTP_CLEANUP_GROUP_CANCEL_CALLBACK cancel_callback) noexcept {
            SetThreadpoolCallbackCleanupGroup(&environment_, cleanup_group, cancel_callback);
        }

        void set_callback_optional_parameters(optional_callback_parameters const *params) {
            if (params) {
                set_callback_optional_parameters(*params);
            }
        }

        void set_callback_optional_parameters(optional_callback_parameters const &params) {
            if (params.runs_long == callback_runs_long::yes) {
                set_callback_runs_long();
            }
            if (params.priority) {
                set_callback_priority(params.priority.value());
            }
            if (params.module) {
                set_library(params.module.value());
            }
        }

        [[nodiscard]] PTP_CALLBACK_ENVIRON get_handle() noexcept {
            return &environment_;
        }

        [[nodiscard]] PTP_CALLBACK_ENVIRON get_handle() const noexcept {
            return const_cast<PTP_CALLBACK_ENVIRON>(&environment_);
        }

        void initialize() noexcept {
            InitializeThreadpoolEnvironment(&environment_);
        }

        void destroy() noexcept {
            DestroyThreadpoolEnvironment(&environment_);
        }

        TP_CALLBACK_ENVIRON environment_{};
    };

    //
    // This is just a helper class that is passed to each callback function and
    // provides an access to the call instance. Do not create instance of this
    // class on your own.
    //
    class callback_instance final {
    public:
        explicit callback_instance(PTP_CALLBACK_INSTANCE instance) noexcept
            : instance_{instance} {
        }

        callback_instance(callback_instance const &) = delete;
        callback_instance &operator=(callback_instance const &) = delete;
        callback_instance(callback_instance const &&) = delete;
        callback_instance operator=(callback_instance const &&) = delete;

        void set_event_on_callback_return(HANDLE event) noexcept {
            SetEventWhenCallbackReturns(instance_, event);
        }

        void release_semaphore_on_callback_return(HANDLE semaphore, DWORD release_count) noexcept {
            ReleaseSemaphoreWhenCallbackReturns(instance_, semaphore, release_count);
        }

        void leave_criticak_section_on_callback_return(PCRITICAL_SECTION critical_section) noexcept {
            LeaveCriticalSectionWhenCallbackReturns(instance_, critical_section);
        }

        void release_mutex_on_callback_return(HANDLE mutex) noexcept {
            ReleaseMutexWhenCallbackReturns(instance_, mutex);
        }

        void free_library_on_callback_return(HMODULE module) noexcept {
            FreeLibraryWhenCallbackReturns(instance_, module);
        }

        [[nodiscard]] bool may_run_long() noexcept {
            return (CallbackMayRunLong(instance_) ? true : false);
        }

    private:
        PTP_CALLBACK_INSTANCE instance_;
    };

    class work_item final {
    protected:
    public:
        template<typename C>
        static [[nodiscard]] work_item_ptr make(C &&callback,
                                                callback_environment const *environment = nullptr) {
            return std::make_unique<work_item>(std::forward<C>(callback), environment);
        }

        template<typename C>
        static [[nodiscard]] work_item_ptr make(C &&callback,
                                                optional_callback_parameters const *optional_parameters) {
            return std::make_unique<work_item>(std::forward<C>(callback), optional_parameters);
        }

        template<typename C>
        explicit work_item(C &&callback, callback_environment const *environment = nullptr)
            : callback_(std::forward<C>(callback)) {
            work_ = CreateThreadpoolWork(&work_item::run_callback,
                                         this,
                                         environment ? environment->get_handle() : nullptr);

            if (nullptr == work_) {
                AC_THROW(GetLastError(), "CreateThreadpoolWork");
            }
        }

        template<typename C>
        work_item(C &&callback, optional_callback_parameters const *optional_parameters)
            : callback_(std::forward<C>(callback)) {
            work_ = CreateThreadpoolWork(
                &work_item::run_callback,
                this,
                optional_parameters ? callback_environment{optional_parameters}.get_handle()
                                    : nullptr);

            if (nullptr == work_) {
                AC_THROW(GetLastError(), "CreateThreadpoolWork");
            }
        }

        ~work_item() noexcept {
            join();
            CloseThreadpoolWork(work_);
            work_ = nullptr;
        }

        void post() noexcept {
            SubmitThreadpoolWork(work_);
        }

        void join() noexcept {
            WaitForThreadpoolWorkCallbacks(work_, FALSE);
        }

        void cancel_and_join() noexcept {
            WaitForThreadpoolWorkCallbacks(work_, TRUE);
        }

    private:
        static void CALLBACK run_callback(PTP_CALLBACK_INSTANCE instance,
                                          void *context,
                                          PTP_WORK work) noexcept {
            work_item *work_item_raw = static_cast<work_item *>(context);
            AC_CODDING_ERROR_IF_NOT(work_item_raw->work_ == work);
            work_item_raw->run(instance);
        }

        void run(PTP_CALLBACK_INSTANCE instance) noexcept {
            callback_instance inst{instance};
            callback_(inst);
        }
        //
        // Delegate that should be called when work
        // item got executed
        //
        work_item_callback callback_;
        //
        // Pointer to the thread pool descriptor
        // of the work item
        //
        PTP_WORK work_{nullptr};
    };

    class timer_work_item final {
    public:
        template<typename C>
        static [[nodiscard]] timer_work_item_ptr make(
            C &&callback, callback_environment const *environment = nullptr) {
            return std::make_unique<timer_work_item>(std::forward<C>(callback), environment);
        }

        template<typename C>
        static [[nodiscard]] timer_work_item_ptr make(
            C &&callback, optional_callback_parameters const *optional_parameters) {
            return std::make_unique<timer_work_item>(std::forward<C>(callback),
                                                     optional_parameters);
        }

        template<typename C>
        explicit timer_work_item(C &&callback, callback_environment const *environment = nullptr)
            : callback_(std::forward<C>(callback)) {
            timer_ = CreateThreadpoolTimer(&timer_work_item::run_callback,
                                           this,
                                           environment ? environment->get_handle() : nullptr);

            if (nullptr == timer_) {
                AC_THROW(GetLastError(), "CreateThreadpoolTimer");
            }
        }

        template<typename C>
        timer_work_item(C &&callback, ac::tp::optional_callback_parameters const *optional_parameters)
            : callback_(std::forward<C>(callback)) {
            timer_ = CreateThreadpoolTimer(
                &timer_work_item::run_callback,
                this,
                optional_parameters ? callback_environment{optional_parameters}.get_handle()
                                    : nullptr);

            if (nullptr == timer_) {
                AC_THROW(GetLastError(), "CreateThreadpoolTimer");
            }
        }

        ~timer_work_item() noexcept {
            join();
            CloseThreadpoolTimer(timer_);
            timer_ = nullptr;
        }

        [[nodiscard]] bool is_set() noexcept {
            return IsThreadpoolTimerSet(timer_);
        }

        void schedule(duration const &due_time,
                      miliseconds period = miliseconds{0},
                      miliseconds window_length = miliseconds{0}) noexcept {
            ULARGE_INTEGER ul_due_time;
            FILETIME ft_due_time;
            ul_due_time.QuadPart = -due_time.count();
            ft_due_time.dwHighDateTime = ul_due_time.HighPart;
            ft_due_time.dwLowDateTime = ul_due_time.LowPart;
            SetThreadpoolTimer(timer_,
                               &ft_due_time,
                               static_cast<DWORD>(period.count()),
                               static_cast<DWORD>(window_length.count()));
        }

        void schedule(time_point const &due_time,
                      miliseconds period = miliseconds{0},
                      miliseconds window_length = miliseconds{0}) noexcept {
            FILETIME ft_due_time{system_clock_time_point_to_filetime(due_time)};
            SetThreadpoolTimer(timer_,
                               &ft_due_time,
                               static_cast<DWORD>(period.count()),
                               static_cast<DWORD>(window_length.count()));
        }

        void join() noexcept {
            SetThreadpoolTimer(timer_, nullptr, 0, 0);
            WaitForThreadpoolTimerCallbacks(timer_, FALSE);
        }

        void cancel_and_join() noexcept {
            SetThreadpoolTimer(timer_, nullptr, 0, 0);
            WaitForThreadpoolTimerCallbacks(timer_, TRUE);
        }

    private:
        static void CALLBACK run_callback(PTP_CALLBACK_INSTANCE instance,
                                          void *context,
                                          PTP_TIMER timer) noexcept {
            timer_work_item *work_item = static_cast<timer_work_item *>(context);
            AC_CODDING_ERROR_IF_NOT(work_item->timer_ == timer);
            work_item->run(instance);
        }

        void run(PTP_CALLBACK_INSTANCE instance) noexcept {
            callback_instance inst{instance};
            { callback_(inst); }
        }

        //
        // Delegate that should be called when work
        // item got executed
        //
        timer_work_item_callback callback_;
        //
        // Pointer to the thread pool descriptor
        // of the timer work item
        //
        PTP_TIMER timer_{nullptr};
    };

    class wait_work_item final {
    public:
        template<typename C>
        static [[nodiscard]] wait_work_item_ptr make(C &&callback,
                                                     callback_environment const *environment = nullptr) {
            return std::make_unique<wait_work_item>(std::forward<C>(callback), environment);
        }

        template<typename C>
        static [[nodiscard]] wait_work_item_ptr make(C &&callback,
                                                     optional_callback_parameters const *optional_parameters) {
            return std::make_unique<wait_work_item>(std::forward<C>(callback),
                                                    optional_parameters);
        }

        template<typename C>
        explicit wait_work_item(C &&callback, callback_environment const *environment = nullptr)
            : callback_(std::forward<C>(callback)) {
            wait_ = CreateThreadpoolWait(&wait_work_item::run_callback,
                                         this,
                                         environment ? environment->get_handle() : nullptr);

            if (nullptr == wait_) {
                AC_THROW(GetLastError(), "CreateThreadpoolWait");
            }
        }

        template<typename C>
        wait_work_item(C &&callback, ac::tp::optional_callback_parameters const *optional_parameters)
            : callback_(std::forward<C>(callback)) {
            wait_ = CreateThreadpoolWait(
                &wait_work_item::run_callback,
                this,
                optional_parameters ? callback_environment{optional_parameters}.get_handle()
                                    : nullptr);

            if (nullptr == wait_) {
                AC_THROW(GetLastError(), "CreateThreadpoolWait");
            }
        }

        ~wait_work_item() noexcept {
            join();
            CloseThreadpoolWait(wait_);
            wait_ = nullptr;
        }

        void schedule_wait(HANDLE handle, duration const &due_time = infinite_duration) noexcept {
            if (due_time == infinite_duration) {
                SetThreadpoolWait(wait_, handle, nullptr);
            } else {
                ULARGE_INTEGER ulDueTime;
                FILETIME ftDueTime;
                ulDueTime.QuadPart = -due_time.count();
                ftDueTime.dwHighDateTime = ulDueTime.HighPart;
                ftDueTime.dwLowDateTime = ulDueTime.LowPart;

                SetThreadpoolWait(wait_, handle, &ftDueTime);
            }
        }

        void schedule_wait(HANDLE handle, time_point const &due_time) noexcept {
            FILETIME ft_due_time{system_clock_time_point_to_filetime(due_time)};

            SetThreadpoolWait(wait_, handle, &ft_due_time);
        }

        void join() noexcept {
            WaitForThreadpoolWaitCallbacks(wait_, FALSE);
        }

        void cancel_and_join() noexcept {
            SetThreadpoolWait(wait_, nullptr, nullptr);
            WaitForThreadpoolWaitCallbacks(wait_, TRUE);
        }

    private:
        static void CALLBACK run_callback(PTP_CALLBACK_INSTANCE instance,
                                          void *context,
                                          PTP_WAIT wait,
                                          TP_WAIT_RESULT wait_result) noexcept {
            wait_work_item *work_item = static_cast<wait_work_item *>(context);
            AC_CODDING_ERROR_IF_NOT(work_item->wait_ == wait);
            work_item->run(instance, wait_result);
        }

        void run(PTP_CALLBACK_INSTANCE instance, TP_WAIT_RESULT wait_result) noexcept {
            callback_instance inst{instance};
            { callback_(inst, wait_result); }
        }

        //
        // Delegate that should be called when work
        // item got executed
        //
        wait_work_item_callback callback_;
        //
        // Pointer to the thread pool descriptor
        // of the timer work item
        //
        PTP_WAIT wait_{nullptr};
    };

    class io_guard final {
    public:
        io_guard() = default;

        explicit io_guard(io_handler *handler) noexcept;

        io_guard(io_guard const &) = delete;
        io_guard &operator=(io_guard const &) = delete;

        io_guard(io_guard &&other) noexcept
            : handler_(other.handler_) {
            other.handler_ = nullptr;
        }

        io_guard &operator=(io_guard &&other) noexcept {
            if (&other != this) {
                handler_ = other.handler_;
                other.handler_ = nullptr;
            }
            return *this;
        }

        ~io_guard() noexcept;

        void failed_start_io() noexcept;

        [[nodiscard]] bool is_armed() const noexcept {
            return nullptr != handler_;
        }

        operator bool() const noexcept {
            return is_armed();
        }

        void disarm() noexcept {
            handler_ = nullptr;
        }

    private:
        io_handler *handler_{nullptr};
    };

    class io_handler final {
        friend class io_guard;

    public:
        template<typename C>
        static [[nodiscard]] io_handler_ptr make(HANDLE handle,
                                                 C &&callback,
                                                 callback_environment const *environment = nullptr) {
            return std::make_unique<io_handler>(handle, std::forward<C>(callback), environment);
        }

        template<typename C>
        static [[nodiscard]] io_handler_ptr make(HANDLE handle,
                                                 C &&callback,
                                                 optional_callback_parameters const *optional_parameters) {
            return std::make_unique<io_handler>(
                handle, std::forward<C>(callback), optional_parameters);
        }

        template<typename C>
        explicit io_handler(HANDLE handle, C &&callback, callback_environment const *environment = nullptr)
            : callback_(std::forward<C>(callback))
            , io_(nullptr) {
            io_ = CreateThreadpoolIo(handle,
                                     &io_handler::run_callback,
                                     this,
                                     environment ? environment->get_handle() : nullptr);

            if (nullptr == io_) {
                AC_THROW(GetLastError(), "CreateThreadpoolIo");
            }
        }

        template<typename C>
        io_handler(HANDLE handle, C &&callback, ac::tp::optional_callback_parameters const *optional_parameters)
            : callback_(std::forward<C>(callback)) {
            io_ = CreateThreadpoolIo(
                handle,
                &io_handler::run_callback,
                this,
                optional_parameters ? callback_environment{optional_parameters}.get_handle()
                                    : nullptr);

            if (nullptr == io_) {
                AC_THROW(GetLastError(), "CreateThreadpoolIo");
            }
        }

        ~io_handler() noexcept {
            WaitForThreadpoolIoCallbacks(io_, FALSE);
            CloseThreadpoolIo(io_);
            io_ = nullptr;
        }

        //
        // Call this method every time before issuing an assync IO.
        //
        [[nodiscard]] io_guard start_io() noexcept {
            return io_guard{this};
        }

        //
        // According to MSDN you MUST call this method whenever
        // IO operation has completed synchronously with an error
        // code other than indicating that operation is pending.
        // Whenever possible prefer using io_guard instead of calling
        // this method directly.
        //
        void failed_start_io() noexcept {
            CancelThreadpoolIo(io_);
        }

        void join() noexcept {
            WaitForThreadpoolIoCallbacks(io_, FALSE);
        }

    private:
        void internal_start_io() noexcept {
            StartThreadpoolIo(io_);
        }

        static void CALLBACK run_callback(PTP_CALLBACK_INSTANCE instance,
                                          void *context,
                                          void *overlapped,
                                          ULONG result,
                                          ULONG_PTR bytes_transferred,
                                          PTP_IO io) noexcept {
            io_handler *handler = static_cast<io_handler *>(context);
            AC_CODDING_ERROR_IF_NOT(handler->io_ == io);
            handler->run(instance, static_cast<OVERLAPPED *>(overlapped), result, bytes_transferred);
        }

        void run(PTP_CALLBACK_INSTANCE instance,
                 OVERLAPPED *overlapped,
                 ULONG result,
                 ULONG_PTR bytes_transferred) noexcept {
            callback_instance inst{instance};
            callback_(inst, overlapped, result, bytes_transferred);
        }

        io_callback callback_;
        PTP_IO io_{nullptr};
    };

    inline io_guard::io_guard(io_handler *handler) noexcept
        : handler_(handler) {
        handler_->internal_start_io();
    }

    inline io_guard ::~io_guard() noexcept {
        if (handler_) {
            handler_->failed_start_io();
        }
    }

    inline void io_guard::failed_start_io() noexcept {
        handler_->failed_start_io();
        handler_ = nullptr;
    }

    class thread_pool final {
    public:
        explicit thread_pool(unsigned long min_threads = ULONG_MAX,
                             unsigned long max_threads = ULONG_MAX,
                             PTP_POOL_STACK_INFORMATION stack_information = nullptr)
            : pool_(nullptr) {
            pool_ = CreateThreadpool(nullptr);

            if (nullptr == pool_) {
                AC_THROW(GetLastError(), "CreateThreadPool");
            }

            if (ULONG_MAX != max_threads) {
                set_max_thread_count(max_threads);
            }

            if (ULONG_MAX != min_threads) {
                set_min_thread_count(min_threads);
            }

            if (stack_information) {
                set_stack_information(stack_information);
            }
        }

        thread_pool(thread_pool &) = delete;
        thread_pool &operator=(thread_pool &) = delete;

        ~thread_pool() noexcept {
            if (pool_) {
                CloseThreadpool(pool_);
                pool_ = nullptr;
            }
        }

        thread_pool(thread_pool &&other) noexcept
            : pool_{other.pool_} {
            other.pool_ = nullptr;
        }

        thread_pool &operator=(thread_pool &&other) noexcept {
            if (&other != this) {
                pool_ = other.pool_;
                other.pool_ = nullptr;
            }
            return *this;
        }

        [[nodiscard]] PTP_POOL get_handle() noexcept {
            return pool_;
        }

        template<typename C>
        [[nodiscard]] work_item_ptr make_work_item(
            C &&callback, optional_callback_parameters const *params = nullptr) {
            callback_environment environment;
            environment.set_thread_pool(pool_);
            environment.set_callback_optional_parameters(params);
            return work_item::make(std::forward<C>(callback), &environment);
        }

        template<typename C>
        [[nodiscard]] timer_work_item_ptr make_timer_work_item(
            C &&callback, optional_callback_parameters const *params = nullptr) {
            callback_environment environment;
            environment.set_thread_pool(pool_);
            environment.set_callback_optional_parameters(params);
            return timer_work_item::make(std::forward<C>(callback), &environment);
        }

        template<typename C>
        [[nodiscard]] wait_work_item_ptr make_wait_work_item(
            C &&callback, optional_callback_parameters const *params = nullptr) {
            callback_environment environment;
            environment.set_thread_pool(pool_);
            environment.set_callback_optional_parameters(params);
            return wait_work_item::make(std::forward<C>(callback), &environment);
        }

        template<typename C>
        [[nodiscard]] io_handler_ptr make_io_handler(
            HANDLE handle, C &&callback, optional_callback_parameters const *params = nullptr) {
            callback_environment environment;
            environment.set_thread_pool(pool_);
            environment.set_callback_optional_parameters(params);
            return io_handler::make(handle, std::forward<C>(callback), &environment);
        }

        template<typename C>
        inline void submit_work(C &&callback) {
            std::unique_ptr<C> cb{new C{std::forward<C>(callback)}};
            if (TrySubmitThreadpoolCallback(
                    &details::submit_work_worker<C>, cb.get(), nullptr)) {
                cb.release();
            } else {
                AC_THROW(GetLastError(), "TrySubmitThreadpoolCallback");
            }
            return;
        }

        template<typename C>
        void submit_work(C &&callback, optional_callback_parameters const *params) {
            std::unique_ptr<C> cb{new C{std::forward<C>(callback)}};
            callback_environment environment;
            environment.set_thread_pool(pool_);
            environment.set_callback_optional_parameters(params);
            if (TrySubmitThreadpoolCallback(
                    &details::submit_work_worker<C>, cb.get(), environment.get_handle())) {
                cb.release();
            } else {
                AC_THROW(GetLastError(), "TrySubmitThreadpoolCallback");
            }
            return;
        }

        template<typename C>
        [[nodiscard]] work_item_ptr post(C &&callback,
                                         optional_callback_parameters const *params = nullptr) {
            work_item_ptr work_item{make_work_item(std::forward<C>(callback), params)};
            work_item->post();
            return work_item;
        }

        template<typename C>
        [[nodiscard]] timer_work_item_ptr schedule(
            C &&callback,
            time_point const &due_time,
            miliseconds period = miliseconds{0},
            miliseconds window_length = miliseconds{0},
            optional_callback_parameters const *params = nullptr) {
            timer_work_item_ptr timer_work_item{
                make_timer_work_item(std::forward<C>(callback), params)};
            timer_work_item->schedule(due_time, period, window_length);
            return timer_work_item;
        }

        template<typename C>
        [[nodiscard]] timer_work_item_ptr schedule(
            C &&callback,
            duration const &due_time,
            miliseconds period = miliseconds{0},
            miliseconds window_length = miliseconds{0},
            optional_callback_parameters const *params = nullptr) {
            timer_work_item_ptr timer_work_item{
                make_timer_work_item(std::forward<C>(callback), params)};
            timer_work_item->schedule(due_time, period, window_length);
            return timer_work_item;
        }

        template<typename C>
        [[nodiscard]] wait_work_item_ptr schedule_wait(
            C &&callback,
            HANDLE handle,
            duration const &due_time = infinite_duration,
            optional_callback_parameters const *params = nullptr) {
            wait_work_item_ptr wait_work_item{
                make_wait_work_item(std::forward<C>(callback), params)};
            wait_work_item->schedule_wait(handle, due_time);
            return wait_work_item;
        }

        template<typename C>
        [[nodiscard]] wait_work_item_ptr schedule_wait(C &&callback,
                                                       HANDLE handle,
                                                       time_point const &due_time,
                                                       optional_callback_parameters const *params = nullptr) {
            wait_work_item_ptr wait_work_item{
                make_wait_work_item(std::forward<C>(callback), params)};
            wait_work_item->schedule_wait(handle, due_time);
            return wait_work_item;
        }

        void get_stack_information(PTP_POOL_STACK_INFORMATION stack_information) noexcept {
            QueryThreadpoolStackInformation(pool_, stack_information);
        }

    private:
        void set_stack_information(PTP_POOL_STACK_INFORMATION stack_information) noexcept {
            SetThreadpoolStackInformation(pool_, stack_information);
        }

        void set_max_thread_count(unsigned long max_threads) noexcept {
            SetThreadpoolThreadMaximum(pool_, max_threads);
        }

        void set_min_thread_count(unsigned long min_threads) noexcept {
            SetThreadpoolThreadMinimum(pool_, min_threads);
        }

        PTP_POOL pool_{nullptr};
    };

    template<typename C>
    inline [[nodiscard]] work_item_ptr make_work_item(
        C &&callback, optional_callback_parameters const *params = nullptr) {
        return work_item::make(std::forward<C>(callback), params);
    }

    template<typename C>
    inline [[nodiscard]] timer_work_item_ptr make_timer_work_item(
        C &&callback, optional_callback_parameters const *params = nullptr) {
        return timer_work_item::make(std::forward<C>(callback), params);
    }

    template<typename C>
    inline [[nodiscard]] wait_work_item_ptr make_wait_work_item(
        C &&callback, optional_callback_parameters const *params = nullptr) {
        return wait_work_item::make(std::forward<C>(callback), params);
    }

    template<typename C>
    inline [[nodiscard]] io_handler_ptr make_io_handler(
        HANDLE handle, C &&callback, optional_callback_parameters const *params = nullptr) {
        return io_handler::make(handle, std::forward<C>(callback), params);
    }

    template<typename C>
    inline [[nodiscard]] work_item_ptr post(C &&callback,
                                            optional_callback_parameters const *params = nullptr) {
        work_item_ptr work_item{work_item::make(std::forward<C>(callback), params)};
        work_item->post();
        return work_item;
    }

    template<typename C>
    inline void submit_work(C &&callback) {
        std::unique_ptr<C> cb{new C{std::forward<C>(callback)}};
        if (TrySubmitThreadpoolCallback(&details::submit_work_worker<C>, cb.get(), nullptr)) {
            cb.release();
        } else {
            AC_THROW(GetLastError(), "TrySubmitThreadpoolCallback");
        }
        return;
    }

    template<typename C>
    inline void submit_work(C &&callback, optional_callback_parameters const *params) {
        std::unique_ptr<C> cb{new C{std::forward<C>(callback)}};
        if (TrySubmitThreadpoolCallback(
                &details::submit_work_worker<C>,
                cb.get(),
                params ? callback_environment{params}.get_handle() : nullptr)) {
            cb.release();
        } else {
            AC_THROW(GetLastError(), "TrySubmitThreadpoolCallback");
        }
        return;
    }

    template<typename C>
    inline [[nodiscard]] timer_work_item_ptr schedule(
        C &&callback,
        time_point const &due_time,
        miliseconds period = miliseconds{0},
        miliseconds window_length = miliseconds{0},
        optional_callback_parameters const *params = nullptr) {
        timer_work_item_ptr timer_work_item{
            timer_work_item::make(std::forward<C>(callback), params)};
        timer_work_item->schedule(due_time, period, window_length);
        return timer_work_item;
    }

    template<typename C>
    inline [[nodiscard]] timer_work_item_ptr schedule(
        C &&callback,
        duration const &due_time,
        miliseconds period = miliseconds{0},
        miliseconds window_length = miliseconds{0},
        optional_callback_parameters const *params = nullptr) {
        timer_work_item_ptr timer_work_item{
            timer_work_item::make(std::forward<C>(callback), params)};
        timer_work_item->schedule(due_time, period, window_length);
        return timer_work_item;
    }

    template<typename C>
    inline [[nodiscard]] wait_work_item_ptr schedule_wait(
        C &&callback,
        HANDLE handle,
        duration const &due_time = infinite_duration,
        optional_callback_parameters const *params = nullptr) {
        wait_work_item_ptr wait_work_item{
            wait_work_item::make(std::forward<C>(callback), params)};
        wait_work_item->schedule_wait(handle, due_time);
        return wait_work_item;
    }

    template<typename C>
    inline [[nodiscard]] wait_work_item_ptr schedule_wait(
        C &&callback,
        HANDLE handle,
        time_point const &due_time,
        optional_callback_parameters const *params = nullptr) {
        wait_work_item_ptr wait_work_item{
            wait_work_item::make(std::forward<C>(callback), params)};
        wait_work_item->schedule_wait(handle, due_time);
        return wait_work_item;
    }

    template<typename T>
    class scoped_join {
    public:
        scoped_join() {
        }

        explicit scoped_join(T *p)
            : p_{p} {
        }

        scoped_join(scoped_join const &other) = delete;
        scoped_join &operator=(scoped_join const &other) = delete;

        scoped_join(scoped_join &&other) noexcept
            : p_{other.p_} {
            other.p_ = nullptr;
        }

        scoped_join &operator=(scoped_join &&other) noexcept {
            if (&other != this) {
                p_ = std::move(other.p_);
                other.p_ = nullptr;
            }
            return *this;
        }

        ~scoped_join() noexcept {
            join();
        }

        void join() noexcept {
            if (p_) {
                p_->join();
                p_ = nullptr;
            }
        }

        [[nodiscard]] bool is_armed() const noexcept {
            return p_ != nullptr;
        }

        operator bool() const noexcept {
            return is_armed();
        }

        void disarm() noexcept {
            p_ = nullptr;
        }

    private:
        T *p_{nullptr};
    };

} // namespace ac::tp

#endif //_AC_HELPERS_WIN32_LIBRARY_THREAD_POOL_HEADER_