#include <TMQMarket/market.h>

#include <TMQUtils/core.h>
#include <TMQUtils/error.h>
#include <TMQSerialisation/ser_mktdata_entities.h>

namespace TMQ
{

namespace Mkt
{

Context Context::LIVE = { "LIVE", std::nullopt };

std::string Context::str() const
{
    return tag + ( date.has_value() ? std::format( "|{:%Y%m%d}", date.value() ) : "" );
}

size_t MarketSnapshot::size() const noexcept
{
	return m_fxRates.size() + m_equities.size();
}

MarketSnapshot Market::snap() const
{
	std::unique_lock<std::shared_mutex> ul( m_mut );

	MarketSnapshot snapshot;

	snapshot.m_fxRates  = m_fxRates.objMap();
	snapshot.m_equities = m_equities.objMap();

	return snapshot;
}

template<c_MDEntity T>
void loadIntoMkt( Market& out_mkt, const MktDataSource::FetchData& resRow )
{
	T obj = deserialise<T>( resRow.blob );
	obj._lastUpdatedBy = resRow.lastUpdatedBy;
	obj._lastUpdatedTs = resRow.lastUpdatedTs;
	obj._active = true; // If returned from fetch then must be an active record

	out_mkt.set( obj );
}

std::shared_ptr<Market> load( const Context& ctx, const std::shared_ptr<MktDataSource>& source )
{
    std::shared_ptr<MktDataSource> mktDataSrc = source ? source : GlobalMktDataSource::get();

    const std::vector<MktDataSource::FetchData> fetchRes = mktDataSrc->fetchLatest( ctx.str() );

	auto mkt = std::make_shared<Market>();
	for( const auto& resRow : fetchRes )
	{
		// TODO: Improve this!

		if( resRow.type == "FX" )
			loadIntoMkt<FXRate>( *mkt, resRow );
		else if( resRow.type == "EQ" )
			loadIntoMkt<EQ>( *mkt, resRow );
		else
			throw TMQException( std::format( "Don't know how to load MDEntities of type {}", resRow.type ) );
	}

	return mkt;
}

template<c_MDEntity T>
void emplaceObjs( const MarketSnapshot& mktSnapshot, std::vector<MktDataSource::InsertData>& out_insData )
{
	for( const auto [iid, obj] : mktSnapshot.getAllObjs<T>() )
	{
		out_insData.emplace_back( MktDataSource::InsertData{
			.type          = MDEntityTraits<T>::type().data(),
			.instrumentID  = iid,
			.asofTs        = obj.asofTs,
			.blob          = TMQ::serialise( obj ),
			.source        = obj.source,
			.lastUpdatedBy = "Evan", // TODO: Change to username
			.active        = obj._active
		} );
	}
}

void save( const std::shared_ptr<Market> mkt, const Context& ctx, const std::shared_ptr<MktDataSource>& source )
{
	save( mkt->snap(), ctx, source );
}

void save( const MarketSnapshot& mktSnapshot, const Context& ctx, const std::shared_ptr<MktDataSource>& source )
{
	std::shared_ptr<MktDataSource> mktDataSrc = source ? source : GlobalMktDataSource::get();

	std::vector<MktDataSource::InsertData> insData;
	insData.reserve( mktSnapshot.size() );

	emplaceObjs<FXRate>( mktSnapshot, insData );
	emplaceObjs<EQ>( mktSnapshot, insData );

	mktDataSrc->insert( ctx.str(), insData ); // TODO: Take context from mkt if specified?
}

}

}