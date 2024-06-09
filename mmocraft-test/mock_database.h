#pragma once

#include "pch.h"

namespace test
{
    class MockDatabase
    {
    public:
        MockDatabase();
        ~MockDatabase();

        void set_fetch_fail_once()
        {
            fetch_result = false;
        }

        void set_outbound_integer(SQLINTEGER value)
        {
            outbound_integer = value;
        }

        bool fetch()
        {
            auto result = fetch_result;
            fetch_result = true;
            return result;
        }

        void outbound_integer_column(SQLINTEGER& value)
        {
            value = outbound_integer;
        }

        void outbound_integer_column(SQLUINTEGER& value)
        {
            value = SQLUINTEGER(outbound_integer);
        }

    private:
        bool fetch_result = true;
        SQLINTEGER outbound_integer = 0;
    };
}