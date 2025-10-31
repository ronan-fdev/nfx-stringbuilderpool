/*
 * MIT License
 *
 * Copyright (c) 2025 nfx
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file StringBuilderPool.cpp
 * @brief Implementation file for StringBuilderPool methods
 */

#include <algorithm>
#include <cstring>

#include "nfx/string/StringBuilderPool.h"
#include "DynamicStringBufferPool.h"

namespace nfx::string
{
	//=====================================================================
	// DynamicStringBuffer class
	//=====================================================================

	//----------------------------------------------
	// Construction
	//----------------------------------------------

	DynamicStringBuffer::DynamicStringBuffer()
		: m_heapBuffer{ nullptr },
		  m_size{ 0 },
		  m_capacity{ STACK_BUFFER_SIZE },
		  m_onHeap{ false }
	{
	}

	DynamicStringBuffer::DynamicStringBuffer( size_t initialCapacity )
		: m_heapBuffer{ nullptr },
		  m_size{ 0 }, m_capacity{ STACK_BUFFER_SIZE },
		  m_onHeap{ false }
	{
		if ( initialCapacity > STACK_BUFFER_SIZE )
		{
			m_heapBuffer = std::make_unique<char[]>( initialCapacity );
			m_capacity = initialCapacity;
			m_onHeap = true;
		}
	}

	DynamicStringBuffer::DynamicStringBuffer( const DynamicStringBuffer& other )
		: m_heapBuffer{ nullptr },
		  m_size{ other.m_size },
		  m_capacity{ other.m_capacity },
		  m_onHeap{ other.m_onHeap }
	{
		if ( m_onHeap )
		{
			m_heapBuffer = std::make_unique<char[]>( m_capacity );
			std::memcpy( m_heapBuffer.get(), other.m_heapBuffer.get(), m_size );
		}
		else
		{
			std::memcpy( m_stackBuffer, other.m_stackBuffer, m_size );
		}
	}

	DynamicStringBuffer::DynamicStringBuffer( DynamicStringBuffer&& other ) noexcept
		: m_heapBuffer{ std::move( other.m_heapBuffer ) },
		  m_size{ other.m_size },
		  m_capacity{ other.m_capacity },
		  m_onHeap{ other.m_onHeap }
	{
		if ( !m_onHeap )
		{
			std::memcpy( m_stackBuffer, other.m_stackBuffer, m_size );
		}

		other.m_size = 0;
		other.m_capacity = STACK_BUFFER_SIZE;
		other.m_onHeap = false;
	}

	//----------------------------------------------
	// Assignment
	//----------------------------------------------

	DynamicStringBuffer& DynamicStringBuffer::operator=( const DynamicStringBuffer& other )
	{
		if ( this != &other )
		{
			if ( other.m_onHeap )
			{
				// Other uses heap, we need heap too
				if ( !m_onHeap || m_capacity < other.m_capacity )
				{
					m_heapBuffer = std::make_unique<char[]>( other.m_capacity );
					m_capacity = other.m_capacity;
					m_onHeap = true;
				}
				std::memcpy( m_heapBuffer.get(), other.m_heapBuffer.get(), other.m_size );
			}
			else
			{
				// Other uses stack, we can use stack too
				if ( m_onHeap )
				{
					m_heapBuffer.reset();
					m_capacity = STACK_BUFFER_SIZE;
					m_onHeap = false;
				}
				std::memcpy( m_stackBuffer, other.m_stackBuffer, other.m_size );
			}
			m_size = other.m_size;
		}
		return *this;
	}

	DynamicStringBuffer& DynamicStringBuffer::operator=( DynamicStringBuffer&& other ) noexcept
	{
		if ( this != &other )
		{
			m_heapBuffer = std::move( other.m_heapBuffer );
			m_size = other.m_size;
			m_capacity = other.m_capacity;
			m_onHeap = other.m_onHeap;

			if ( !m_onHeap )
			{
				std::memcpy( m_stackBuffer, other.m_stackBuffer, m_size );
			}

			other.m_size = 0;
			other.m_capacity = STACK_BUFFER_SIZE;
			other.m_onHeap = false;
		}
		return *this;
	}

	//----------------------------------------------
	// Capacity and size information
	//----------------------------------------------

	size_t DynamicStringBuffer::size() const noexcept
	{
		return m_size;
	}

	size_t DynamicStringBuffer::capacity() const noexcept
	{
		return m_capacity;
	}

	bool DynamicStringBuffer::isEmpty() const noexcept
	{
		return m_size == 0;
	}

	//----------------------------------------------
	// Buffer management
	//----------------------------------------------

	void DynamicStringBuffer::clear() noexcept
	{
		m_size = 0;
	}

	void DynamicStringBuffer::reserve( size_t newCapacity )
	{
		if ( newCapacity > m_capacity )
		{
			ensureCapacity( newCapacity );
		}
	}

	void DynamicStringBuffer::resize( size_t newSize )
	{
		ensureCapacity( newSize );
		m_size = newSize;
	}

	//----------------------------------------------
	// Data access
	//----------------------------------------------

	char* DynamicStringBuffer::data() noexcept
	{
		return currentBuffer();
	}

	const char* DynamicStringBuffer::data() const noexcept
	{
		return currentBuffer();
	}

	char& DynamicStringBuffer::operator[]( size_t index )
	{
		return currentBuffer()[index];
	}

	const char& DynamicStringBuffer::operator[]( size_t index ) const
	{
		return currentBuffer()[index];
	}

	//----------------------------------------------
	// Content manipulation
	//----------------------------------------------

	void DynamicStringBuffer::append( std::string_view str )
	{
		if ( !str.empty() )
		{
			const size_t new_size = m_size + str.size();
			ensureCapacity( new_size );
			std::memcpy( currentBuffer() + m_size, str.data(), str.size() );
			m_size = new_size;
		}
	}

	void DynamicStringBuffer::append( const std::string& str )
	{
		append( std::string_view{ str } );
	}

	void DynamicStringBuffer::append( const char* str )
	{
		if ( str )
		{
			append( std::string_view{ str, std::strlen( str ) } );
		}
	}

	void DynamicStringBuffer::push_back( char c )
	{
		ensureCapacity( m_size + 1 );
		currentBuffer()[m_size] = c;
		++m_size;
	}

	//----------------------------------------------
	// String conversion
	//----------------------------------------------

	std::string DynamicStringBuffer::toString() const
	{
		return std::string( currentBuffer(), m_size );
	}

	std::string_view DynamicStringBuffer::toStringView() const noexcept
	{
		return std::string_view{ currentBuffer(), m_size };
	}

	//----------------------------------------------
	// Iterator interface
	//----------------------------------------------

	DynamicStringBuffer::iterator DynamicStringBuffer::begin() noexcept
	{
		return currentBuffer();
	}

	DynamicStringBuffer::const_iterator DynamicStringBuffer::begin() const noexcept
	{
		return currentBuffer();
	}

	DynamicStringBuffer::iterator DynamicStringBuffer::end() noexcept
	{
		return currentBuffer() + m_size;
	}

	DynamicStringBuffer::const_iterator DynamicStringBuffer::end() const noexcept
	{
		return currentBuffer() + m_size;
	}

	//----------------------------------------------
	// Private methods
	//----------------------------------------------

	void DynamicStringBuffer::ensureCapacity( size_t needed_capacity )
	{
		if ( needed_capacity <= m_capacity )
		{
			return;
		}

		// Calculate new capacity with growth factor
		size_t new_capacity = std::max( needed_capacity,
			static_cast<size_t>( m_capacity * GROWTH_FACTOR ) );

		if ( !m_onHeap && new_capacity <= STACK_BUFFER_SIZE )
		{
			return;
		}

		if ( !m_onHeap )
		{
			// Transition from stack to heap
			m_heapBuffer = std::make_unique<char[]>( new_capacity );
			if ( m_size > 0 )
			{
				std::memcpy( m_heapBuffer.get(), m_stackBuffer, m_size );
			}
			m_onHeap = true;
			m_capacity = new_capacity;
		}
		else
		{
			// Expand existing heap buffer
			auto new_buffer = std::make_unique<char[]>( new_capacity );
			if ( m_size > 0 )
			{
				std::memcpy( new_buffer.get(), m_heapBuffer.get(), m_size );
			}
			m_heapBuffer = std::move( new_buffer );
			m_capacity = new_capacity;
		}
	}

	char* DynamicStringBuffer::currentBuffer() noexcept
	{
		return m_onHeap ? m_heapBuffer.get() : m_stackBuffer;
	}

	const char* DynamicStringBuffer::currentBuffer() const noexcept
	{
		return m_onHeap ? m_heapBuffer.get() : m_stackBuffer;
	}

	//=====================================================================
	// StringBuilderLease class
	//=====================================================================

	//----------------------------------------------
	// Destruction
	//----------------------------------------------

	StringBuilderLease::~StringBuilderLease()
	{
		dispose();
	}

	//----------------------------------------------
	// Private implementation methods
	//----------------------------------------------

	void StringBuilderLease::dispose()
	{
		if ( m_valid )
		{
			dynamicStringBufferPool().returnToPool( m_buffer );
			m_buffer = nullptr;
			m_valid = false;
		}
	}

	void StringBuilderLease::throwInvalidOperation() const
	{
		throw std::runtime_error{ "Tried to access StringBuilder after it was returned to pool" };
	}

	//=====================================================================
	// StringBuilderPool class
	//=====================================================================

	//----------------------------------------------
	// Static factory methods
	//----------------------------------------------

	StringBuilderLease StringBuilderPool::lease()
	{
		return StringBuilderLease( dynamicStringBufferPool().get() );
	}

	//----------------------------
	// Statistics methods
	//----------------------------

	StringBuilderPool::PoolStatistics StringBuilderPool::stats() noexcept
	{
		const auto& internalStats = dynamicStringBufferPool().stats();
		return PoolStatistics{
			.threadLocalHits = internalStats.threadLocalHits.load(),
			.dynamicStringBufferPoolHits = internalStats.dynamicStringBufferPoolHits.load(),
			.newAllocations = internalStats.newAllocations.load(),
			.totalRequests = internalStats.totalRequests.load(),
			.hitRate = internalStats.hitRate() };
	}

	void StringBuilderPool::resetStats() noexcept
	{
		dynamicStringBufferPool().resetStats();
	}

	//----------------------------
	// Lease management
	//----------------------------

	size_t StringBuilderPool::clear()
	{
		dynamicStringBufferPool().resetStats();

		return dynamicStringBufferPool().clear();
	}

	size_t StringBuilderPool::size() noexcept
	{
		return dynamicStringBufferPool().size();
	}
} // namespace nfx::string
