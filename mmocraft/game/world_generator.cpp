#include "pch.h"
#include "world_generator.h"

#include "config/config.h"
#include "proto/generated/config.pb.h"
#include "proto/generated/world_metadata.pb.h"
#include "util/protobuf_util.h"
#include "logging/logger.h"

namespace fs = std::filesystem;

namespace game
{
    void WorldGenerator::create_world_if_not_exist(const fs::path& block_data_path, const fs::path& metadata_path)
    {
        if (fs::exists(metadata_path))
            return;

        const auto& world_conf = config::get_world_config();

        CONSOLE_LOG_IF(fatal, world_conf.width() < 10 || world_conf.height() < 10 || world_conf.length() < 10)
            << "world map size must greater than 10x10x10.";

        auto map_size = util::Coordinate3D{ world_conf.width(), world_conf.height(), world_conf.length()};
        std::size_t map_volume = map_size.x * map_size.y * map_size.z;

        // write world files to the disk.
        create_block_file(block_data_path, map_size, map_volume);
        create_metadata_file(metadata_path, map_size);
    }

    void WorldGenerator::create_block_file(const fs::path& block_data_path, util::Coordinate3D map_size, std::size_t map_volume)
    {
        auto blocks_ptr = WorldGenerator::generate_flat_world(map_size);

        std::ofstream block_file(block_data_path);
        // block data header (map volume)
        map_volume = ::htonl(u_long(map_volume));
        block_file.write(reinterpret_cast<char*>(&map_volume), block_file_header_size);
        // raw block data
        block_file.write(blocks_ptr.get(), map_size.x * map_size.y * map_size.z);
    }

    void WorldGenerator::create_metadata_file(const fs::path& metadata_path, util::Coordinate3D map_size)
    {
        WorldMetadata metadata;
        metadata.set_format_version(WORLD_METADATA_FORMAT_VERSION);

        metadata.set_width(map_size.x);
        metadata.set_height(map_size.y);
        metadata.set_length(map_size.z);
        metadata.set_volume(map_size.x * map_size.y * map_size.z);

        metadata.set_spawn_x(map_size.x / 2);
        metadata.set_spawn_y(map_size.y);
        metadata.set_spawn_z(map_size.z / 2);
        metadata.set_spawn_yaw(0);
        metadata.set_spawn_pitch(0);

        metadata.set_created_at(util::current_timestmap());

        util::proto_message_to_json_file(metadata, metadata_path);
    }



    std::unique_ptr<BlockID[]> WorldGenerator::generate_flat_world(util::Coordinate3D map_size)
    {
        const auto plain_size = map_size.x * map_size.z;
        auto blocks = new BlockID[plain_size * map_size.y]();

        auto num_of_dirt_block = plain_size * (map_size.y / 2);
        std::memset(blocks, BLOCK_DIRT, num_of_dirt_block);
        std::memset(blocks + num_of_dirt_block, BLOCK_GRASS, plain_size);
        std::memset(blocks, BLOCK_BEDROCK, plain_size);

        return std::unique_ptr<BlockID[]>(blocks);
    }
}