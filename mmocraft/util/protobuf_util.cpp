#include "pch.h"
#include "protobuf_util.h"

namespace util
{
    void proto_message_to_json_file(const google::protobuf::Message& message, const char* file_path)
    {
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;

        std::string config_json;
        google::protobuf::util::MessageToJsonString(message, &config_json, options);

        std::ofstream config_file(file_path);
        CONSOLE_LOG_IF(error, config_file.fail()) << "Fail to create config file at \"" << file_path << '"';
        config_file << config_json << std::endl;
    }

    void json_file_to_proto_message(google::protobuf::Message* message, const char* file_path)
    {
        std::ifstream config_file(file_path);
        std::string config_json((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
        google::protobuf::util::JsonStringToMessage(config_json, message);
    }
}