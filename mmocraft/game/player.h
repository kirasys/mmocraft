#pragma once

#include <string.h>

#include "net/packet.h"
#include "util/common_util.h"

namespace game
{
	using PlayerID = unsigned;

	enum PlayerType
	{
		INVALID,

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
		Player(PlayerID, PlayerType, const char* username, const char* password);

		PlayerID get_identity_number() const
		{
			return identity_number;
		}

	private:
		PlayerID identity_number;
		PlayerType player_type;
		char _username[net::PacketFieldConstraint::max_username_length + 1];
		char _password[net::PacketFieldConstraint::max_password_length + 1];
	};
}