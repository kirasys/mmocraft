#pragma once

#include "util/common_util.h"
#include "win/win_type.h"
#include "win/win_base_object.h"
#include "win/smart_handle.h"

namespace win
{
    class FileMapping final : public win::WinBaseObject<win::Handle>, util::NonCopyable
    {
    public:
        FileMapping();
        ~FileMapping();

        FileMapping(FileMapping&&) = default;
        FileMapping& operator=(FileMapping&&) = default;

        bool is_valid() const override
        {
            return _file_handle.is_valid() && _mapping_handle.is_valid() && _mapping_address != NULL;
        }

        win::Handle get_handle() const override
        {
            return _mapping_handle.get();
        }

        void close() noexcept override;

        bool open(const std::string& file_path);

        void flush(std::size_t n = 0);

        auto data()
        {
            return static_cast<char*>(_mapping_address);
        }

    private:
        win::UniqueHandle _file_handle;
        win::UniqueHandle _mapping_handle;
        void* _mapping_address = 0;
    };
}