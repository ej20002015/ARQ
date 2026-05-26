#pragma once

%{
#include <ARQUtils/time.h>
#include <limits>
%}

%include <stdint.i>

namespace ARQ
{
namespace Time
{

/*
* ----------- Date class -----------
*/

// TYPEMAP: C# System.DateOnly? <-> C++ ARQ::Time::Date
%typemap(cscode) ARQ::Time::Date %{

    // --- C++ to C# ---
    public global::System.DateOnly? ToDateOnly()
    {
        int ymd = this.getYMD();
        if (ymd == 0)
            return null;
        else
            return new global::System.DateOnly(ymd / 10000, (ymd / 100) % 100, ymd % 100);
    }

    public static implicit operator global::System.DateOnly?(Date d)
    {
        return d?.ToDateOnly();
    }

    // --- C# to C++ ---
    public static implicit operator Date(global::System.DateOnly? d)
    {
        if (!d.HasValue)
            return Date.fromYMD(0);
        else
            return Date.fromYMD(d.Value.Year * 10000 + d.Value.Month * 100 + d.Value.Day);
    }

    public static Date FromDateOnly(global::System.DateOnly? d) => d;

    public override string ToString()
    {
        var d = this.ToDateOnly();
        return d.HasValue ? d.Value.ToString("O") : "null";
    }
%}

class Date
{
public:
    Date() = default;

    %extend {
        int32_t getYMD() const
        {
            if( !$self->isSet() )
                return 0;
            else
                return $self->ymdInt().val(); 
        }

        static Date fromYMD( int32_t ymd )
        {
            if( ymd == 0 )
                return ARQ::Time::Date();
            else
                return ARQ::Time::Date( ARQ::Time::YYYYMMDDInt( ymd ) );
        }
    }

};

/*
* ----------- DateTime class -----------
*/

// TYPEMAP: C# System.DateTime <-> C++ ARQ::Time::DateTime
%typemap(cscode) ARQ::Time::DateTime %{
    private const long NULL_SENTINEL = long.MinValue;

    // --- C++ to C# ---
    public global::System.DateTime? ToDateTime()
    {
        long us = this.getUnixMicroseconds();
        if (us == NULL_SENTINEL)
            return null;

        // C# Ticks are 100ns so 1 Microsecond = 10 Ticks
        return global::System.DateTime.UnixEpoch.AddTicks(us * 10);
    }

    public static implicit operator global::System.DateTime?(DateTime dt)
    {
        return dt?.ToDateTime();
    }

    // --- C# to C++ ---
    public static implicit operator DateTime(global::System.DateTime? dt)
    {
        if (!dt.HasValue)
            return DateTime.fromUnixMicroseconds( NULL_SENTINEL );
        
        // Convert to UTC, get Ticks since Epoch, and divide by 10 to get Microseconds
        long ticks = (dt.Value.ToUniversalTime() - global::System.DateTime.UnixEpoch).Ticks;
        return DateTime.fromUnixMicroseconds(ticks / 10);
    }

    public static DateTime FromDateTime(global::System.DateTime? dt) => dt;
    
    public override string ToString()
    {
        var dt = this.ToDateTime();
        return dt.HasValue ? dt.Value.ToString("O") : "null";
    }
%}

class DateTime 
{
public:
    DateTime();

    %extend {
        int64_t getUnixMicroseconds() const
        {
            if( !$self->isSet() )
                return std::numeric_limits<int64_t>::min();
            else
                return $self->microsecondsSinceEpoch().val();
        }

        static DateTime fromUnixMicroseconds(int64_t us) {
            if( us == std::numeric_limits<int64_t>::min() )
                return ARQ::Time::DateTime();
            else
                return ARQ::Time::DateTime( ARQ::Time::Microseconds( us ) );
        }
    }
};

}
}