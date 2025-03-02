#include <gtest/gtest.h>

#include <TMQMarket/market.h>

using namespace TMQ;

class TestMarketListener : public Market::Listener
{
public:
	void onMktUpdate( const MktType::Type type, const std::string_view id ) override
	{
		m_lastMktTypeUpdated = type;
		m_lastId = id;
	}

	MktType::Type getLastMktTypeUpdated() const
	{
		return m_lastMktTypeUpdated;
	}

	const std::string& getLastId() const
	{
		return m_lastId;
	}

private:
	MktType::Type m_lastMktTypeUpdated;
	std::string m_lastId;
};

// Test Default Constructor
TEST( MarketTests, TestListeners )
{
    Market mkt;
	TestMarketListener listener1, listener2;
	mkt.registerListener( &listener1 );
	mkt.registerListener( &listener2 );

	mkt.setEquity( "AAPL", MktType::Equity( 1, 2, 3, 4, 5 ) );
	EXPECT_EQ( listener1.getLastMktTypeUpdated(), MktType::EQUITY );
	EXPECT_EQ( listener1.getLastId(), "AAPL" );
	EXPECT_EQ( listener2.getLastMktTypeUpdated(), MktType::EQUITY );
	EXPECT_EQ( listener2.getLastId(), "AAPL" );

	mkt.setFxPair( "EURUSD", MktType::FXPair( 1.25, 1.2, 1.3 ) );
	EXPECT_EQ( listener1.getLastMktTypeUpdated(), MktType::FX_PAIR );
	EXPECT_EQ( listener1.getLastId(), "EURUSD" );
	EXPECT_EQ( listener2.getLastMktTypeUpdated(), MktType::FX_PAIR );
	EXPECT_EQ( listener2.getLastId(), "EURUSD" );
}