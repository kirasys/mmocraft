#pragma once

#include <string.h>

#include "net/packet.h"
#include "util/common_util.h"

namespace game
{
	using PlayerID = unsigned;

	enum PlayerType
	{
		// Users logged in without password.
		// destory all information after disconnecting.
		GUEST,

		// Users logged in with password but is not registered. 
		// they can register by entering command.
		NEW_USER,

		// Users logged in successfully.
		AUTHENTICATED_USER,

		// Users with administrator privileges.
		ADMIN,
	};

	class Player : util::NonCopyable
	{
	public:
		Player(PlayerID player_id, PlayerType player_type, const char* username, const char* password);

	private:
		PlayerID _id;
		PlayerType _type;
		char _username[net::PacketFieldConstraint::max_username_length + 1];
		char _password[net::PacketFieldConstraint::max_password_length + 1];
	};
}