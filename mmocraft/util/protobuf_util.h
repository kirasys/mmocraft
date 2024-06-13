#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <google/protobuf/util/json_util.h>

#include "logging/logger.h"

namespace util
{
    void proto_message_to_json_file(const google::protobuf::Message& message, const char* file_path);

    void json_file_to_proto_message(google::protobuf::Message* message, const char* file_path);
}