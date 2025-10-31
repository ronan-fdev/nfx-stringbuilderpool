/**
 * @file StringBuilderPool.inl
 * @brief Inline method implementations for StringBuilderPool
 * @details High-performance inline implementations for string builder pooling infrastructure
 */

#include <iterator>
#include <utility>

namespace nfx::string
{
	//=====================================================================
	// StringBuilder class
	//=====================================================================

	//----------------------------------------------
	// Construction
	//----------------------------------------------

	inline StringBuilder::StringBuilder( DynamicStringBuffer& buffer )
		: m_buffer{ buffer }
	{
	}

	//----------------------------------------------
	// Array access operators
	//----------------------------------------------

	inline char& StringBuilder::operator[]( size_t index )
	{
		return m_buffer[index];
	}

	inline const char& StringBuilder::operator[]( size_t index ) const
	{
		return m_buffer[index];
	}

	//----------------------------------------------
	// String append operations
	//----------------------------------------------

	inline void StringBuilder::append( std::string_view str )
	{
		m_buffer.append( str );
	}

	inline void StringBuilder::append( const std::string& str )
	{
		m_buffer.append( str );
	}

	inline void StringBuilder::append( const char* str )
	{
		if ( str )
		{
			m_buffer.append( str );
		}
	}

	inline void StringBuilder::push_back( char c )
	{
		m_buffer.push_back( c );
	}

	//----------------------------------------------
	// Stream operators
	//----------------------------------------------

	inline StringBuilder& StringBuilder::operator<<( std::string_view str )
	{
		append( str );
		return *this;
	}

	inline StringBuilder& StringBuilder::operator<<( const std::string& str )
	{
		append( str );
		return *this;
	}

	inline StringBuilder& StringBuilder::operator<<( const char* str )
	{
		append( str );
		return *this;
	}

	inline StringBuilder& StringBuilder::operator<<( char c )
	{
		push_back( c );
		return *this;
	}

	//----------------------------------------------
	// Size and capacity management
	//----------------------------------------------

	inline size_t StringBuilder::length() const noexcept
	{
		return m_buffer.size();
	}

	inline void StringBuilder::resize( size_t newSize )
	{
		m_buffer.resize( newSize );
	}

	//----------------------------------------------
	// Iterator interface
	//----------------------------------------------

	inline StringBuilder::iterator StringBuilder::begin()
	{
		return m_buffer.data();
	}

	inline StringBuilder::const_iterator StringBuilder::begin() const
	{
		return m_buffer.data();
	}

	inline StringBuilder::iterator StringBuilder::end()
	{
		return m_buffer.end();
	}

	inline StringBuilder::const_iterator StringBuilder::end() const
	{
		return m_buffer.end();
	}

	//----------------------------------------------
	// StringBuilder::Enumerator class
	//----------------------------------------------

	//----------------------------
	// Construction
	//----------------------------

	inline StringBuilder::Enumerator::Enumerator( const StringBuilder& builder )
		: m_data{ builder.begin() },
		  m_end{ builder.end() },
		  m_current{ builder.begin() - 1 }
	{
	}

	//----------------------------
	// Enumerator operations
	//----------------------------

	inline bool StringBuilder::Enumerator::next()
	{
		const char* next_pos = std::next( m_current );
		if ( next_pos < m_end )
		{
			m_current = next_pos;
			return true;
		}
		return false;
	}

	inline char StringBuilder::Enumerator::current() const
	{
		return *m_current;
	}

	inline void StringBuilder::Enumerator::reset()
	{
		m_current = std::prev( m_data );
	}

	//=====================================================================
	// StringBuilderLease class
	//=====================================================================

	//----------------------------------------------
	// Construction
	//----------------------------------------------

	inline StringBuilderLease::StringBuilderLease( DynamicStringBuffer* buffer )
		: m_buffer{ buffer },
		  m_valid{ true }
	{
	}

	inline StringBuilderLease::StringBuilderLease( StringBuilderLease&& other ) noexcept
		: m_buffer{ std::exchange( other.m_buffer, nullptr ) },
		  m_valid{ std::exchange( other.m_valid, false ) }
	{
	}

	//----------------------------------------------
	// Assignment
	//----------------------------------------------

	inline StringBuilderLease& StringBuilderLease::operator=( StringBuilderLease&& other ) noexcept
	{
		if ( this != &other )
		{
			dispose();
			m_buffer = std::exchange( other.m_buffer, nullptr );
			m_valid = std::exchange( other.m_valid, false );
		}

		return *this;
	}

	//----------------------------------------------
	// Public interface implementations
	//----------------------------------------------

	inline StringBuilder StringBuilderLease::create()
	{
		if ( !m_valid )
		{
			throwInvalidOperation();
		}

		return StringBuilder{ *m_buffer };
	}

	inline DynamicStringBuffer& StringBuilderLease::buffer()
	{
		if ( !m_valid )
		{
			throwInvalidOperation();
		}

		return *m_buffer;
	}

	inline std::string StringBuilderLease::toString() const
	{
		if ( !m_valid )
		{
			throwInvalidOperation();
		}

		return m_buffer->toString();
	}
} // namespace nfx::string
