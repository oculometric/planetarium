#pragma once

#include <stdexcept>
// TODO: reduce includes of render server with intermediate file (common.h), also copy typedefs here
// TODO: reimplement deduplication

template<class T, template<class...> class U>
inline constexpr bool is_instance_of_v = std::false_type{ };

template<template<class...> class U, class... Vs>
inline constexpr bool is_instance_of_v<U<Vs...>, U> = std::true_type{ };

template <class T>
struct PTCountedPointer
{
public:
	typedef T type;

private:
	T* underlying_pointer = nullptr;
	size_t* reference_counter = nullptr;

public:
	PTCountedPointer() : underlying_pointer(nullptr), reference_counter(new size_t(1)) { }
	PTCountedPointer(T* pointer) : underlying_pointer(pointer), reference_counter(new size_t(1)) { }
	inline PTCountedPointer(T* pointer, size_t* counter) : underlying_pointer(pointer), reference_counter(counter)
	{
		if (counter == nullptr)
			throw new std::runtime_error("a counted pointer may not be initialised with a null counter pointer");
	}
	inline PTCountedPointer(const PTCountedPointer& other) noexcept
	{
		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		if (other.underlying_pointer != nullptr)
			(*reference_counter)++;
	}
	inline PTCountedPointer(PTCountedPointer&& other) noexcept
	{
		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		other.reference_counter = nullptr;
		other.underlying_pointer = nullptr;
	}
	inline PTCountedPointer& operator=(const PTCountedPointer& other) noexcept
	{
		decrementCounter();

		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		if (other.underlying_pointer != nullptr)
			(*reference_counter)++;

		return *this;
	}
	inline PTCountedPointer& operator=(PTCountedPointer&& other) noexcept
	{
		decrementCounter();

		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		other.reference_counter = nullptr;
		other.underlying_pointer = nullptr;

		return *this;
	}

	template <class C>
	inline operator C()
	{
		static_assert((is_instance_of_v<C, PTCountedPointer>), "can only cast to a reference counter!");
		typename C::type* new_ptr = (typename C::type*)((void*)underlying_pointer);
		return PTCountedPointer<typename C::type>(new_ptr, reference_counter);
	}

	inline bool operator<(const PTCountedPointer& b) const
	{
		return getPointer() < b.getPointer();
	}

	inline T* operator->() const
	{
		return underlying_pointer;
	}
	inline bool operator==(T* other) const
	{
		return underlying_pointer == other;
	}
	inline bool operator==(const PTCountedPointer& other) const
	{
		return underlying_pointer == other.underlying_pointer;
	}

	inline size_t getCount() const
	{
		return *reference_counter;
	}
	inline T* getPointer() const
	{
		return underlying_pointer;
	}
	inline bool isValid() const
	{
		return underlying_pointer != nullptr;
	}

	inline ~PTCountedPointer()
	{
		decrementCounter();
	}

private:
	inline void decrementCounter()
	{
		if (reference_counter == nullptr)
			return;
		(*reference_counter)--;
		if (*reference_counter == 0)
		{
			if (underlying_pointer != nullptr)
			{
				auto name = typeid(T).name();
				debugLog("deleting counted object of type " + std::string(name));
				delete underlying_pointer;
			}
			delete reference_counter;
		}
	}
};
