#pragma once

namespace game
{
	using BlockID = char;

	constexpr BlockID BLOCK_AIR = 0;
	constexpr BlockID BLOCK_STONE = 1;
	constexpr BlockID BLOCK_GRASS = 2;
	constexpr BlockID BLOCK_DIRT = 3;
	constexpr BlockID BLOCK_COBBLE = 4;
	constexpr BlockID BLOCK_WOOD = 5;
	constexpr BlockID BLOCK_SAPLING = 6;
	constexpr BlockID BLOCK_BEDROCK = 7;
	constexpr BlockID BLOCK_WATER = 8;
	constexpr BlockID BLOCK_STILL_WATER = 9;
	constexpr BlockID BLOCK_LAVA = 10;
	constexpr BlockID BLOCK_STILL_LAVA = 11;
	constexpr BlockID BLOCK_SAND = 12;
	constexpr BlockID BLOCK_GRAVEL = 13;
	constexpr BlockID BLOCK_GOLD_ORE = 14;
	constexpr BlockID BLOCK_IRON_ORE = 15;
	constexpr BlockID BLOCK_COAL_ORE = 16;
	constexpr BlockID BLOCK_LOG = 17;
	constexpr BlockID BLOCK_LEAVES = 18;
	constexpr BlockID BLOCK_SPONGE = 19;
	constexpr BlockID BLOCK_GLASS = 20;
	constexpr BlockID BLOCK_RED = 21;
	constexpr BlockID BLOCK_ORANGE = 22;
	constexpr BlockID BLOCK_YELLOW = 23;
	constexpr BlockID BLOCK_LIME = 24;
	constexpr BlockID BLOCK_GREEN = 25;
	constexpr BlockID BLOCK_TEAL = 26;
	constexpr BlockID BLOCK_AQUA = 27;
	constexpr BlockID BLOCK_CYAN = 28;
	constexpr BlockID BLOCK_BLUE = 29;
	constexpr BlockID BLOCK_INDIGO = 30;
	constexpr BlockID BLOCK_VIOLET = 31;
	constexpr BlockID BLOCK_MAGENTA = 32;
	constexpr BlockID BLOCK_PINK = 33;
	constexpr BlockID BLOCK_BLACK = 34;
	constexpr BlockID BLOCK_GRAY = 35;
	constexpr BlockID BLOCK_WHITE = 36;
	constexpr BlockID BLOCK_DANDELION = 37;
	constexpr BlockID BLOCK_ROSE = 38;
	constexpr BlockID BLOCK_BROWN_SHROOM = 39;
	constexpr BlockID BLOCK_RED_SHROOM = 40;
	constexpr BlockID BLOCK_GOLD = 41;
	constexpr BlockID BLOCK_IRON = 42;
	constexpr BlockID BLOCK_DOUBLE_SLAB = 43;
	constexpr BlockID BLOCK_SLAB = 44;
	constexpr BlockID BLOCK_BRICK = 45;
	constexpr BlockID BLOCK_TNT = 46;
	constexpr BlockID BLOCK_BOOKSHELF = 47;
	constexpr BlockID BLOCK_MOSSY_ROCKS = 48;
	constexpr BlockID BLOCK_OBSIDIAN = 49;

	enum BlockMode
	{
		UNSET = 0,
		SET = 1,
	};
}