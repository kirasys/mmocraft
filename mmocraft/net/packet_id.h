#pragma once

namespace net
{
    namespace packet_type_id {
        enum value
        {
            // Client <-> Server
            handshake = 0,
            set_player_position = 8,
            chat_message = 0xD,

            // Client -> Server
            set_block_client = 5,

            // Server -> Client
            ping = 1,
            level_initialize = 2,
            level_datachunk = 3,
            level_finalize = 4,
            set_block_server = 6,
            spawn_player = 7,
            update_player_position = 9,
            update_player_coordinate = 0xA,
            update_player_orientation = 0xB,
            despawn_player = 0xC,
            disconnect_player = 0xE,
            update_usertype = 0xF,

            // CPE
            ext_info = 0x10,
            ext_entry = 0x11,
            click_distance = 0x12,
            custom_blocks = 0x13,
            held_block = 0x14,
            text_hotkey = 0x15,
            ext_add_playername = 0x16,
            ext_add_entity2 = 0x21,
            ext_remove_playername = 0x18,
            env_set_color = 0x19,
            make_selection = 0x1A,
            remove_selection = 0x1B,
            block_permissions = 0x1C,
            change_model = 0x1D,
            env_set_weathertype = 0x1F,
            hack_control = 0x20,
            player_clicked = 0x22,
            define_block = 0x23,
            remove_block_definition = 0x24,
            define_blockext = 0x25,
            bulk_block_update = 0x26,
            set_text_color = 0x27,
            set_map_env_url = 0x28,
            set_map_env_property = 0x29,
            set_entity_property = 0x2A,
            two_way_ping = 0x2B,
            set_inventory_order = 0x2C,
            set_hotbar = 0x2D,
            set_spawnpoint = 0x2E,
            velocity_control = 0x2F,
            define_effect = 0x30,
            spawn_effect = 0x31,
            define_model = 0x32,
            define_modelpart = 0x33,
            undefine_model = 0x34,
            ext_entity_teleport = 0x36,

            /* Custom Protocol */
            set_playerid = 0x37,
            ext_message = 0x38,
            ext_ping = 0x39,

            // Indicate size of the enum class.
            size,

            // Invalid packet ID.
            invalid = 0xFF,
        };
    }
}