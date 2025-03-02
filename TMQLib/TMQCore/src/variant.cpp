#include "TMQCore/variant.h"

namespace TMQ
{
	VariantArr::VariantArr( const uint32_t rows, const uint32_t cols /*= 1*/ )
		: m_nRows( rows )
		, m_nCols( cols )
	{
		m_data = std::make_unique<Variant[]>( m_nRows * m_nCols );
	}

	VariantArr::VariantArr( const VariantArr& other )
		: m_nRows( other.m_nRows )
		, m_nCols( other.m_nCols )
	{
		m_data = std::make_unique<Variant[]>( m_nRows * m_nCols );
		std::copy( other.m_data.get(), other.m_data.get() + m_nRows * m_nCols, m_data.get() );
	}

	VariantArr& VariantArr::operator=( const VariantArr& other )
	{
		if( this != &other )
		{
			m_nRows = other.m_nRows;
			m_nCols = other.m_nCols;
			m_data = std::make_unique<Variant[]>( m_nRows * m_nCols );
			std::copy( other.m_data.get(), other.m_data.get() + m_nRows * m_nCols, m_data.get() );
		}
		return *this;
	}

	VariantArr::VariantArr( VariantArr&& other ) noexcept
		: m_nRows( other.m_nRows )
		, m_nCols( other.m_nCols )
		, m_data( std::move( other.m_data ) )
	{
		other.m_nRows = 0;
		other.m_nCols = 0;
	}

	VariantArr& VariantArr::operator=( VariantArr&& other ) noexcept
	{
		if( this != &other )
		{
			m_nRows = other.m_nRows;
			m_nCols = other.m_nCols;
			m_data = std::move( other.m_data );
			other.m_nRows = 0;
			other.m_nCols = 0;
		}
		return *this;
	}
}