#pragma once

#include <filesystem>

#include "game/block.h"
#include "util/common_util.h"
#include "util/math.h"

#define WORLD_METADATA_FORMAT_VERSION 1

namespace game
{
    class WorldGenerator
    {
    public:
        static constexpr std::size_t block_file_header_size = 4;

        static void create_world_if_not_exist(const std::filesystem::path& block_data_path, const std::filesystem::path& metadata_path);

        static void create_block_file(const std::filesystem::path& block_data_path, util::Coordinate3D map_size, std::size_t map_volume);

        static void create_metadata_file(const std::filesystem::path& metadata_path, util::Coordinate3D map_size);

        static std::unique_ptr<BlockID[]> generate_flat_world(util::Coordinate3D map_size);
    };
}