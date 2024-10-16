#include "pch.h"
#include <vector>

#include "win/object_pool.h"

struct Foo {
	int a = 0;
	char b = '\0';
	Foo() = default;
	Foo(int a_, char b_)
		: a(a_), b(b_) { }
	~Foo() {

	}
};

TEST(object_pool, success_reserve_to_max) {
	const int max_capacity = 1000;
	win::ObjectPool<Foo> object_pool(max_capacity);

	bool success = object_pool.reserve(max_capacity);

	EXPECT_TRUE(success);
	EXPECT_EQ(object_pool.capacity(), max_capacity);
}
TEST(object_pool, new_object_working_properly) {
	win::ObjectGlobalPool<Foo> object_pool(1000);

	auto obj1_id = object_pool.new_object(1, 'a');
	auto obj2_id = object_pool.new_object(2, 'b');
	auto obj3_id = object_pool.new_object(3, 'c');
	auto obj1 = obj1_id.get();
	auto obj2 = obj2_id.get();
	auto obj3 = obj3_id.get();

	ASSERT_TRUE(obj1 != nullptr);
	ASSERT_TRUE(obj2 != nullptr);
	ASSERT_TRUE(obj3 != nullptr);

	EXPECT_EQ(obj1->a, 1);
	EXPECT_EQ(obj1->b, 'a');

	EXPECT_EQ(obj2->a, 2);
	EXPECT_EQ(obj2->b, 'b');

	EXPECT_EQ(obj3->a, 3);
	EXPECT_EQ(obj3->b, 'c');
}

TEST(object_pool, success_new_object_to_max) {
	const int max_capacity = 10;
	win::ObjectGlobalPool<Foo> object_pool(max_capacity);
	
	std::vector<win::ObjectGlobalPool<Foo>::Pointer> object_ptrs;
	for (int i = 0; i < max_capacity; i++)
		object_ptrs.emplace_back(object_pool.new_object(i, 'a'));

	for (int i = 0; i < max_capacity; i++) {
		EXPECT_NE(object_ptrs[i].get(), nullptr);
	}
}

TEST(object_pool, fail_new_object_over_capacity) {
	const int max_capacity = 10;
	win::ObjectGlobalPool<Foo> object_pool(max_capacity);

	std::vector<win::ObjectGlobalPool<Foo>::Pointer> object_ptrs;
	for (int i = 0; i < max_capacity + 1; i++)
		object_ptrs.emplace_back(object_pool.new_object(i, 'a'));

	for (int i = 0; i < max_capacity; i++)
		EXPECT_NE(object_ptrs[i].get(), nullptr);
	
	EXPECT_EQ(object_ptrs.back().get(), nullptr);
}

TEST(object_pool, free_object_automatically) {
	const int max_capacity = 3;
	win::ObjectGlobalPool<Foo> object_pool(max_capacity);

	{
		auto obj1 = object_pool.new_object(1, 'a');
		auto obj2 = object_pool.new_object(1, 'a');
		auto obj3 = object_pool.new_object(1, 'a');
	}

	auto obj1 = object_pool.new_object(1, 'a');
	auto obj2 = object_pool.new_object(1, 'a');
	auto obj3 = object_pool.new_object(1, 'a');

	EXPECT_TRUE(obj1);
	EXPECT_TRUE(obj2);
	EXPECT_TRUE(obj3);
}

TEST(object_pool, free_object_by_id) {
	win::ObjectGlobalPool<Foo> object_pool(3);

	auto obj1_id = object_pool.new_object_unsafe(1, 'a');
	auto obj2_id = object_pool.new_object_unsafe(1, 'a');
	auto obj3_id = object_pool.new_object_unsafe(1, 'a');

	EXPECT_TRUE(win::ObjectGlobalPool<Foo>::free_object(obj1_id));
	EXPECT_TRUE(win::ObjectGlobalPool<Foo>::free_object(obj2_id));
	EXPECT_TRUE(win::ObjectGlobalPool<Foo>::free_object(obj3_id));

	// check if objects deleted properly.
	auto obj4_id = object_pool.new_object_unsafe(1, 'a');
	EXPECT_TRUE(win::ObjectGlobalPool<Foo>::free_object(obj4_id));
}

TEST(object_pool, free_object_by_pointer) {
	win::ObjectPool<Foo> object_pool(3);

	auto obj1 = object_pool.new_object_raw(1, 'a');
	auto obj2 = object_pool.new_object_raw(1, 'a');
	auto obj3 = object_pool.new_object_raw(1, 'a');

	EXPECT_TRUE(object_pool.free_object(obj1));
	EXPECT_TRUE(object_pool.free_object(obj2));
	EXPECT_TRUE(object_pool.free_object(obj3));

	// check if objects deleted properly.
	auto obj4 = object_pool.new_object_raw(1, 'a');
	EXPECT_TRUE(object_pool.free_object(obj4));
}