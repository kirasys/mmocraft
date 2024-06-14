#include "pch.h"
#include "file_mapping.h"

#include "logging/logger.h"

namespace win
{
    FileMapping::FileMapping()
    {
        
    }

    FileMapping::~FileMapping()
    {
        if (_mapping_address != NULL)
            ::UnmapViewOfFile(_mapping_address);
    }

    void FileMapping::close() noexcept
    {
        if (_mapping_address != NULL) {
            ::UnmapViewOfFile(_mapping_address);
            _mapping_address = NULL;
        }

        _mapping_handle.reset();
        _file_handle.reset();
    }

    bool FileMapping::open(const std::string& file_path)
    {
        _file_handle.reset(::CreateFileA(
            file_path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        ));

        if (not _file_handle.is_valid()) {
            CONSOLE_LOG(error) << "Unable to open file: " << file_path;
            return false;
        }

        auto mapping_handle = ::CreateFileMapping(
            _file_handle.get(),
            NULL,
            PAGE_READWRITE,
            0, 0,
            NULL
        );

        if (mapping_handle == NULL) {
            CONSOLE_LOG(error) << "Unable to create file mapping with " << ::GetLastError();
            return false;
        }

        _mapping_handle.reset(mapping_handle);

        _mapping_address = ::MapViewOfFile(
            mapping_handle,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            0
        );

        if (mapping_handle == NULL) {
            CONSOLE_LOG(error) << "Unable to mapping file with " << ::GetLastError();
            return false;
        }

        return true;
    }

    void FileMapping::flush(std::size_t n)
    {
        if (not ::FlushViewOfFile(_mapping_address, n))
            CONSOLE_LOG(error) << "Unable to flush mapping.";
    }
}