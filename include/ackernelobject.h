#ifndef _AC_HELPERS_WIN32_LIBRARY_KERNEL_OBJECT_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_KERNEL_OBJECT_HEADER_

#include "accommon.h"

namespace ac {

    inline [[nodiscard]] DWORD wait_single_object(HANDLE h, DWORD milliseconds = INFINITE) {
        AC_CODDING_ERROR_IF(NULL == h);
        DWORD rc = WaitForSingleObject(h, milliseconds);
        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    inline [[nodiscard]] DWORD wait_single_object_ex(HANDLE h,
                                                     DWORD milliseconds = INFINITE,
                                                     bool alertable = true) {
        AC_CODDING_ERROR_IF(NULL == h);
        DWORD rc = WaitForSingleObjectEx(h, milliseconds, WINBOOL(alertable));
        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    inline [[nodiscard]] void verify_handles(HANDLE const *h, DWORD handles_count) noexcept {
        HANDLE const *cur = h;
        HANDLE const *end = h + handles_count;
        for (; cur != end; ++cur) {
            AC_CODDING_ERROR_IF(nullptr == *cur);
        }
    }

    template<DWORD N>
    inline [[nodiscard]] void verify_handles(HANDLE (&h)[N]) noexcept {
        verify_handles(h, N);
    }

    inline [[nodiscard]] size_t wait_result_to_idx(size_t wait_result) {
#ifdef max
#define AC_SAVE_MAX max
#undef max
        size_t idx = std::numeric_limits<size_t>::max();
#define max AC_SAVE_MAX
#undef AC_SAVE_MAX
#endif // max

        if (wait_result != WAIT_TIMEOUT && wait_result != WAIT_FAILED) {
            if (wait_result >= WAIT_ABANDONED_0) {
                idx = wait_result - WAIT_ABANDONED_0;
            } else {
                idx = wait_result - WAIT_OBJECT_0;
            }
        }
        return idx;
    }

    inline [[nodiscard]] DWORD wait_multiple_objects(HANDLE const *h,
                                                     DWORD handles_count,
                                                     DWORD milliseconds = INFINITE,
                                                     bool wait_all = false) {
        verify_handles(h, handles_count);

        DWORD rc = WaitForMultipleObjects(handles_count, h, WINBOOL(wait_all), milliseconds);

        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    template<DWORD N>
    inline [[nodiscard]] DWORD wait_multiple_objects(HANDLE (&h)[N],
                                                     DWORD milliseconds = INFINITE,
                                                     bool wait_all = false) {
        return wait_multiple_objects(h, N, milliseconds, wait_all);
    }

    inline [[nodiscard]] DWORD wait_multiple_objects_ex(HANDLE const *h,
                                                        DWORD handles_count,
                                                        DWORD milliseconds = INFINITE,
                                                        bool wait_all = false,
                                                        bool alertable = true) {
        verify_handles(h, handles_count);

        DWORD rc = WaitForMultipleObjectsEx(
            handles_count, h, WINBOOL(wait_all), milliseconds, WINBOOL(alertable));
        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    template<DWORD N>
    inline [[nodiscard]] DWORD wait_multiple_objects_ex(HANDLE (&h)[N],
                                                        DWORD milliseconds = INFINITE,
                                                        bool wait_all = false,
                                                        bool alertable = true) {
        return wait_multiple_objects_ex(h, N, milliseconds, wait_all, alertable);
    }

    inline [[nodiscard]] DWORD signale_and_wait(HANDLE signal_handle,
                                                HANDLE wait_handle,
                                                DWORD milliseconds = INFINITE,
                                                bool alertable = true) {
        AC_CODDING_ERROR_IF(NULL == signal_handle);
        AC_CODDING_ERROR_IF(NULL == wait_handle);
        DWORD rc = 0;
        rc = SignalObjectAndWait(
            signal_handle, wait_handle, milliseconds, WINBOOL(alertable));
        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    inline [[nodiscard]] DWORD msg_wait_multiple_objects(HANDLE const *h,
                                                         DWORD handles_count,
                                                         DWORD milliseconds = INFINITE,
                                                         bool wait_all = false,
                                                         DWORD wake_mask = QS_ALLINPUT) {
        verify_handles(h, handles_count);

        DWORD rc = MsgWaitForMultipleObjects(
            handles_count, h, WINBOOL(wait_all), milliseconds, wake_mask);
        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    template<DWORD N>
    inline [[nodiscard]] DWORD msg_wait_multiple_objects(HANDLE (&h)[N],
                                                         DWORD milliseconds = INFINITE,
                                                         bool wait_all = false,
                                                         DWORD wake_mask = QS_ALLINPUT) {
        return msg_wait_multiple_objects(h, N, milliseconds, wait_all, wake_mask);
    }

    inline [[nodiscard]] DWORD msg_wait_multiple_objects_ex(HANDLE const *h,
                                                            DWORD handles_count,
                                                            DWORD milliseconds = INFINITE,
                                                            DWORD wake_mask = QS_ALLINPUT,
                                                            DWORD flags = MWMO_ALERTABLE) {
        verify_handles(h, handles_count);

        DWORD rc = MsgWaitForMultipleObjectsEx(
            handles_count, h, milliseconds, wake_mask, flags);

        AC_CODDING_ERROR_IF(WAIT_FAILED == rc);
        return rc;
    }

    template<DWORD N>
    inline [[nodiscard]] DWORD msg_wait_multiple_objects_ex(HANDLE (&h)[N],
                                                            DWORD milliseconds = INFINITE,
                                                            DWORD wake_mask = QS_ALLINPUT,
                                                            DWORD flags = MWMO_ALERTABLE) {
        return msg_wait_multiple_objects_ex(h, N, milliseconds, wake_mask, flags);
    }

    class kernel_object {
    public:
        kernel_object() noexcept
            : h_{nullptr} {
        }

        explicit kernel_object(HANDLE h,
                               bool duplicate_handle = false,
                               DWORD options = DUPLICATE_SAME_ACCESS,
                               DWORD desired_access = 0,
                               HANDLE source_process = GetCurrentProcess(),
                               HANDLE destination_process = GetCurrentProcess(),
                               bool inheritance = false)
            : h_(nullptr) {
            if (duplicate_handle) {
                if (h != nullptr) {
                    duplicate(h, options, desired_access, source_process, destination_process, inheritance);
                }
            } else {
                attach(h);
            }
        }

        kernel_object(kernel_object const &ko,
                      DWORD options = DUPLICATE_SAME_ACCESS,
                      DWORD desired_access = 0,
                      HANDLE source_process = GetCurrentProcess(),
                      HANDLE destination_process = GetCurrentProcess(),
                      bool inheritance = false)
            : h_(nullptr) {
            if (ko.is_valid()) {
                duplicate(ko.h_, options, desired_access, source_process, destination_process, inheritance);
            }
        }

        kernel_object(kernel_object &&ko) noexcept
            : h_(ko.h_) {
            ko.h_ = nullptr;
        }

        virtual ~kernel_object() noexcept {
            no_fail_close();
        }

        kernel_object &operator=(kernel_object &&ko) noexcept {
            if (this != &ko) {
                no_fail_close();
                h_ = ko.h_;
                ko.h_ = nullptr;
            }
            return *this;
        }

        void close() {
            if (is_valid()) {
                if (!CloseHandle(h_)) {
                    //
                    // Crash application ASAP if someone has closed our handle
                    // Closing a random handle is a very dangerous bug.
                    // Consider a case when someone is closing a handle
                    // to a synchronization object. That would drive program
                    // to became undeterministic and the best strategy is to
                    // fail fast, produce a crash dump and analyze what is
                    // wrong and why are we getting into this situation.
                    //
                    AC_CODDING_ERROR_IF(ERROR_INVALID_HANDLE == GetLastError());
                    //
                    // Just throw an exception if this is any other type
                    // of error
                    //
                    AC_THROW(GetLastError(), "CloseHandle");
                }

                h_ = nullptr;
            }
        }

        [[nodiscard]] bool is_valid() const noexcept {
            return h_ != nullptr;
        }

        explicit operator bool() const noexcept {
            return is_valid();
        }

        [[nodiscard]] HANDLE const get_handle() const noexcept {
            return h_;
        }

        [[nodiscard]] HANDLE get_handle() noexcept {
            return h_;
        }

        void attach(HANDLE h) {
            close();
            h_ = h;
        }

        [[nodiscard]] HANDLE detach() noexcept {
            HANDLE th = h_;
            h_ = nullptr;
            return th;
        }

        void duplicate(HANDLE h,
                       DWORD options = DUPLICATE_SAME_ACCESS,
                       DWORD desired_access = 0,
                       HANDLE source_process = GetCurrentProcess(),
                       HANDLE destination_process = GetCurrentProcess(),
                       bool inheritance = false) {
            close();
            if (!DuplicateHandle(
                    source_process, h, destination_process, &h_, desired_access, inheritance, options)) {
                DWORD err = GetLastError();
                AC_CODDING_ERROR_IF(ERROR_INVALID_HANDLE == err);
                AC_THROW(err, "DuplicateHandle");
            }
        }

        void duplicate(kernel_object const &ko,
                       DWORD options = DUPLICATE_SAME_ACCESS,
                       DWORD desired_access = 0,
                       HANDLE source_process = GetCurrentProcess(),
                       HANDLE destination_process = GetCurrentProcess(),
                       bool inheritance = false) {
            duplicate(ko.h_, options, desired_access, source_process, destination_process, inheritance);
        }

        [[nodiscard]] DWORD wait(DWORD milliseconds = INFINITE) const noexcept {
            return wait_single_object(h_, milliseconds);
        }

        [[nodiscard]] DWORD wait_ex(DWORD milliseconds = INFINITE,
                                    bool alertable = true) const noexcept {
            return wait_single_object_ex(h_, milliseconds, alertable);
        }

        DWORD get_info() const {
            DWORD info = 0;
            if (!GetHandleInformation(h_, &info)) {
                AC_THROW(GetLastError(), "GetHandleInformation");
            }
            return info;
        }

        void set_info(DWORD mask, DWORD flag) {
            if (!SetHandleInformation(h_, mask, flag)) {
                AC_THROW(GetLastError(), "SetHandleInformation");
            }
        }

        kernel_object &operator=(kernel_object const &ko) {
            if (&ko != this) {
                close();
                if (ko.is_valid()) {
                    duplicate(ko);
                }
            }
            return *this;
        }

        void swap(kernel_object &other) noexcept {
            HANDLE h = other.h_;
            other.h_ = h_;
            h_ = h;
        }

        bool is_same_object(HANDLE h) {
            return CompareObjectHandles(h_, h);
        }

        bool is_same_object(kernel_object const &rhs) {
            return CompareObjectHandles(h_, rhs.h_);
        }

    private:


        void no_fail_close() {
            if (is_valid()) {
                if (!CloseHandle(h_)) {
                    //
                    // Crash application ASAP if someone has closed our handle
                    // Closing a random handle is a very dangerous bug.
                    // Consider a case when someone is closing a handle
                    // to a synchronization object. That would drive program
                    // to became undeterministic and the best strategy is to
                    // fail fast, produce a crash dump and analyze what is
                    // wrong and why are we getting into this situation.
                    //
                    AC_CODDING_ERROR_IF_NOT(ERROR_SUCCESS == GetLastError());
                }

                h_ = nullptr;
            }
        }

        HANDLE h_;
    };

    inline void swap(kernel_object &lhs, kernel_object &rhs) noexcept {
        lhs.swap(rhs);
    }

    inline bool operator==(kernel_object const &lhs, kernel_object const &rhs) noexcept {
        return lhs.get_handle() == rhs.get_handle();
    }

    inline bool operator!=(kernel_object const &lhs, kernel_object const &rhs) noexcept {
        return lhs.get_handle() != rhs.get_handle();
    }

    inline bool operator<(kernel_object const &lhs, kernel_object const &rhs) noexcept {
        return lhs.get_handle() < rhs.get_handle();
    }

    inline bool operator<=(kernel_object const &lhs, kernel_object const &rhs) noexcept {
        return lhs.get_handle() <= rhs.get_handle();
    }

    inline bool operator>(kernel_object const &lhs, kernel_object const &rhs) noexcept {
        return lhs.get_handle() > rhs.get_handle();
    }

    inline bool operator>=(kernel_object const &lhs, kernel_object const &rhs) noexcept {
        return lhs.get_handle() >= rhs.get_handle();
    }

    // Encapsulates operations on event kernel object
    class event: public kernel_object {
    public:

        enum event_type_t : bool { manuel = true, automatic = false};
        enum event_state_t : bool { signaled = true, unsignaled = false };

        event() {
        }

        // Creates event. Last parameter indicates if object with such a name
        // already exists
        explicit event(event_type_t event_type,
                       event_state_t event_state = unsignaled,
                       LPCTSTR name = nullptr,
                       LPSECURITY_ATTRIBUTES security_attributes = nullptr,
                       bool *exists = nullptr) {
            bool exists_tmp = !create(event_type, event_state, name, security_attributes);
            if (exists)
                *exists = exists_tmp;
        }

        // Opens existing event
        explicit event(LPCTSTR name, DWORD desired_access = EVENT_MODIFY_STATE, bool inherit = false) {
            open(name, desired_access, inherit);
        }

        // Duplicating or taking ownership
        explicit event(HANDLE h,
                       bool duplicate_handle = false,
                       DWORD options = DUPLICATE_SAME_ACCESS,
                       DWORD desired_access = 0,
                       HANDLE source_process = GetCurrentProcess(),
                       HANDLE destination_process = GetCurrentProcess(),
                       bool inheritance = false)
            : kernel_object(h, duplicate_handle, options, desired_access, source_process, destination_process, inheritance) {
        }

        event(event const &other)
            : kernel_object(other) {
        }

        event(event &&other) noexcept 
            : kernel_object(std::move(other)) {
        }

        event &operator=(event const &other) {
            kernel_object::operator=(other);
            return *this;
        }

        event &operator=(event &&other) noexcept {
            kernel_object::operator=(std::move(other));
            return *this;
        }

        // Creates event. Function returns false if event with such a name
        // already exists.
        bool create(event_type_t event_type = manuel,
                    event_state_t event_state = unsignaled,
                    LPCTSTR name = nullptr,
                    LPSECURITY_ATTRIBUTES security_attributes = nullptr) {
            close();
            HANDLE h = CreateEvent(
                security_attributes, WINBOOL(event_type), WINBOOL(event_state), name);
            DWORD error = GetLastError();
            AC_THROW_IF(h == nullptr, error, "CreateEvent");
            bool rc = (ERROR_ALREADY_EXISTS != error);
            attach(h);
            return rc;
        }

        void open(LPCTSTR name, DWORD desired_access = EVENT_MODIFY_STATE, bool inherit = false) {
            close();
            HANDLE h = OpenEvent(desired_access, WINBOOL(inherit), name);
            AC_THROW_IF(h == nullptr, GetLastError(), "OpenEvent failed");
            attach(h);
        }

        void pulse() {
            if (!PulseEvent(get_handle())) {
                AC_CRASH_APPLICATION();
            }
        }

        void reset() {
            if (!ResetEvent(get_handle())) {
                AC_CRASH_APPLICATION();
            }
        }

        void set() {
            if (!SetEvent(get_handle())) {
                AC_CRASH_APPLICATION();
            }
        }
    };

    // Encapsulates mutex kernel object. 
    class mutex : public kernel_object {
    public:
        // Creates object. Last parameter indicates if object with such a name
        // already exists
        explicit mutex( bool initial_owner = false,
                        LPCTSTR name = nullptr,
                        LPSECURITY_ATTRIBUTES security_attributes = nullptr,
                        bool *exists = nullptr) {
            bool exists_tmp = !create( initial_owner, name, security_attributes );
            if (exists) *exists = exists_tmp;
        }

        // Opens existing object
        explicit mutex( LPCTSTR name,
                        DWORD desired_access = MUTEX_MODIFY_STATE,
                        bool inherit = false ) {
            open( name, desired_access, inherit );
        }

        mutex(mutex const & other)
            : kernel_object(other) {
        }

        mutex(mutex && other) noexcept 
            : kernel_object(std::move(other)) {
        }

        mutex &operator= (mutex const & other) {
            kernel_object::operator=(other);
            return *this;
        }

        mutex &operator= (mutex && other) noexcept {
            kernel_object::operator=(std::move(other));
            return *this;
        }

        // Creates object. Function returns false if event with such a name already
        // exists.
        bool create( bool initial_owner = false,
                     LPCTSTR name = nullptr,
                     LPSECURITY_ATTRIBUTES security_attributes = nullptr) {
            close( );
            HANDLE h = CreateMutex( security_attributes, WINBOOL( initial_owner ), name );
            DWORD error = GetLastError( );
            AC_THROW_IF( h == nullptr, error, "CreateMutex" );
            bool rc = (error != ERROR_ALREADY_EXISTS);
            attach( h );
            return rc;
        }

        void open( LPCTSTR name,
                   DWORD desired_access = MUTEX_MODIFY_STATE,
                   bool inherit = false ) {
            close( );
            HANDLE h = OpenMutex( desired_access, WINBOOL( inherit ), name );
            AC_THROW_IF( h == nullptr, GetLastError( ), "OpenMutex" );
            attach( h );
        }

        void release( ) {
            if (!ReleaseMutex( get_handle( ) )) {
                AC_CRASH_APPLICATION();
            }
        }
    };

    // Encapsulates operations on semaphore kernel object. 
    class semaphore : public kernel_object {
    public:
        // Creates object. Last parameter indicates if object with such a name
        // already exists
        explicit semaphore( long initial_count,
                            long max_count,
                            LPCTSTR name = nullptr,
                            LPSECURITY_ATTRIBUTES security_attributes = nullptr,
                            bool *exists = nullptr) {
            bool exists_tmp = !create( initial_count, max_count, name, security_attributes );
            if (exists) *exists = exists_tmp;
        }

        // Opens existing object
        explicit semaphore( LPCTSTR name,
                            DWORD desired_access = SEMAPHORE_MODIFY_STATE,
                            bool inherit = false ) {
            open( name, desired_access, inherit );
        }

        // Creates object. Function returns false if event with such a name already
        // exists.
        bool create( long initial_count,
                     long max_count,
                     LPCTSTR name = nullptr,
                     LPSECURITY_ATTRIBUTES security_attributes = nullptr) {
            close( );
            HANDLE h = CreateSemaphore( security_attributes, initial_count, max_count, name );
            DWORD error = GetLastError( );
            AC_THROW_IF( h == nullptr, error, "CreateSemaphore" );
            bool rc = (error != ERROR_ALREADY_EXISTS);
            attach( h );
            return rc;
        }

        void open( LPCTSTR name,
                   DWORD desired_access = SEMAPHORE_MODIFY_STATE,
                   bool inherit = false ) {
            close( );
            HANDLE h = OpenSemaphore( desired_access, WINBOOL( inherit ), name );
            AC_THROW_IF( h == nullptr, GetLastError( ), "OpenSemaphore" );
            attach( h );
        }

        bool release( long release_count = 1, long* prev_count = nullptr) {
            long prev = 0;
            BOOL rc = ReleaseSemaphore( get_handle( ), release_count, &prev );
            if (prev_count) {
                *prev_count = prev;
            }
            if (!rc) {
                AC_CRASH_APPLICATION();
            }
            return CPPBOOL( rc );
        }
    };

} // namespace ac

#endif //_AC_HELPERS_WIN32_LIBRARY_KERNEL_OBJECT_HEADER_