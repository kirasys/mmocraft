#pragma once

namespace game
{
	using BlockID = char;

	namespace block_id {
		constexpr BlockID air = 0;
		constexpr BlockID stone = 1;
		constexpr BlockID grass = 2;
		constexpr BlockID dirt = 3;
		constexpr BlockID cobble = 4;
		constexpr BlockID wood = 5;
		constexpr BlockID sapling = 6;
		constexpr BlockID bedrock = 7;
		constexpr BlockID water = 8;
		constexpr BlockID still_water = 9;
		constexpr BlockID lava = 10;
		constexpr BlockID still_lava = 11;
		constexpr BlockID sand = 12;
		constexpr BlockID gravel = 13;
		constexpr BlockID gold_ore = 14;
		constexpr BlockID iron_ore = 15;
		constexpr BlockID coal_ore = 16;
		constexpr BlockID log = 17;
		constexpr BlockID leaves = 18;
		constexpr BlockID sponge = 19;
		constexpr BlockID glass = 20;
		constexpr BlockID red = 21;
		constexpr BlockID orange = 22;
		constexpr BlockID yellow = 23;
		constexpr BlockID lime = 24;
		constexpr BlockID green = 25;
		constexpr BlockID teal = 26;
		constexpr BlockID aqua = 27;
		constexpr BlockID cyan = 28;
		constexpr BlockID blue = 29;
		constexpr BlockID indigo = 30;
		constexpr BlockID violet = 31;
		constexpr BlockID magenta = 32;
		constexpr BlockID pink = 33;
		constexpr BlockID black = 34;
		constexpr BlockID gray = 35;
		constexpr BlockID white = 36;
		constexpr BlockID dandelion = 37;
		constexpr BlockID rose = 38;
		constexpr BlockID brown_shroom = 39;
		constexpr BlockID red_shroom = 40;
		constexpr BlockID gold = 41;
		constexpr BlockID iron = 42;
		constexpr BlockID double_slab = 43;
		constexpr BlockID slab = 44;
		constexpr BlockID brick = 45;
		constexpr BlockID tnt = 46;
		constexpr BlockID bookshelf = 47;
		constexpr BlockID mossy_rocks = 48;
		constexpr BlockID obsidian = 49;
	};

	enum block_creation_mode
	{
		unset = 0,
		set = 1,
	};
}