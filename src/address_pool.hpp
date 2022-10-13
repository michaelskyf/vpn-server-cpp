// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stddef.h>
#include <stdexcept>

template <typename T>
class address_pool
{
public:
	address_pool(size_t size = 0): size_m{size}
	{
		bitfield_m = new int[size_m / (sizeof(int) * 8) + 1]{};
		data_m = new T[size_m];
	}

	~address_pool()
	{
		delete[] bitfield_m;
		delete[] data_m;
	}

	address_pool(const address_pool&) = delete;
	address_pool(address_pool&& other) noexcept: size_m{other.size_m}, bitfield_m{other.bitfield_m}, data_m{other.data_m} 
	{
		other.size_m = 0;
		other.bitfield_m = nullptr;
		other.data_m = nullptr;
	}

	address_pool& operator= (address_pool&& other) noexcept
	{
		delete[] bitfield_m;
		delete[] data_m;
		
		size_m = other.size_m;
		bitfield_m = other.bitfield_m;
		data_m = other.data_m;

		other.size_m = 0;
		other.bitfield_m = nullptr;
		other.data_m = nullptr;

		return *this;
	}

	void add(size_t index, T&& val)
	{
		if(index >= size_m)
			throw std::runtime_error("Index exceeds maximum size");

		bitfield_m[index/(sizeof(int)*8)] |= 1 << index%(sizeof(int)*8);

		data_m[index] = std::move(val);
	}

	size_t get_free_index() const
	{
		for(size_t i = 0; i < size_m; i++)
		{
			if(bitfield_m[i] != -1)
			{
				int found = bitfield_m[i];
				for(unsigned int bit = 0; bit < sizeof(int) * 8; bit++)
				{
					if((found & 1) == 0)
						return i * sizeof(int) * 8 + bit;

					found = found >> 1;
				}
			}
		}

		throw std::runtime_error("No indices left");
	}

	size_t find(T& val) const
	{
		for(size_t i = 0; i < size_m; i++)
		{
			if((bitfield_m[i/(sizeof(int)*8)] >> i%(sizeof(int)*8) & 1) == 0)
				continue;

			if(val == data_m[i])
				return i;
		}

		throw std::runtime_error("Not found");
	}

	T& get(size_t index) const
	{
		if(index >= size_m)
			throw std::runtime_error("Index exceeds maximum size");

		if((bitfield_m[index/(sizeof(int)*8)] >> index%(sizeof(int)*8) & 1) == 0)
			throw std::runtime_error("Not set");

		return data_m[index];
	}

	void del(size_t index)
	{
		if(index >= size_m)
			throw std::runtime_error("Index exceeds maximum size");

		bitfield_m[index/(sizeof(int)*8)] &= ~(1 << index%(sizeof(int)*8));
		data_m[index] = {};
	}

	size_t size() const noexcept
	{
		return size_m;
	}

private:
	size_t size_m;
	int* bitfield_m = nullptr;
	T* data_m = nullptr;
};