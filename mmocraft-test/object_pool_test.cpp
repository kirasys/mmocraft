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

TEST(ObjectPoolTest, Reserve_Success) {
	const int max_capacity = 1000;
	win::ObjectPool<Foo> object_pool(max_capacity);

	bool success = object_pool.reserve(max_capacity);

	EXPECT_TRUE(success);
}
TEST(ObjectPoolTest, Reserve_Success_Max) {
	const int max_capacity = 1000;
	win::ObjectPool<Foo> object_pool(max_capacity);

	object_pool.reserve(max_capacity);

	EXPECT_EQ(object_pool.capacity(), max_capacity);
}

TEST(ObjectPoolTest, Reserve_Fail_Exceed) {
	const int max_capacity = 1000;
	win::ObjectPool<Foo> object_pool(max_capacity);

	object_pool.reserve(max_capacity);
	bool success = object_pool.reserve(max_capacity + 1);

	EXPECT_FALSE(success);
}

TEST(ObjectPoolTest, New_Object_Correctly) {
	win::ObjectPool<Foo> object_pool(1000);

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

TEST(ObjectPoolTest, New_Object_To_Max) {
	const int max_capacity = 10;
	win::ObjectPool<Foo> object_pool(max_capacity);
	
	std::vector<win::ObjectPool<Foo>::Pointer> object_ptrs;
	for (int i = 0; i < max_capacity; i++)
		object_ptrs.emplace_back(object_pool.new_object(i, 'a'));

	for (int i = 0; i < max_capacity; i++) {
		EXPECT_NE(object_ptrs[i].get(), nullptr);
	}
}

TEST(ObjectPoolTest, New_Object_Exceed) {
	const int max_capacity = 10;
	win::ObjectPool<Foo> object_pool(max_capacity);

	std::vector<win::ObjectPool<Foo>::Pointer> object_ptrs;
	for (int i = 0; i < max_capacity + 1; i++)
		object_ptrs.emplace_back(object_pool.new_object(i, 'a'));

	for (int i = 0; i < max_capacity; i++)
		EXPECT_NE(object_ptrs[i].get(), nullptr);
	
	EXPECT_EQ(object_ptrs.back().get(), nullptr);
}

TEST(ObjectPoolTest, New_Object_Unsafe_Delete_Correctly) {
	win::ObjectPool<Foo> object_pool(1000);

	auto obj1_id = object_pool.new_object_unsafe(1, 'a');
	auto obj2_id = object_pool.new_object_unsafe(1, 'a');
	auto obj3_id = object_pool.new_object_unsafe(1, 'a');

	EXPECT_TRUE(win::ObjectPool<Foo>::free_object(obj1_id));
	EXPECT_TRUE(win::ObjectPool<Foo>::free_object(obj2_id));
	EXPECT_TRUE(win::ObjectPool<Foo>::free_object(obj3_id));
}

TEST(ObjectPoolTest, New_Object_Raw_Delete_Correctly) {
	win::ObjectPool<Foo> object_pool(1000);

	auto obj1 = object_pool.new_object_raw(1, 'a');
	auto obj2 = object_pool.new_object_raw(1, 'a');
	auto obj3 = object_pool.new_object_raw(1, 'a');

	EXPECT_TRUE(object_pool.free_object(obj1));
	EXPECT_TRUE(object_pool.free_object(obj2));
	EXPECT_TRUE(object_pool.free_object(obj3));
}