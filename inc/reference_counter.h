#pragma once

#include <stdexcept>
// TODO: all resources become PTCountedPointers
// TODO: eliminate resource manager (for now)
// TODO: move swapchain refs to call into render server (rather than args)
// TODO: reduce includes of render server with intermediate file (common.h), also copy typedefs here
// TODO: reimplement deduplication

template <class T>
struct PTCountedPointer
{
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
		return PTCountedPointer<C>((C*)underlying_pointer, reference_counter);
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
				delete underlying_pointer;
			delete reference_counter;
		}
	}
};
