#include <ARQSerialisation/ser_mktdata_entities.h>

#include <ARQUtils/buffer.h>
#include <ARQUtils/time.h>

#include <flatbuffers/flatbuffers.h>

//#include <fbs_generated/fxrate.h>
//#include <fbs_generated/eq.h>
//
//namespace ARQ
//{
//
///*
//* --------- FXRate ---------
//*/
//
//Buffer serialise( const FXRate& fxrate )
//{
//	flatbuffers::FlatBufferBuilder builder;
//
//	const auto fbUser = fbs::CreateFXRateDirect(
//		builder,
//		fxrate.instrumentID.c_str(),
//		fxrate.source.c_str(),
//		Time::tpToLong( fxrate.asofTs ),
//		fxrate.rate,
//		fxrate.bid,
//		fxrate.ask
//	);
//
//	builder.Finish( fbUser );
//	const uint8_t* const buffer = builder.GetBufferPointer();
//	const size_t size = builder.GetSize();
//	/*size_t _1, _2;
//	builder.ReleaseRaw( _1, _2 );*/
//	return Buffer( buffer, size ); // TODO: Try converting buffer to a std::unique_ptr so we can steal memory
//}
//
//template<>
//FXRate deserialise( const BufferView buffer )
//{
//	const uint8_t* const bufPtr = reinterpret_cast<const uint8_t*>( buffer.data );
//	const fbs::FXRate* const fbFXRate = fbs::GetFXRate( bufPtr );
//
//	FXRate fxRate = {
//		std::chrono::system_clock::time_point(),
//		"",
//		false,
//		fbFXRate->instrument_id()->c_str(),
//		fbFXRate->source()->c_str(),
//		Time::longToTp( fbFXRate->asof_ts() ),
//		fbFXRate->rate(),
//		fbFXRate->bid(),
//		fbFXRate->ask(),
//	};
//
//	return fxRate;
//}
//
//template ARQSerialisation_API FXRate deserialise( const BufferView buffer );
//
///*
//* --------- EQ ---------
//*/
//
//Buffer serialise( const EQ& eq )
//{
//	flatbuffers::FlatBufferBuilder builder;
//
//	const auto fbUser = fbs::CreateEQDirect(
//		builder,
//		eq.instrumentID.c_str(),
//		eq.source.c_str(),
//		Time::tpToLong( eq.asofTs ),
//		eq.price,
//		eq.open,
//		eq.close,
//		eq.high,
//		eq.low
//	);
//
//	builder.Finish( fbUser );
//	const uint8_t* const buffer = builder.GetBufferPointer();
//	const size_t size = builder.GetSize();
//	/*size_t _1, _2;
//	builder.ReleaseRaw( _1, _2 );*/
//	return Buffer( buffer, size ); // TODO: Try converting buffer to a std::unique_ptr so we can steal memory
//}
//
//template<>
//EQ deserialise( const BufferView buffer )
//{
//	const uint8_t* const bufPtr = reinterpret_cast<const uint8_t*>( buffer.data );
//	const fbs::EQ* const fbEq = fbs::GetEQ( bufPtr );
//
//	EQ eq = {
//		std::chrono::system_clock::time_point(),
//		"",
//		false,
//		fbEq->instrument_id()->c_str(),
//		fbEq->source()->c_str(),
//		Time::longToTp( fbEq->asof_ts() ),
//		fbEq->price(),
//		fbEq->open(),
//		fbEq->close(),
//		fbEq->high(),
//		fbEq->low()
//	};
//
//	return eq;
//}
//
//template ARQSerialisation_API EQ deserialise( const BufferView buffer );
//
//}
