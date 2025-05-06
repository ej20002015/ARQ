#pragma once

#include <TMQCore/data_entity.h>

namespace TMQ
{

struct MDEntity : public DataEntity
{
	bool _active = true;
	std::string instrumentID;
	std::string source;
	std::chrono::system_clock::time_point asofTs;
};

template<typename T>
concept c_MDEntity = std::is_base_of_v<MDEntity, T>;

template<c_MDEntity T>
class MDEntityTraits
{
public:
	static consteval std::string_view const type() { static_assert( false ); return ""; }
};

/*
* -------------- MKTDATA ENTITY DEFINITIONS --------------
*/

// FXRate

struct FXRate : public MDEntity
{
	double rate;
	double bid;
	double ask;
};

template<>
class MDEntityTraits<FXRate>
{
public:
	static consteval std::string_view const type() { return "FX"; }
};

// EQ

struct EQ : public MDEntity
{
	double price;
	double open;
	double close;
	double high;
	double low;
};

template<>
class MDEntityTraits<EQ>
{
public:
	static consteval std::string_view const type() { return "EQ"; }
};

}