#include "pch.h"
#include "master_server.h"

#include <array>
#include <cstring>

#include "config/config.h"
#include "database/sql_statement.h"
#include "logging/error.h"

namespace net
{
	MasterServer::MasterServer(std::string_view ip,
		int port,
		unsigned max_client_connections,
		unsigned num_of_event_threads,
		int concurrency_hint)
		: server_core{ *this, ip, port, max_client_connections, num_of_event_threads, concurrency_hint }
		, database_core{ }
	{ 
		const auto& conf = config::get_config();

		if (not database_core.connect_with_password(conf.db))
			throw error::DATABASE_CONNECT;
	}

	error::ErrorCode MasterServer::handle_packet(DescriptorType::Connection conn_descriptor, Packet* packet)
	{
		switch (packet->id) {
		case PacketID::Handshake:
			return handle_handshake_packet(conn_descriptor, *static_cast<PacketHandshake*>(packet));
		default:
			return error::PACKET_UNIMPLEMENTED_ID;
		}
	}

	error::ErrorCode MasterServer::handle_handshake_packet(DescriptorType::Connection conn_descriptor, PacketHandshake& packet)
	{
		deferred_packet_stack.push<net::PacketHandshake>(conn_descriptor, packet);
		return error::PACKET_HANDLE_DEFERRED;
	}

	void MasterServer::serve_forever()
	{
		server_core.start_network_io_service();

		while (1) {
			std::size_t start_tick = util::current_monotonic_tick();

			flush_deferred_packet();

			ConnectionDescriptor::flush_server_message();
			ConnectionDescriptor::flush_client_message();

			std::size_t end_tick = util::current_monotonic_tick();

			if (auto diff = end_tick - start_tick; diff < 1000)
				util::sleep_ms(std::max(1000 - diff, std::size_t(100)));
		}
	}

	void MasterServer::process_deferred_packet_result()
	{
		DeferredPacketResult* results[] = {
			deferred_handshake_packet_event.result.pop(),
		};

		for (const auto* result : results) {
			while (result) {
				process_deferred_packet_result_internal(result);
				result = result->next;
			}
		}

		for (auto result : results) {
			while (result) {
				delete result;
			}
		}
	}

	void MasterServer::process_deferred_packet_result_internal(const DeferredPacketResult* result)
	{
		switch (result->error_code) {
		case error::PACKET_RESULT_SUCCESS_LOGIN:
			break;
		case error::PACKET_RESULT_FAIL_LOGIN:
		case error::PACKET_RESULT_ALREADY_LOGIN:
			ConnectionDescriptor::disconnect(
				result->connection_descriptor,
				error::get_error_message(result->error_code)
			);
			break;
		default:
			logging::cerr() << "Unexpected packet result: " << result->error_code;
		}
	}

	void MasterServer::flush_deferred_packet()
	{
		flush_deferred_packet_internal(deferred_handshake_packet_event);
	}

	/**
	 *  Event handler interface
	 */

	void MasterServer::handle_deferred_packet(DeferredPacketEvent<PacketHandshake>* event)
	{
		database::PlayerLoginSQL player_login{ database_core.get_connection_handle() };
		database::PlayerSearchSQL player_search{ database_core.get_connection_handle() };

		for (const auto* packet = event->head; packet; packet = packet->next) {
			auto player_type = game::PlayerType::INVALID;

			if (not player_search.search(packet->username)) {
				player_type = std::strlen(packet->password) ? game::PlayerType::NEW_USER : game::PlayerType::GUEST;
			}
			else if (player_login.authenticate(packet->username, packet->password)) {
				player_type = std::strcmp(packet->username, "admin")
					? game::PlayerType::AUTHENTICATED_USER : game::PlayerType::ADMIN;
			}

			error::ErrorCode result = error::PACKET_RESULT_FAIL_LOGIN;

			if (player_type != game::PlayerType::INVALID) {
				result = ConnectionDescriptor::associate_game_player(
					packet->connection_descriptor,
					player_search.get_player_identity_number(),
					player_type,
					packet->username,
					packet->password) ? error::PACKET_RESULT_SUCCESS_LOGIN : error::PACKET_RESULT_ALREADY_LOGIN;
			}

			event->result.push(packet->connection_descriptor, result);
		}
	}
}