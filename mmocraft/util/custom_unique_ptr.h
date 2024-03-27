#pragma once
#include "noncopyable.h"

namespace util
{
	template <typename T>
	class UniquePtrEmptyDeleter : util::NonCopyable
	{
	public:
		UniquePtrEmptyDeleter(T* ptr) noexcept
			: m_ptr(ptr)
		{ }

		UniquePtrEmptyDeleter(UniquePtrEmptyDeleter&& other) noexcept
		{
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}

		UniquePtrEmptyDeleter& operator=(UniquePtrEmptyDeleter&& other) noexcept
		{
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}

		const T* operator->() const
		{
			return m_ptr;
		}

		T* operator->()
		{
			return m_ptr;
		}

		const T& operator*() const
		{
			return *m_ptr;
		}

		T& operator*()
		{
			return *m_ptr;
		}

		const T* get() const
		{
			return m_ptr;
		}

		T* get()
		{
			return m_ptr;
		}

	private:
		T* m_ptr{ nullptr };
	};
}