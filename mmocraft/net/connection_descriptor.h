#pragma once

#include <cassert>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "io/io_event.h"
#include "game/player.h"

namespace net
{
	// Descriptor enum class for the concurrent access. (especially sending message)
	// : It is possible to access the descriptor simultaneously at the different event (send, recv, deferrent_packet, etc..)
	//   By specifying the event type when calling method, we can ensure that only one thread accessing the same data.
	namespace DescriptorType
	{
		enum Connection { };
		// Because connection descriptor maintains dedicated resources for each events.
		// it is safe to use the same type for different socket events.
		using Accept = Connection;
		using Receive = Connection;
		using Send = Connection;

		enum DeferredPacket { };

		enum Tick { };
		constexpr Tick tick_descriptor = Tick(0);
	}


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

		template <typename DescType>
		static bool disconnect(DescType desc, std::string_view reason)
		{
			auto& desc_entry = descriptor_table[desc];
			if (not desc_entry.is_online)
				return false;

			desc_entry.connection->set_offline();

			net::PacketDisconnectPlayer disconnect_packet{ reason };

			if constexpr (std::is_same_v<DescType, DescriptorType::Connection>)
				return disconnect_packet.serialize(*desc_entry.io_send_event_small_data);
			else if constexpr (std::is_same_v<DescType, DescriptorType::DeferredPacket>)
				return disconnect_packet.serialize(*desc_entry.io_send_event_data);
			else
				assert(false);
		}


		// Tick thread level apis.

		static void flush_server_message(DescriptorType::Tick);

		static void flush_client_message(DescriptorType::Tick);

		// Deferred packet level apis.

		static bool associate_game_player(DescriptorType::DeferredPacket, game::PlayerID, game::PlayerType, const char*, const char*);

		/// Connection (socket event) level apis.

		static bool issue_descriptor_number(DescriptorType::Connection&);

		static void set_descriptor_data(DescriptorType::Connection, DescriptorData);

		static void delete_descriptor(DescriptorType::Connection);

		static void activate_receive_cycle(DescriptorType::Connection);

		static void activate_send_cycle(DescriptorType::Connection);

	private:

		static void shrink_descriptor_end();

		static unsigned descriptor_table_capacity;
		static unsigned descriptor_end;
		static std::unique_ptr<DescriptorData[]> descriptor_table;

		static std::unordered_map<game::PlayerID, game::Player*> player_lookup_table;
	};
}