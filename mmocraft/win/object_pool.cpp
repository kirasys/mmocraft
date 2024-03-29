#include "pch.h"
#include "object_pool.h"

namespace win
{
	// 100 object pools is enough to use during the program execution.
	constexpr int MAX_OBJECT_POOL_AMOUNT = 100;
	std::atomic<std::uint32_t> num_of_free_object_pool{ MAX_OBJECT_POOL_AMOUNT };
	void* pool_table[MAX_OBJECT_POOL_AMOUNT + 1];
}