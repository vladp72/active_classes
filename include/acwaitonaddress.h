#ifndef _AC_HELPERS_WIN32_LIBRARY_WAIT_ON_ADDRESS_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_WAIT_ON_ADDRESS_HEADER_

#pragma once

#include <windows.h>
#include "accommon.h"

#pragma comment(lib, "Synchronization.lib")

namespace ac {

#if (_WIN32_WINNT >= 0x0600)

    class wait_on_address {
    public:
        wait_on_address() = delete;
        wait_on_address(wait_on_address &) = delete;
        wait_on_address(wait_on_address &&) = delete;
        wait_on_address &operator=(wait_on_address &) = delete;
        wait_on_address &operator=(wait_on_address &&) = delete;

        template<typename T>
        static [[nodiscard]] bool try_wait(T const volatile *address, T undesired_value, DWORD milliseconds = INFINITE) noexcept {
            static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ||
                              sizeof(T) == 8,
                          "Only 1, 2 4 or 8 bytes values are supported");

            static_assert(std::is_trivially_copyable<T>::value, "Only POD types are supported");

            return WaitOnAddress(
                       const_cast<T volatile *>(address), &undesired_value, sizeof(T), milliseconds)
                       ? true
                       : false;
        }

        template<typename T>
        static void wait(T const volatile *address, T undesired_value, DWORD milliseconds = INFINITE) {
            if (!try_wait(address, undesired_value, milliseconds)) {
                AC_THROW(GetLastError(), "WaitOnAddress");
            }
        }

        template<typename T>
        static void wake_single(T const volatile *address) noexcept {
            WakeByAddressSingle(reinterpret_cast<void *>(const_cast<T *>(address)));
        }

        template<typename T>
        static void wake_all(T const volatile *address) noexcept {
            WakeByAddressAll(reinterpret_cast<void *>(const_cast<T *>(address)));
        }
    };

#endif //(_WIN32_WINNT >= 0x0600)

} // namespace ac

#endif //_AC_HELPERS_WIN32_LIBRARY_WAIT_ON_ADDRESS_HEADER_
