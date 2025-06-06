#pragma once

// TODO: all resources become PTCountedPointers
// TODO: eliminate resource manager (for now)

template <class T>
struct PTCountedPointer
{
private:
	T* underlying_pointer = nullptr;
	size_t* reference_counter = nullptr;

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

public:
	PTCountedPointer() : underlying_pointer(nullptr), reference_counter(new size_t(1))
	{
	}
	PTCountedPointer(T* pointer) : underlying_pointer(pointer), reference_counter(new size_t(1)) { }
	inline PTCountedPointer(const PTCountedPointer& other)
	{
		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		if (other.underlying_pointer != nullptr)
			(*reference_counter)++;
	}
	inline PTCountedPointer(PTCountedPointer&& other)
	{
		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		other.reference_counter = nullptr;
		other.underlying_pointer = nullptr;
	}
	inline PTCountedPointer& operator=(const PTCountedPointer& other)
	{
		decrementCounter();

		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		if (other.underlying_pointer != nullptr)
			(*reference_counter)++;

		return *this;
	}
	inline PTCountedPointer& operator=(PTCountedPointer&& other)
	{
		decrementCounter();

		reference_counter = other.reference_counter;
		underlying_pointer = other.underlying_pointer;

		other.reference_counter = nullptr;
		other.underlying_pointer = nullptr;

		return *this;
	}

	inline T* operator->() const
	{
		return underlying_pointer;
	}

	inline size_t getCount() const
	{
		return *reference_counter;
	}
	inline T* getPointer() const
	{
		return underlying_pointer;
	}

	inline ~PTCountedPointer()
	{
		decrementCounter();
	}
};
