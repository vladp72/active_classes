#ifndef _AC_HELPERS_WIN32_LIBRARY_FILE_OBJECT_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_FILE_OBJECT_HEADER_

#pragma once

#include "accommon.h"
#include "ackernelobject.h"
#include "actp.h"

namespace ac {

    template<typename>
    struct get_file_info;
    template<>
    struct get_file_info<FILE_BASIC_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileBasicInfo;
    };
    template<>
    struct get_file_info<FILE_STANDARD_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileStandardInfo;
    };
    template<>
    struct get_file_info<FILE_NAME_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileNameInfo;
    };
    template<>
    struct get_file_info<FILE_STREAM_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileStreamInfo;
    };
    template<>
    struct get_file_info<FILE_COMPRESSION_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileCompressionInfo;
    };
    template<>
    struct get_file_info<FILE_ATTRIBUTE_TAG_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileAttributeTagInfo;
    };
    template<>
    struct get_file_info<FILE_ID_BOTH_DIR_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileIdBothDirectoryRestartInfo;
    };
    template<>
    struct get_file_info<FILE_REMOTE_PROTOCOL_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS value = FileRemoteProtocolInfo;
    };

    template<typename T>
    inline constexpr FILE_INFO_BY_HANDLE_CLASS get_file_info_v = get_file_info<T>::value;

    template<typename>
    struct set_file_info;
    template<>
    struct set_file_info<FILE_BASIC_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS const value = FileBasicInfo;
    };
    template<>
    struct set_file_info<FILE_RENAME_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS const value = FileRenameInfo;
    };
    template<>
    struct set_file_info<FILE_DISPOSITION_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS const value = FileDispositionInfo;
    };
    template<>
    struct set_file_info<FILE_ALLOCATION_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS const value = FileAllocationInfo;
    };
    template<>
    struct set_file_info<FILE_END_OF_FILE_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS const value = FileEndOfFileInfo;
    };
    template<>
    struct set_file_info<FILE_IO_PRIORITY_HINT_INFO> {
        static constexpr FILE_INFO_BY_HANDLE_CLASS const value = FileIoPriorityHintInfo;
    };

    template<typename T>
    inline constexpr FILE_INFO_BY_HANDLE_CLASS set_file_info_v = set_file_info<T>::value;

    class file_object: public kernel_object {
    public:
        [[nodiscard]] static DWORD try_create_directory(
            wchar_t const *name, LPSECURITY_ATTRIBUTES security_attributes = nullptr) noexcept {
            if (!::CreateDirectoryW(name, security_attributes)) {
                return GetLastError();
            }
            return ERROR_SUCCESS;
        }

        static void create_directory(wchar_t const *name,
                                     LPSECURITY_ATTRIBUTES security_attributes = nullptr) {
            if (!::CreateDirectoryW(name, security_attributes)) {
                AC_THROW(GetLastError(), "CreateDirectory");
            }
        }

        [[nodiscard]] static DWORD try_erase(wchar_t const *name) noexcept {
            if (!DeleteFileW(name)) {
                return GetLastError();
            }
            return ERROR_SUCCESS;
        }

        static void erase(wchar_t const *name) {
            if (!DeleteFileW(name)) {
                AC_THROW(GetLastError(), "DeleteFile");
            }
        }

        static void copy(wchar_t const *existing_name,
                         wchar_t const *new_name,
                         bool fail_if_exists = true) {
            if (!CopyFileW(existing_name, new_name, fail_if_exists)) {
                AC_THROW(GetLastError(), "CopyFile");
            }
        }

        static void copy_ex(wchar_t const *existing_name,
                            wchar_t const *new_name,
                            DWORD flags,
                            LPPROGRESS_ROUTINE progressRoutine = NULL,
                            LPVOID data = NULL,
                            LPBOOL cancel = NULL) {
            if (!CopyFileExW(existing_name, new_name, progressRoutine, data, cancel, flags)) {
                AC_THROW(GetLastError(), "CopyFileEx");
            }
        }

        static void move(wchar_t const *existing_name, wchar_t const *new_name) {
            if (!MoveFileW(existing_name, new_name)) {
                AC_THROW(GetLastError(), "MoveFile");
            }
        }

        [[nodiscard]] static DWORD try_move_ex(wchar_t const *existing_name,
                                               wchar_t const *new_name,
                                               DWORD flags) noexcept {
            DWORD err = ERROR_SUCCESS;

            if (!MoveFileExW(existing_name, new_name, flags)) {
                err = GetLastError();
            }

            return err;
        }

        static void move_ex(wchar_t const *existing_name, wchar_t const *new_name, DWORD flags) {
            if (!MoveFileExW(existing_name, new_name, flags)) {
                AC_THROW(GetLastError(), "MoveFileEx");
            }
        }

        static void set_security(wchar_t const *file_name,
                                 SECURITY_INFORMATION security_information,
                                 PSECURITY_DESCRIPTOR security_descriptor) {
            if (!SetFileSecurityW(file_name, security_information, security_descriptor)) {
                AC_THROW(GetLastError(), "SetFileSecurity");
            }
        }

        static void set_attributes(wchar_t const *file_name, DWORD attributes) {
            if (!SetFileAttributesW(file_name, attributes)) {
                AC_THROW(GetLastError(), "SetFileAttributes");
            }
        }

        [[nodiscard]] static DWORD get_attributes(wchar_t const *file_name) noexcept {
            return GetFileAttributesW(file_name);
        }

        [[nodiscard]] static __int64 get_compressed_size(wchar_t const *file_name) {
            DWORD high_part = 0;
            DWORD low_part = GetCompressedFileSizeW(file_name, &high_part);

            if (INVALID_FILE_SIZE == low_part) {
                AC_THROW(GetLastError(), "GetCompressedFileSize");
            }
            return make_qword(low_part, high_part);
        }

        static void make_hard_link(wchar_t const *file_name,
                                   wchar_t const *existing_file_name,
                                   LPSECURITY_ATTRIBUTES security_attributes = nullptr) {
            if (!CreateHardLinkW(file_name, existing_file_name, security_attributes)) {
                AC_THROW(GetLastError(), "CreateHardLink");
            }
        }

        file_object() {
        }
        //
        // Duplicates handle
        //
        file_object(file_object const &f)
            : kernel_object(f)
            , name_(f.name_) {
        }

        file_object(file_object &&f) noexcept
            : kernel_object(std::move(f))
            , name_(std::move(f.name_)) {
        }

        file_object(wchar_t const *name,
                    DWORD desiered_access,
                    DWORD share_mode,
                    DWORD creation_disposition,
                    DWORD flag_and_attributes = FILE_ATTRIBUTE_NORMAL,
                    LPSECURITY_ATTRIBUTES security_attributes = nullptr,
                    HANDLE template_file = nullptr) {
            create(name, desiered_access, share_mode, creation_disposition, flag_and_attributes, security_attributes, template_file);
        }

        const file_object &operator=(file_object const &f) {
            if (&f != this) {
                kernel_object::operator=(f);
                name_ = f.name_;
            }
            return *this;
        };

        const file_object &operator=(file_object &&f) noexcept {
            if (&f != this) {
                kernel_object::operator=(std::move(f));
                name_ = std::move(f.name_);
            }
            return *this;
        };

        [[nodiscard]] DWORD try_create(wchar_t const *name,
                                       DWORD desiered_access,
                                       DWORD share_mode,
                                       DWORD creation_disposition,
                                       DWORD flag_and_attributes = FILE_ATTRIBUTE_NORMAL,
                                       LPSECURITY_ATTRIBUTES security_attributes = nullptr,
                                       HANDLE template_file = nullptr) {
            close();

            HANDLE h = CreateFileW(name,
                                   desiered_access,
                                   share_mode,
                                   security_attributes,
                                   creation_disposition,
                                   flag_and_attributes,
                                   template_file);

            if (h == INVALID_HANDLE_VALUE) {
                return GetLastError();
            }

            attach(h);

            name_ = name;

            return ERROR_SUCCESS;
        };

        void create(wchar_t const *name,
                    DWORD desiered_access,
                    DWORD share_mode,
                    DWORD creation_disposition,
                    DWORD flag_and_attributes = FILE_ATTRIBUTE_NORMAL,
                    LPSECURITY_ATTRIBUTES security_attributes = nullptr,
                    HANDLE template_file = nullptr) {
            close();

            HANDLE h = CreateFileW(name,
                                   desiered_access,
                                   share_mode,
                                   security_attributes,
                                   creation_disposition,
                                   flag_and_attributes,
                                   template_file);

            if (h == INVALID_HANDLE_VALUE) {
                AC_THROW(GetLastError(), "CreateFile");
            }

            attach(h);

            name_ = name;
        };

        std::wstring const &get_name() const {
            return name_;
        }

        void close() {
            kernel_object::close();
            name_.clear();
        };

        void set_end_of_file() {
            if (!::SetEndOfFile(get_handle())) {
                AC_THROW(GetLastError(), "SetEndOfFile");
            }
        }

        void resize(__int64 pos) {
            ::LARGE_INTEGER oldPosition, delta;
            delta.QuadPart = pos;
            if (!::SetFilePointerEx(get_handle(), delta, &oldPosition, FILE_BEGIN)) {
                AC_THROW(GetLastError(), "SetFilePointerEx");
            }

            if (!::SetEndOfFile(get_handle())) {
                AC_THROW(GetLastError(), "SetEndOfFile");
            }
        }

        [[nodiscard]] __int64 set_file_pointer(__int64 pos, LONG method = FILE_CURRENT) {
            LONG high_pos = get_high_dword(pos);
            LONG low_pos =
                ::SetFilePointer(get_handle(), get_low_dword(pos), &high_pos, method);
            if (INVALID_SET_FILE_POINTER == low_pos && ERROR_SUCCESS != GetLastError()) {
                AC_THROW(GetLastError(), "SetFilePointer");
            }
            return make_qword(low_pos, high_pos);
        }
        //
        // Returns false if EOF was reached
        //
        bool read(LPVOID buffer, DWORD number_of_bytes_to_read, DWORD *number_of_bytes_read) {
            bool rc = true;
            if (!ReadFile(get_handle(), buffer, number_of_bytes_to_read, number_of_bytes_read, nullptr)) {
                if (ERROR_HANDLE_EOF == GetLastError()) {
                    rc = false;
                } else {
                    AC_THROW(GetLastError(), "ReadFile");
                }
            } else {
                rc = (*number_of_bytes_read != 0);
            }
            return rc;
        }
        //
        // Returns true if IO completed synchronosly and
        // false otherwise
        //
        [[nodiscard]] bool read(LPVOID buffer,
                                DWORD number_of_bytes_to_read,
                                DWORD *bytes_read,
                                bool *is_eof,
                                OVERLAPPED *o) {
            DWORD error = ERROR_SUCCESS;

            if (!ReadFile(get_handle(), buffer, number_of_bytes_to_read, bytes_read, o)) {
                error = GetLastError();

                if (ERROR_HANDLE_EOF == error) {
                    if (is_eof) {
                        *is_eof = true;
                        //
                        // The IO has completed synchronosly
                        //
                        error = ERROR_SUCCESS;
                    } else {
                        AC_THROW(error, "ReadFile");
                    }
                } else if (ERROR_IO_PENDING != error) {
                    AC_THROW(error, "ReadFile");
                }
            }
            return (ERROR_SUCCESS == error);
        }

        DWORD read_sync(LPVOID buffer, LONGLONG offset, DWORD number_of_bytes_to_read, bool *is_eof) {
            DWORD error = ERROR_SUCCESS;
            DWORD number_of_bytes_read = 0;

            event e{event::manuel, event::unsignaled};

            OVERLAPPED overlapped = {};
            overlapped.hEvent = e.get_handle();
            overlapped.Offset = get_low_dword(offset);
            overlapped.OffsetHigh = get_high_dword(offset);

            if (!read(buffer, number_of_bytes_to_read, &number_of_bytes_read, is_eof, &overlapped)) {
                if (!CPPBOOL(GetOverlappedResult(
                        get_handle(), &overlapped, &number_of_bytes_read, TRUE))) {
                    if (ERROR_HANDLE_EOF == error) {
                        *is_eof = true;
                        //
                        // The IO has completed synchronosly
                        //
                        error = ERROR_SUCCESS;
                    } else {
                        AC_THROW(error, "ReadFile");
                    }
                }
            }
            //
            // If we read less than requested then we've reached EOF
            //
            if (ERROR_SUCCESS == error) {
                AC_CODDING_ERROR_IF_NOT(number_of_bytes_to_read >= number_of_bytes_read);
                if (number_of_bytes_to_read > number_of_bytes_read) {
                    *is_eof = true;
                }
            }

            return number_of_bytes_read;
        }

        //
        // Synchronosly writes to the file.
        // Returns number of bytes written to the file
        //
        DWORD write(VOID const *buffer, DWORD number_of_bytes_to_write) {
            DWORD number_of_bytes_wrote = 0;
            if (!WriteFile(get_handle(), buffer, number_of_bytes_to_write, &number_of_bytes_wrote, nullptr)) {
                AC_THROW(GetLastError(), "WriteFile");
            }

            return number_of_bytes_wrote;
        }
        //
        // Returns true if IO completed synchronosly and
        // returns false otherwise
        //
        [[nodiscard]] bool write(VOID const *buffer,
                                 DWORD number_of_bytes_to_write,
                                 OVERLAPPED *o,
                                 DWORD *number_of_bytes_wrote = nullptr) {
            DWORD error = ERROR_SUCCESS;
            DWORD number_of_bytes_wrote_tmp = 0;

            if (!WriteFile(get_handle(), buffer, number_of_bytes_to_write, &number_of_bytes_wrote_tmp, o)) {
                error = GetLastError();

                if (o) {
                    if (ERROR_IO_PENDING != error) {
                        AC_THROW(error, "WriteFile");
                    }
                } else {
                    AC_THROW(error, "WriteFile");
                }
            }

            if (number_of_bytes_wrote) {
                *number_of_bytes_wrote = number_of_bytes_wrote_tmp;
            }

            return (ERROR_SUCCESS == error);
        }

        [[nodiscard]] DWORD write_sync(VOID const *buffer, LONGLONG offset, DWORD number_of_bytes_to_write) {
            DWORD number_of_bytes_wrote = 0;

            event e{event::manuel, event::unsignaled};

            OVERLAPPED overlapped = {};
            overlapped.hEvent = e.get_handle();
            overlapped.Offset = get_low_dword(offset);
            overlapped.OffsetHigh = get_high_dword(offset);

            if (!write(buffer, number_of_bytes_to_write, &overlapped, &number_of_bytes_wrote)) {
                if (!CPPBOOL(GetOverlappedResult(
                        get_handle(), &overlapped, &number_of_bytes_wrote, TRUE))) {
                    AC_THROW(GetLastError(), "WriteFile");
                }
            }

            return number_of_bytes_wrote;
        }
        //
        // Cancels all IOs issued by the current thread
        // on this file object
        //
        [[nodiscard]] DWORD try_cancel_all_thread_io() noexcept {
            DWORD error = ERROR_SUCCESS;
            if (!::CancelIo(get_handle())) {
                error = GetLastError();
            }
            return error;
        }

        void cancel_all_thread_io() {
            DWORD error = try_cancel_all_thread_io();
            if (ERROR_SUCCESS != error) {
                AC_THROW(error, "CancelIo");
            }
        }
        //
        // Cancels all IO issued by the current process
        // on this file object
        //
        [[nodiscard]] DWORD try_cancel_all_io() noexcept {
            DWORD error = ERROR_SUCCESS;
            if (!::CancelIoEx(get_handle(), nullptr)) {
                error = GetLastError();
            }
            return error;
        }

        void cancel_all_io() {
            DWORD error = try_cancel_all_io();
            if (ERROR_SUCCESS != error) {
                AC_THROW(error, "CancelIoEx");
            }
        }
        //
        // Cancels single IO operations issued on this file
        // object
        //
        [[nodiscard]] DWORD try_cancel_io(OVERLAPPED *o) noexcept {
            DWORD error = ERROR_SUCCESS;
            if (!::CancelIoEx(get_handle(), o)) {
                error = GetLastError();
            }
            return error;
        }

        void cancel_io(OVERLAPPED *o) {
            DWORD error = try_cancel_io(o);
            if (ERROR_SUCCESS != error) {
                AC_THROW(error, "CancelIoEx");
            }
        }

        [[nodiscard]] DWORD try_send_io_ctrl(DWORD control_code,
                                             VOID const *in_buffer,
                                             DWORD in_buffer_size,
                                             VOID *out_buffer,
                                             DWORD out_buffer_size,
                                             DWORD *bytes_returned,
                                             OVERLAPPED *o = nullptr) noexcept {
            DWORD error = ERROR_SUCCESS;

            if (!DeviceIoControl(get_handle(),
                                 control_code,
                                 const_cast<VOID *>(in_buffer),
                                 in_buffer_size,
                                 out_buffer,
                                 out_buffer_size,
                                 bytes_returned,
                                 o)) {
                error = GetLastError();
            }

            return error;
        }

        [[nodiscard]] DWORD try_send_io_ctrl(DWORD control_code,
                                             std::vector<byte> const &in_buffer,
                                             std::vector<byte> &out_buffer,
                                             DWORD *bytes_returned,
                                             OVERLAPPED *o = nullptr) noexcept {
            return try_send_io_ctrl(control_code,
                                    in_buffer.data(),
                                    static_cast<DWORD>(in_buffer.size()),
                                    out_buffer.data(),
                                    static_cast<DWORD>(out_buffer.size()),
                                    bytes_returned,
                                    o);
        }

        DWORD try_send_io_ctrl_sync(DWORD control_code,
                                    VOID const *in_buffer,
                                    DWORD in_buffer_size,
                                    VOID *out_buffer,
                                    DWORD out_buffer_size,
                                    DWORD *bytes_returned) {
            ac::event e{event::manuel, event::unsignaled};
            OVERLAPPED overlapped = {};
            overlapped.hEvent = e.get_handle();

            DWORD error = try_send_io_ctrl(
                control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, bytes_returned, &overlapped);
            if (ERROR_IO_PENDING == error) {
                if (!CPPBOOL(GetOverlappedResult(
                        get_handle(), &overlapped, bytes_returned, TRUE))) {
                    error = GetLastError();
                } else {
                    error = ERROR_SUCCESS;
                }
            }
            return error;
        }

        [[nodiscard]] DWORD try_send_io_ctrl_sync(DWORD control_code,
                                                  std::vector<byte> const &in_buffer,
                                                  std::vector<byte> &out_buffer,
                                                  DWORD *bytes_returned) {
            return try_send_io_ctrl_sync(control_code,
                                         in_buffer.data(),
                                         static_cast<DWORD>(in_buffer.size()),
                                         out_buffer.data(),
                                         static_cast<DWORD>(out_buffer.size()),
                                         bytes_returned);
        }

        [[nodiscard]] DWORD try_send_io_ctrl_sync(DWORD control_code) {
            DWORD bytes_returned;
            return try_send_io_ctrl_sync(control_code, NULL, 0, NULL, 0, &bytes_returned);
        }

        [[nodiscard]] DWORD try_send_io_ctrl(DWORD control_code) {
            DWORD bytes_returned = 0;
            return try_send_io_ctrl(
                control_code, nullptr, 0, nullptr, 0, &bytes_returned, nullptr);
        }

        [[nodiscard]] DWORD send_io_ctrl(DWORD control_code,
                                         VOID const *in_buffer,
                                         DWORD in_buffer_size,
                                         VOID *out_buffer,
                                         DWORD out_buffer_size) {
            DWORD bytes_returned = 0;

            DWORD error = try_send_io_ctrl(
                control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, &bytes_returned, nullptr);
            if (ERROR_SUCCESS != error) {
                AC_THROW(error, "DeviceIoControl");
            }

            return bytes_returned;
        }

        [[nodiscard]] DWORD send_io_ctrl_sync(DWORD control_code,
                                              VOID const *in_buffer,
                                              DWORD in_buffer_size,
                                              VOID *out_buffer,
                                              DWORD out_buffer_size) {
            DWORD bytes_returned = 0;

            DWORD error = try_send_io_ctrl_sync(
                control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, &bytes_returned);
            if (ERROR_SUCCESS != error) {
                AC_THROW(error, "DeviceIoControl");
            }

            return bytes_returned;
        }

        bool send_io_ctrl(DWORD control_code) {
            return send_io_ctrl(control_code, nullptr, 0, nullptr, 0);
        }

        //
        // Returns true if IO completed synchronosly and
        // returns false otherwise
        //
        [[nodiscard]] bool send_io_ctrl(DWORD control_code,
                                        VOID const *in_buffer,
                                        DWORD in_buffer_size,
                                        VOID *out_buffer,
                                        DWORD out_buffer_size,
                                        OVERLAPPED *o,
                                        DWORD *bytes_returned = nullptr) {
            DWORD bytes_returned_tmp = 0;

            DWORD error = try_send_io_ctrl(
                control_code, in_buffer, in_buffer_size, out_buffer, out_buffer_size, &bytes_returned_tmp, o);

            if (ERROR_SUCCESS != error) {
                if (ERROR_IO_PENDING != error) {
                    AC_THROW(error, "DeviceIoControl");
                }
            }

            if (bytes_returned) {
                *bytes_returned = bytes_returned_tmp;
            }

            return (ERROR_SUCCESS == error);
        }

        [[nodiscard]] DWORD try_get_information(FILE_INFO_BY_HANDLE_CLASS file_information_class,
                                                void *file_information,
                                                DWORD buffer_size) noexcept {
            DWORD error = ERROR_SUCCESS;

            if (!GetFileInformationByHandleEx(
                    get_handle(), file_information_class, file_information, buffer_size)) {
                error = GetLastError();
            }

            return error;
        }

        void get_information(FILE_INFO_BY_HANDLE_CLASS file_information_class,
                             void *file_information,
                             DWORD buffer_size) {
            DWORD error = try_get_information(
                file_information_class, file_information, buffer_size);

            if (error != ERROR_SUCCESS) {
                AC_THROW(error, "TryGetInformation");
            }
        }

        template<typename T>
        [[nodiscard]] DWORD try_get_information(
            T *file_information,
            FILE_INFO_BY_HANDLE_CLASS file_information_class = get_file_info_v<T>) {
            return try_get_information(
                file_information_class, file_information, sizeof(*file_information));
        }

        template<typename T>
        void get_information(T *file_information,
                             FILE_INFO_BY_HANDLE_CLASS file_information_class = get_file_info_v<T>) {
            DWORD error = try_get_information(file_information, file_information_class);

            if (error != ERROR_SUCCESS) {
                AC_THROW(error, "TryGetInformation");
            }
        }

        DWORD try_set_information(FILE_INFO_BY_HANDLE_CLASS file_information_class,
                                  void const *file_information,
                                  DWORD buffer_size) {
            DWORD error = ERROR_SUCCESS;

            if (!SetFileInformationByHandle(get_handle(),
                                            file_information_class,
                                            const_cast<void *>(file_information),
                                            buffer_size)) {
                error = GetLastError();
            }

            return error;
        }

        void set_information(FILE_INFO_BY_HANDLE_CLASS file_information_class,
                             void const *file_information,
                             DWORD buffer_size) {
            DWORD error = try_set_information(
                file_information_class, file_information, buffer_size);

            if (error != ERROR_SUCCESS) {
                AC_THROW(error, "TrySetInformation");
            }
        }

        template<typename T>
        DWORD try_set_information(T const &file_information,
                                  FILE_INFO_BY_HANDLE_CLASS file_information_class = set_file_info_v<T>) {
            return try_set_information(
                file_information_class, &file_information, sizeof(file_information));
        }

        template<typename T>
        void set_information(T const &file_information,
                             FILE_INFO_BY_HANDLE_CLASS file_information_class = set_file_info_v<T>) {
            DWORD error = try_set_information(file_information, file_information_class);

            if (error != ERROR_SUCCESS) {
                AC_THROW(error, "TrySetInformation");
            }
        }

        __int64 get_size() const {
            DWORD file_size_high = 0L;
            DWORD file_size = GetFileSize(get_handle(), &file_size_high);
            if (INVALID_SET_FILE_POINTER == file_size && NO_ERROR != GetLastError()) {
                AC_THROW(GetLastError(), "GetFileSize");
            }
            return make_qword(file_size, file_size_high);
        }

        __int64 get_compressed_size() const {
            return get_compressed_size(name_.c_str());
        }

        void flush() {
            if (!FlushFileBuffers(get_handle())) {
                AC_THROW(GetLastError(), "FlushFileBuffers");
            }
        }

        void set_security(SECURITY_INFORMATION security_information,
                          PSECURITY_DESCRIPTOR security_descriptor) {
            if (!SetFileSecurityW(name_.c_str(), security_information, security_descriptor)) {
                AC_THROW(GetLastError(), "SetFileSecurity");
            }
        }

        void set_attributes(DWORD attributes) {
            if (!SetFileAttributesW(name_.c_str(), attributes)) {
                AC_THROW(GetLastError(), "SetFileAttributes");
            }
        }

        [[nodiscard]] DWORD get_attributes() const {
            return GetFileAttributesW(name_.c_str());
        }

        [[nodiscard]] DWORD get_file_type() const {
            return ::GetFileType(get_handle());
        }

        void set_short_name(wchar_t const *short_name) {
            if (!SetFileShortNameW(get_handle(), short_name)) {
                AC_THROW(GetLastError(), "SetFileShortName");
            }
        }

        void get_time(LPFILETIME creation_time,
                      LPFILETIME last_access_time,
                      LPFILETIME last_write_time) const {
            if (!GetFileTime(get_handle(), creation_time, last_access_time, last_write_time)) {
                AC_THROW(GetLastError(), "GetFileTime");
            }
        }

        [[nodiscard]] FILETIME get_creation_time() const {
            FILETIME t;
            zero_pod(&t);
            get_time(&t, nullptr, nullptr);
            return t;
        }

        [[nodiscard]] FILETIME get_last_access_time() const {
            FILETIME t;
            zero_pod(&t);
            get_time(nullptr, &t, nullptr);
            return t;
        }

        FILETIME get_last_write_time() const {
            FILETIME t;
            zero_pod(&t);
            get_time(nullptr, nullptr, &t);
            return t;
        }

        void set_time(FILETIME const *creation_time,
                      FILETIME const *last_access_time,
                      FILETIME const *last_write_time) {
            if (!SetFileTime(get_handle(), creation_time, last_access_time, last_write_time)) {
                AC_THROW(GetLastError(), "SetFileTime");
            }
        }

        void set_creation_time(FILETIME const &time) {
            set_time(&time, nullptr, nullptr);
        }

        void set_last_access_time(FILETIME const &time) {
            set_time(nullptr, &time, nullptr);
        }

        void set_last_write_time(FILETIME const &time) {
            set_time(nullptr, nullptr, &time);
        }

        void get_information(LPBY_HANDLE_FILE_INFORMATION information) const {
            if (!GetFileInformationByHandle(get_handle(), information)) {
                AC_THROW(GetLastError(), "GetInformation");
            }
        }

        void byte_range_lock(ULONGLONG offset, ULONGLONG size) {
            DWORD offsetLow = get_low_dword(offset);
            DWORD offsetHigh = get_high_dword(offset);
            DWORD sizeLow = get_low_dword(size);
            DWORD sizeHigh = get_high_dword(size);
            if (!LockFile(get_handle(), offsetLow, offsetHigh, sizeLow, sizeHigh)) {
                AC_THROW(GetLastError(), "LockFile");
            }
        }
        //
        // Returns true if IO completed synchronosly and
        // returns false otherwise
        //
        bool byte_range_lock_ex(DWORD flags, OVERLAPPED *o, ULONGLONG size) {
            DWORD sizeLow = get_low_dword(size);
            DWORD sizeHigh = get_high_dword(size);
            DWORD error = ERROR_SUCCESS;

            if (!LockFileEx(get_handle(), flags, 0, sizeLow, sizeHigh, o)) {
                error = GetLastError();

                if (ERROR_IO_PENDING != error) {
                    AC_THROW(error, "LockFileEx");
                }
            }
            return (ERROR_SUCCESS == error);
        }

        void byte_range_unlock(ULONGLONG offset, ULONGLONG size) {
            DWORD offsetLow = get_low_dword(offset);
            DWORD offsetHigh = get_high_dword(offset);
            DWORD sizeLow = get_low_dword(size);
            DWORD sizeHigh = get_high_dword(size);

            if (!UnlockFile(get_handle(), offsetLow, offsetHigh, sizeLow, sizeHigh)) {
                AC_THROW(GetLastError(), "UnlockFile");
            }
        }
        //
        // Returns true if IO completed synchronosly and
        // returns false otherwise
        //
        bool byte_range_unlock_ex(OVERLAPPED *o, ULONGLONG size) {
            DWORD sizeLow = get_low_dword(size);
            DWORD sizeHigh = get_high_dword(size);
            DWORD error = ERROR_SUCCESS;

            if (!UnlockFileEx(get_handle(), 0, sizeLow, sizeHigh, o)) {
                error = GetLastError();

                if (ERROR_IO_PENDING != error) {
                    AC_THROW(GetLastError(), "UnlockFileEx");
                }
            }
            return (ERROR_SUCCESS == error);
        }

        void swap(file_object &other) {
            kernel_object::swap(other);
            std::swap(other.name_, name_);
        }

    protected:
        std::wstring name_;
    };

    inline void swap(file_object &lhs, file_object &rhs) {
        lhs.swap(rhs);
    }
    class scoped_file_delete {
    public:
        explicit scoped_file_delete(wchar_t const *name)
            : armed_(true)
            , name_(name ? name : std::wstring{}) {
        }

        scoped_file_delete(const scoped_file_delete &) = delete;
        scoped_file_delete &operator=(const scoped_file_delete &) = delete;

        scoped_file_delete(scoped_file_delete &&other) noexcept
            : armed_(other.armed_)
            , name_(std::move(other.name_)) {
            other.armed_ = true;
        }

        scoped_file_delete &operator=(scoped_file_delete &&other) noexcept {
            if (this != &other) {
                armed_ = other.armed_;
                name_ = std::move(other.name_);
            }
            return *this;
        }

        ~scoped_file_delete() noexcept {
            if (is_armed()) {
                try_delete();
            }
        }

        void disarm() noexcept {
            armed_ = false;
        }

        void arm() noexcept {
            armed_ = true;
        }

        [[nodiscard]] bool is_armed() const noexcept {
            return armed_ && !name_.empty();
        }

        void try_delete() noexcept {
            if (is_armed()) {
                if (DeleteFileW(name_.c_str())) {
                    disarm();
                }
            }
        }

        [[nodiscard]] std::wstring const &get_file_name() const noexcept {
            return name_;
        }

        void set_file_name(wchar_t const *name) {
            name_ = name;
        }

    private:
        bool armed_;
        std::wstring name_;
    };

    class scoped_files_delete {
    public:
        typedef std::vector<scoped_file_delete> files_t;

        typedef files_t::iterator iterator;
        typedef files_t::const_iterator const_iterator;

        typedef files_t::const_reference const_reference;
        typedef files_t::value_type value_type;

        scoped_files_delete() noexcept {
        }

        scoped_files_delete(const scoped_files_delete &) = delete;
        scoped_files_delete &operator=(const scoped_files_delete &) = delete;

        scoped_files_delete(scoped_files_delete &&other) noexcept
            : files_(std::move(other.files_)) {
        }

        scoped_files_delete &operator=(scoped_files_delete &&other) noexcept {
            if (this != &other) {
                files_ = std::move(other.files_);
            }
            return *this;
        }

        [[nodiscard]] iterator begin() {
            return files_.begin();
        }

        [[nodiscard]] const_iterator begin() const {
            return files_.begin();
        }

        [[nodiscard]] iterator end() {
            return files_.end();
        }

        [[nodiscard]] const_iterator end() const {
            return files_.end();
        }

        ~scoped_files_delete() {
            try_delete();
        }

        void disarm_all() noexcept {
            for (auto &e : files_) {
                e.disarm();
            }
        }

        void arm_all() noexcept {
            for (auto &e : files_) {
                e.arm();
            }
        }

        [[nodiscard]] bool has_armed() const noexcept {
            bool has_armed = false;

            for (auto &e : files_) {
                if (e.is_armed()) {
                    has_armed = true;
                    break;
                }
            }
            return has_armed;
        }

        void try_delete() noexcept {
            for (auto &e : files_) {
                e.try_delete();
            }
        }

        void add(scoped_file_delete &&file) {
            files_.push_back(std::move(file));
        }

        void add(scoped_files_delete &&files) {
            scoped_files_delete tmp(std::move(files));

            for (auto &e : tmp) {
                add(std::move(e));
            }
        }

        [[nodiscard]] size_t size() const noexcept {
            return files_.size();
        }

        [[nodiscard]] bool is_empty() const noexcept {
            return files_.empty();
        }

    private:
        files_t files_;
    };

    class directory_iterator {
    public:

        using iterator_category = std::input_iterator_tag;
        using value_type = WIN32_FIND_DATAW;
        using difference_type = ptrdiff_t;
        using pointer = WIN32_FIND_DATAW*;
        using reference = WIN32_FIND_DATAW&;

        // member functions
        directory_iterator() noexcept
            : h_(INVALID_HANDLE_VALUE) {
        }

        explicit directory_iterator(wchar_t const *file_name,
                                    FINDEX_INFO_LEVELS info_level = FindExInfoStandard,
                                    FINDEX_SEARCH_OPS search_operation = FindExSearchNameMatch,
                                    DWORD flags = 0)
            : h_(INVALID_HANDLE_VALUE) {
            zero_pod(&d_);
            h_ = FindFirstFileExW(
                file_name, info_level, &d_, search_operation, nullptr, flags);
            if (INVALID_HANDLE_VALUE == h_) {
                DWORD error = GetLastError();
                if (ERROR_FILE_NOT_FOUND != error) {
                    AC_THROW(GetLastError(), "FindFirstFileEx");
                }
            }
        }

        directory_iterator(directory_iterator &&other) noexcept
            : h_(other.h_) {
            other.h_ = INVALID_HANDLE_VALUE;
            std::swap(d_, other.d_);
        }

        ~directory_iterator() noexcept {
            close();
        }

        directory_iterator &operator=(directory_iterator &&other) noexcept {
            if (this != &other) {
                close();
                std::swap(h_, other.h_);
                std::swap(d_, other.d_);
            }
            return *this;
        }

        [[nodiscard]] WIN32_FIND_DATAW const &operator*() const noexcept {
            return d_;
        }

        [[nodiscard]] WIN32_FIND_DATAW const *operator->() const noexcept {
            return &d_;
        }

        directory_iterator &operator++() {
            if (!FindNextFileW(h_, &d_)) {
                DWORD error = GetLastError();
                if (ERROR_NO_MORE_FILES == error) {
                    close();
                } else {
                    AC_THROW(GetLastError(), "FindNextFile");
                }
            }
            return *this;
        }

    private:
        friend bool operator==(directory_iterator const &lhs, directory_iterator const &rhs);

        void close() noexcept {
            if (INVALID_HANDLE_VALUE != h_) {
                if (!FindClose(h_)) {
                    AC_CRASH_APPLICATION();
                }
                h_ = INVALID_HANDLE_VALUE;
            }
        }

        HANDLE h_{nullptr};
        WIN32_FIND_DATAW d_{};
    };

    inline bool operator==(directory_iterator const &lhs, directory_iterator const &rhs) {
        return lhs.h_ == rhs.h_;
    }

    inline bool operator!=(directory_iterator const &lhs, directory_iterator const &rhs) {
        return !(lhs == rhs);
    }

    namespace path {

        inline static void combine_in_place(std::wstring &path1, std::wstring const &path2) {
            if (path2.empty())
                return;

            if (path1.empty()) {
                path1 = path2;
                return;
            }

            if (path1.back() != '\\') {
                path1.push_back('\\');
            }

            path1.append(path2);
        }

        inline static [[nodiscard]] std::wstring combine(std::wstring const &path1,
                                                         std::wstring const &path2) {
            std::wstring result(path1);
            combine_in_place(result, path2);
            return result;
        }

    }; // namespace path

    inline void make_sure_parent_directory_exists(std::wstring const &path) {
        AC_THROW_IF(path.empty(), ERROR_INVALID_PARAMETER, "make_sure_parent_directory_exists path parameter cannot be empty");

        std::wstring expanded_path = expand_environment_variable(path);

        std::vector<std::wstring> path_history;
        bool found_parent = false;

        std::wstring parent_folder{expanded_path};
        std::wstring::size_type pos = parent_folder.rfind(L'\\');

        AC_THROW_IF(
            std::wstring::npos == pos,
            ERROR_INVALID_PARAMETER,
            "make_sure_parent_directory_exists path does not contain any '\\'");

        file_object fo;
        DWORD err = ERROR_SUCCESS;
        //
        // Look for the first existing parent folder
        //
        for (; pos != std::wstring::npos;) {
            parent_folder = parent_folder.substr(0, pos);
            //
            // a naive way to catch a relative path
            //
            AC_THROW_IF(
                parent_folder == L"\\",
                ERROR_INVALID_PARAMETER,
                "make_sure_parent_directory_exists path canot be relative");

            err = fo.try_create((parent_folder + L"\\").c_str(),
                                FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS);

            if (ERROR_SUCCESS == err) {
                DWORD file_attributes = fo.get_attributes();

                AC_THROW_IF_NOT(
                    (file_attributes & FILE_ATTRIBUTE_DIRECTORY), ERROR_INVALID_PARAMETER, "make_sure_parent_directory_exists path component does not have FILE_ATTRIBUTE_DIRECTORY");

                fo.close();
                found_parent = true;
                break;
            }
            //
            // remember current path
            //
            path_history.push_back(parent_folder);
            //
            // Look for parent folder
            //
            pos = parent_folder.rfind(L'\\');
        }

        AC_THROW_IF_NOT(found_parent, ERROR_INVALID_PARAMETER, "make_sure_parent_directory_exists path does not have any existing components");
        //
        // pop back all child folders and create them component by component
        //
        while (!path_history.empty()) {
            parent_folder = path_history.back();

            file_object::create_directory(parent_folder.c_str());

            path_history.pop_back();
        }
    }

    inline void make_sure_directory_exists(std::wstring const &path) {
        make_sure_parent_directory_exists(path::combine(path, std::wstring(L"fake_file_name")));
    }

    inline scoped_files_delete find_delete_candidates_by_constraint(
        wstring_collection const &file_queries,
        std::optional<std::chrono::system_clock::time_point> oldest_allowed_time = std::nullopt,
        std::optional<long long> max_allowed_size = std::nullopt,
        std::optional<long> min_files_to_keep = std::nullopt,
        std::optional<long> max_files_to_keep = std::nullopt) {
        struct file_info_t {
            std::wstring file_name;
            long long file_size;
            std::chrono::system_clock::time_point create_time;
        };

        struct compare_file_info_by_fie_name {
            bool operator()(file_info_t const &lhs, file_info_t const &rhs) const {
                return lhs.file_name < rhs.file_name;
            }
        };

        struct compare_file_info_by_create_time {
            bool operator()(file_info_t const &lhs, file_info_t const &rhs) const {
                return lhs.create_time < rhs.create_time;
            }
        };
        //
        // Set of files sorted by name used to enforce that
        // each file is counted only once
        //
        using file_name_set_t = std::set<file_info_t, compare_file_info_by_fie_name>;
        //
        // Multiset of file sorted by create time in reverse order
        // (from largest to smallest).
        // Used to quickly locate oldest files
        //
        using file_time_multiset_t =
            std::multiset<file_info_t, compare_file_info_by_create_time>;

        file_name_set_t files_to_delete;
        file_name_set_t files_by_name;
        bool delete_all{false};

        if (min_files_to_keep && max_files_to_keep) {
            AC_CODDING_ERROR_IF(min_files_to_keep > max_files_to_keep);
        }
        //
        // If no constraints then delete all files
        //
        if (!oldest_allowed_time && !max_allowed_size && !min_files_to_keep) {
            delete_all = true;
        }

        long long remaining_size{0LL};
        //
        // Find all files matching each of the provided queries.
        // Caller can use multiple queries when he wants to monitor
        // files with different extensions and\or files in multiple
        // folders. For example query set can be:
        // c:\wer1\*.dmp
        // c:\wer1\*.dm?
        // c:\wer1\*.zip
        // c:\wer1\*.cab
        // c:\wer2\*.dmp
        // c:\wer2\*.zip
        // c:\wer2\*.z*
        // c:\wer2\*.cab
        // In that case we will enforce constraints across all files
        // matching all filters, and if some files match multiple filters
        // then we will count that file only once.
        //
        // Always delete files that are too old.
        //
        for (auto const &query : file_queries) {
            directory_iterator dir_cur{query.c_str()};
            directory_iterator dir_end;

            for (; dir_cur != dir_end; ++dir_cur) {
                WIN32_FIND_DATAW const &data{*dir_cur};

                file_info_t file_info{
                    std::wstring{data.cFileName},
                    make_longlong(data.nFileSizeLow, data.nFileSizeHigh),
                    filetime_to_system_clock_time_point(data.ftCreationTime)};

                if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }

                if (delete_all || oldest_allowed_time &&
                                      *oldest_allowed_time > file_info.create_time) {
                    files_to_delete.insert(file_info);
                } else if (max_allowed_size || max_files_to_keep) {
                    bool was_inserted = files_by_name.insert(file_info).second;
                    if (was_inserted) {
                        remaining_size += file_info.file_size;
                    }
                }
            }
        }
        //
        // If we are still over size or count limit,
        // and if we are not under min files to keep
        //
        if (min_files_to_keep.value_or(0) < static_cast<long>(files_by_name.size()) &&
            ((max_allowed_size && remaining_size > *max_allowed_size) ||
             (max_files_to_keep && static_cast<long>(files_by_name.size()) > *max_files_to_keep))) {
            file_time_multiset_t files_by_time;
            //
            // Sort remaining files by timestamp
            // in descending order
            //
            while (!files_by_name.empty()) {
                file_info_t const &file_info{*files_by_name.begin()};
                files_by_time.insert(file_info);
                files_by_name.erase(files_by_name.begin());
            }
            //
            // Store how much we need to free
            //
            long long size_to_free =
                max_allowed_size ? remaining_size - *max_allowed_size : 0;
            long long files_count_to_delete =
                max_files_to_keep ? files_by_time.size() - *max_files_to_keep : 0;
            //
            // Keep deleting oldest file until we free enough space and
            // got below max file count. Stop if number of
            // remaining files reaches min file count
            //
            for (auto it{files_by_time.begin()};
                 it != files_by_time.end() &&
                 min_files_to_keep.value_or(0) < static_cast<long>(files_by_time.size()) &&
                 (size_to_free > 0 || files_count_to_delete > 0);) {
                {
                    file_info_t const &file_info{*it};
                    bool was_inserted = files_to_delete.insert(file_info).second;
                    if (was_inserted) {
                        --files_count_to_delete;
                        size_to_free -= file_info.file_size;
                    }
                }
                it = files_by_time.erase(it);
            }
        }
        //
        // Create RAII deleter helpers and return them as a collection
        // Each deletes will attempt to delete file in it's destructor.
        // Before that caller has a chance to examine list and cancel
        // deletion for selected files.
        //
        scoped_files_delete result;
        while (!files_to_delete.empty()) {
            {
                file_info_t const &file_info{*files_to_delete.begin()};
                result.add(scoped_file_delete{file_info.file_name.c_str()});
            }
            files_to_delete.erase(files_to_delete.begin());
        }
        return result;
    }

} // namespace ac

#endif //_AC_HELPERS_WIN32_LIBRARY_FILE_OBJECT_HEADER_