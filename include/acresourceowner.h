#ifndef _AC_HELPERS_WIN32_LIBRARY_RESOURCE_OWNERL_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_RESOURCE_OWNERL_HEADER_

#pragma once

namespace ac {

    template<typename T>
    class acquire_traits final {
    public:
        [[nodiscard]] static bool try_acquire(T *v) {
            return v->try_acquire();
        }

        template<typename... P>
        [[nodiscard]] static bool try_acquire(T *v, P... param) {
            return v->try_acquire(param...);
        }

        static void acquire(T *v) {
            v->acquire();
        }

        static void release(T *v) noexcept {
            v->release();
        }
    };

    template<typename T>
    class anti_acquire_traits final {
    public:
        static void acquire(T *v) noexcept {
            v->release();
        }

        static void release(T *v) noexcept {
            v->acquire();
        }
    };

    template<typename T>
    class acquire_shared_traits final {
    public:
        static void acquire(T *v) {
            v->acquire_shared();
        }

        [[nodiscard]] static bool try_acquire(T *v) {
            return v->try_acquire_shared();
        }

        template<typename... P>
        [[nodiscard]] static bool try_acquire(T *v, P... param) {
            return v->try_acquire_shared(param...);
        }

        static void release(T *v) noexcept {
            v->release_shared();
        }
    };

    template<typename T>
    class anti_acquire_shared_traits final {
    public:
        static void acquire(T *v) noexcept {
            v->release_shared();
        }

        static void release(T *v) noexcept {
            v->acquire_shared();
        }
    };

    template<typename T>
    class acquire_exclusive_traits final {
    public:
        static void acquire(T *v) {
            v->acquire_exclusive();
        }

        [[nodiscard]] static bool try_acquire(T *v) {
            return v->try_acquire_exclusive();
        }

        template<typename... P>
        [[nodiscard]] static bool try_acquire(T *v, P... param) {
            return v->try_acquire_exclusive(param...);
        }

        static void release(T *v) noexcept {
            v->release_exclusive();
        }
    };

    template<typename T>
    class anti_acquire_exclusive_traits final {
    public:
        static void acquire(T *v) noexcept {
            v->release_exclusive();
        }

        static void release(T *v) noexcept {
            v->acquire_exclusive();
        }
    };

    //
    // A template class that uses RAII to ensure that an object will be property
    // unlocked at the end of the scope. Class is inherited from ScopedObj which
    // allows avoided specifying template parameters in some cases.
    // *acquire* family of the functions may be used to deduce template parameters
    //
    template<
        // This object must support interface expected by acquire traits (L template parameter)
        typename T,
        // Acquire traits. Maps acquire/release/try_acquire calls to object
        // specific method calls
        typename L = acquire_traits<T>>
    class resource_owner final {
    public:
        typedef L acquire_traits_t;
        typedef T resource_t;

        resource_owner(resource_owner const &owner) = delete;

        explicit resource_owner() noexcept
            : resource_(nullptr) {
        }

        resource_owner(resource_owner &&owner) noexcept
            : resource_(owner.detach()) {
        }

        resource_owner &operator=(resource_owner const &owner) = delete;

        resource_owner &operator=(resource_owner &&owner) noexcept {
            attach(owner.detach());
            return *this;
        }

        explicit resource_owner(resource_t *resource = nullptr)
            : resource_(nullptr) {
            acquire(resource);
        }

        template<typename... P>
        explicit resource_owner(resource_t *resource, P... param)
            : resource_(nullptr) {
            try_acquire(resource, param...);
        }

        void acquire(resource_t *resource) {
            if (resource != resource_) {
                release();
                if (resource) {
                    acquire_traits_t::acquire(resource);
                    resource_ = resource;
                }
            }
        }

        [[nodiscard]] bool is_valid() const noexcept {
            return resource_;
        }

        operator bool() const noexcept {
            return is_valid();
        }

        [[nodiscard]] bool try_acquire(resource_t *resource) {
            bool rc = false;
            if (resource != resource_) {
                release();
                if (resource) {
                    rc = acquire_traits_t::try_acquire(resource);
                    if (rc) {
                        resource_ = resource;
                    }
                }
            } else {
                rc = true;
            }
            return rc;
        }

        template<typename... P>
        [[nodiscard]] bool try_acquire(resource_t *resource, P... param) {
            bool rc = false;
            if (resource != resource_) {
                release();
                if (resource) {
                    rc = acquire_traits_t::try_acquire(resource, param...);
                    if (rc) {
                        resource_ = resource;
                    }
                }
            } else {
                rc = true;
            }
            return rc;
        }

        void attach(resource_t *resource) noexcept {
            if (resource != resource_) {
                release();
                if (resource) {
                    resource_ = resource;
                }
            }
        }

        [[nodiscard]] T *detach() noexcept {
            T *tmp = resource_;
            resource_ = nullptr;
            return tmp;
        }

        [[nodiscard]] T *get() noexcept {
            return resource_;
        }

        [[nodiscard]] T const *get() const noexcept {
            return resource_;
        }

        void release() noexcept {
            if (resource_) {
                acquire_traits_t::release(resource_);
                resource_ = nullptr;
            }
        }

        ~resource_owner() noexcept {
            release();
        }

    private:
        resource_t *resource_;
    };

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_traits_t> acquire_resource(T &resource) {
        return resource_owner<T, typename T::acqiure_traits_t>{&resource};
    }

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::anti_acqiure_traits_t> release_resource(T &resource) {
        return resource_owner<T, typename T::anti_acqiure_traits_t>{&resource};
    }

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_traits_t> try_acquire_resource(T &resource) {
        resource_owner<T, typename T::acqiure_traits_t> owner;
        owner.try_acquire(&resource);
        return owner;
    }

    template<typename T, typename... P>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_traits_t> try_acquire_resource(
        T &resource, P... param) {
        return resource_owner<T, typename T::acqiure_traits_t>{&resource, param...};
    }

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_shared_traits_t> acquire_resource_shared(
        T &resource) {
        return resource_owner<T, typename T::acqiure_shared_traits_t>{&resource};
    }

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_shared_traits_t> try_acquire_resource_shared(
        T &resource) {
        resource_owner<T, typename T::acqiure_shared_traits_t> owner;
        owner.try_acquire_shared(&resource);
        return owner;
    }

    template<typename T, typename... P>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_traits_t> try_acquire_resource_shared(
        T &resource, P... param) {
        return resource_owner<T, typename T::acqiure_shared_traits_t>{&resource, param...};
    }

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_exclusive_traits_t> acquire_resource_exclusive(
        T &resource) {
        return resource_owner<T, typename T::acqiure_exclusive_traits_t>{&resource};
    }

    template<typename T>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_exclusive_traits_t> try_acquire_resource_exclusive(
        T &resource) {
        resource_owner<T, typename T::acqiure_exclusive_traits_t> owner;
        owner.try_acquire_exclusive(&resource);
        return owner;
    }

    template<typename T, typename... P>
    [[nodiscard]] inline resource_owner<T, typename T::acqiure_traits_t> try_acquire_resource_exclusive(
        T &resource, P... param) {
        return resource_owner<T, typename T::acqiure_exclusive_traits_t>{&resource, param...};
    }

    class srw_lock final {
    public:
        using acqiure_shared_traits_t = acquire_shared_traits<srw_lock>;
        using acqiure_exclusive_traits_t = acquire_exclusive_traits<srw_lock>;
        using anti_acquire_shared_traits_t = anti_acquire_shared_traits<srw_lock>;
        using anti_acquire_exclusive_traits_t = anti_acquire_exclusive_traits<srw_lock>;

        using shared_lock_guard =
            resource_owner<srw_lock, srw_lock::acqiure_shared_traits_t>;
        using exclusive_lock_guard =
            resource_owner<srw_lock, srw_lock::acqiure_exclusive_traits_t>;
        using anti_shared_lock_guard =
            resource_owner<srw_lock, srw_lock::anti_acquire_shared_traits_t>;
        using anti_exclusive_lock_guard =
            resource_owner<srw_lock, srw_lock::anti_acquire_exclusive_traits_t>;

        srw_lock() noexcept {
            InitializeSRWLock(&lock_);
        }

        srw_lock(srw_lock const &) = delete;
        srw_lock(srw_lock &&) = delete;

        srw_lock &operator=(srw_lock const &) = delete;
        srw_lock &operator=(srw_lock &&) = delete;

        ~srw_lock() noexcept {
        }

        void acquire_exclusive() noexcept {
            AcquireSRWLockExclusive(&lock_);
        }

        [[nodiscard]] bool try_acquire_exclusive() noexcept {
            return TryAcquireSRWLockExclusive(&lock_) ? true : false;
        }

        void release_exclusive() noexcept {
            ReleaseSRWLockExclusive(&lock_);
        }

        void acquire_shared() noexcept {
            AcquireSRWLockShared(&lock_);
        }

        [[nodiscard]] bool try_acquire_shared() noexcept {
            return TryAcquireSRWLockShared(&lock_) ? true : false;
        }

        void release_shared() noexcept {
            ReleaseSRWLockShared(&lock_);
        }

    private:
        friend class condition_variable;

        SRWLOCK lock_;
    };

    class rw_lock final {
    public:
        using acqiure_shared_traits_t = acquire_shared_traits<rw_lock>;
        using acqiure_exclusive_traits_t = acquire_exclusive_traits<rw_lock>;
        using anti_acquire_shared_traits_t = anti_acquire_shared_traits<rw_lock>;
        using anti_acquire_exclusive_traits_t = anti_acquire_exclusive_traits<rw_lock>;

        using shared_lock_guard = resource_owner<rw_lock, rw_lock::acqiure_shared_traits_t>;
        using exclusive_lock_guard =
            resource_owner<rw_lock, rw_lock::acqiure_exclusive_traits_t>;
        using anti_shared_lock_guard =
            resource_owner<rw_lock, rw_lock::anti_acquire_shared_traits_t>;
        using anti_exclusive_lock_guard =
            resource_owner<rw_lock, rw_lock::anti_acquire_exclusive_traits_t>;

        rw_lock() noexcept
            : exclusive_owner_(0)
            , readers_count_(0) {
        }

        ~rw_lock() noexcept {
            if (0 != exclusive_owner_ || 0 != readers_count_) {
                __fastfail(1);
            }
        }

        void acquire_exclusive() noexcept {
            lock_.acquire_exclusive();
            dbg_acquire_exclusive();
        }

        [[nodiscard]] bool try_acquire_exclusive() noexcept {
            bool result = lock_.try_acquire_exclusive();
            if (result) {
                dbg_acquire_exclusive();
            }
            return result;
        }

        void release_exclusive() noexcept {
            dbg_release_exclusive();
            lock_.release_exclusive();
        }

        void acquire_shared() noexcept {
            lock_.acquire_shared();
            dbg_acquire_shared();
        }

        [[nodiscard]] bool try_acquire_shared() noexcept {
            bool result = lock_.try_acquire_shared();
            if (result) {
                dbg_acquire_shared();
            }
            return result;
        }

        void release_shared() noexcept {
            dbg_release_shared();
            lock_.release_shared();
        }

        [[nodiscard]] bool i_have_lock() const noexcept {
            return (exclusive_owner_.load(std::memory_order_acquire) == GetCurrentThreadId());
        }

    private:
        friend class condition_variable;

        void dbg_release_shared() noexcept {
            if (0 >= readers_count_.fetch_add(-1, std::memory_order_relaxed)) {
                __fastfail(1);
            }
        }

        void dbg_acquire_shared() noexcept {
            if (0 > readers_count_.fetch_add(1, std::memory_order_relaxed)) {
                __fastfail(1);
            }
        }

        void dbg_release_exclusive() noexcept {
            DWORD prev_owner = exclusive_owner_.exchange(0, std::memory_order_acq_rel);
            if (prev_owner != GetCurrentThreadId()) {
                __fastfail(1);
            }
        }

        void dbg_acquire_exclusive() noexcept {
            DWORD prev_owner = exclusive_owner_.exchange(
                GetCurrentThreadId(), std::memory_order_release);
            if (prev_owner != 0) {
                __fastfail(1);
            }
        }

        srw_lock lock_;
        //
        // Following two fields are here just to help with debugging
        //
        std::atomic<DWORD> exclusive_owner_;
        std::atomic<LONG> readers_count_;
    };
} // namespace ac

#endif //_AC_HELPERS_WIN32_LIBRARY_RESOURCE_OWNERL_HEADER_
