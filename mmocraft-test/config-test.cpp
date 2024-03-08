#include "pch.h"
#include "../mmocraft/config/config.cpp"
#include "../mmocraft/logging/logger.cpp"

TEST(ConfigTest, GetConfig) {
	decltype(auto) conf = config::get_config();

	EXPECT_TRUE(conf.loaded);
}