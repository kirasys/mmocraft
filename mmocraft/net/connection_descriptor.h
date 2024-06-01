#pragma once

#include <cassert>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "io/io_event.h"
#include "game/player.h"

namespace net
{
	enum ConnectionLevelDescriptor { };

	enum WorkerLevelDescriptor { };

	enum AdminLevelDescriptor { };

	class ConnectionServer;

	class ConnectionDescriptor
	{
	public:
		struct DescriptorData
		{
			net::ConnectionServer* connection;
			win::Socket raw_socket;

			io::IoSendEventSmallData* io_send_event_small_data;
			io::IoSendEventData* io_send_event_data;
			io::IoSendEvent* io_send_event;
			io::IoRecvEvent* io_recv_event;

			game::Player* player = nullptr;

			bool is_online = false;
			bool is_send_event_running = false;
			bool is_recv_event_running = false;
		};

		ConnectionDescriptor() = delete;

		static void initialize(unsigned max_client_connections);

		template <typename DescriptorType>
		static bool disconnect(DescriptorType desc, std::string_view reason)
		{
			auto& desc_entry = descriptor_table[desc];
			if (not desc_entry.is_online)
				return false;

			desc_entry.connection->set_offline();

			net::PacketDisconnectPlayer disconnect_packet{ reason };

			if constexpr (std::is_same_v<DescriptorType, ConnectionLevelDescriptor>)
				return disconnect_packet.serialize(*desc_entry.io_send_event_small_data);
			else if constexpr (std::is_same_v<DescriptorType, WorkerLevelDescriptor>)
				return disconnect_packet.serialize(*desc_entry.io_send_event_data);
			else
				assert(false);
		}

		// Connection level apis.

		// Worker level apis.

		static void flush_server_message(WorkerLevelDescriptor);

		static void flush_client_message(WorkerLevelDescriptor);

		static bool associate_game_player(WorkerLevelDescriptor, game::PlayerID, game::PlayerType, const char*, const char*);


		/// Admin level apis.

		static bool issue_descriptor_number(AdminLevelDescriptor&);

		static void delete_descriptor(AdminLevelDescriptor);

		static void set_descriptor_data(AdminLevelDescriptor, DescriptorData);

		static void activate_receive_cycle(AdminLevelDescriptor);

		static void activate_send_cycle(AdminLevelDescriptor);

	private:

		static void shrink_descriptor_end();

		static unsigned descriptor_table_capacity;
		static unsigned descriptor_end;
		static std::unique_ptr<DescriptorData[]> descriptor_table;

		static std::unordered_map<game::PlayerID, game::Player*> player_lookup_table;
	};
}