#pragma once

#include <memory>
#include <unordered_map>

#include "io/io_event.h"

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

			game::Player* player;

			bool is_online = false;
			bool is_send_event_running = false;
			bool is_recv_event_running = false;
		};

		ConnectionDescriptor() = delete;

		static void initialize(unsigned max_client_connections);

		// Connection level apis.

		static bool push_server_message(ConnectionLevelDescriptor, std::byte*, std::size_t);

		static bool push_disconnect_message(ConnectionLevelDescriptor, std::string_view);

		static bool on_login_complete(ConnectionLevelDescriptor, game::PlayerID, game::PlayerType, const char*, const char*);

		// Worker level apis.

		static bool push_server_message(WorkerLevelDescriptor, std::byte*, std::size_t);

		static void flush_server_message(WorkerLevelDescriptor);

		static void flush_client_message(WorkerLevelDescriptor);


		/// Admin level apis.

		static bool issue_descriptor_number(AdminLevelDescriptor&);

		static void delete_descriptor(AdminLevelDescriptor);

		static void set_descriptor_data(AdminLevelDescriptor, DescriptorData);

		static void activate_receive_cycle(AdminLevelDescriptor);

		static void activate_send_cycle(AdminLevelDescriptor);

		static bool push_disconnect_message(AdminLevelDescriptor, std::string_view);

	private:

		static void shrink_descriptor_end();

		static unsigned descriptor_table_capacity;
		static unsigned descriptor_end;
		static std::unique_ptr<DescriptorData[]> descriptor_table;

		static std::unordered_map<game::PlayerID, game::Player*> player_lookup_table;
	};
}