#pragma once

#include <chrono>

namespace TMQ
{
namespace MktType
{

struct Base
{
	std::string lastUpdatedBy;
	std::chrono::system_clock::time_point lastUpdatedTm;

protected:
	Base()
		: lastUpdatedTm( std::chrono::system_clock::now() )
	{
	}

	Base( const std::string& u )
		: lastUpdatedBy( u ), lastUpdatedTm( std::chrono::system_clock::now() )
	{
	}
};

template<typename T>
concept c_MktType = std::is_base_of_v<Base, T>;

enum Type
{
	NONE,
	FX_PAIR,
	EQUITY
};

struct FXPair : public Base
{
    double rate;
    double bid;
    double ask;

	FXPair() = default;
    FXPair( const double r, const double b, const double a )
        : Base()
		, rate( r ), bid( b ), ask( a )
    {
    }
};

struct Equity : public Base
{
	double price;
	double open;
	double close;
	double high;
	double low;

	Equity() = default;
	Equity( const double p, const double o, const double c, const double h, const double l )
		: Base()
		, price( p ), open( o ), close( c ), high( h ), low( l )
	{
	}
};

}
}