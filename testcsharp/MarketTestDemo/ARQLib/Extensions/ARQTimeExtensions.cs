using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using ARQ = ARQLib.ARQ;


namespace ARQLib.Extensions
{
    public static class Extensions
    {
        private static readonly long UNIX_EPOCH_TICKS = new DateTime(1970, 1, 1, 0, 0, 0 ,0, 0 ).Ticks;

        public static DateTime ToDateTime(this ARQ.Time.Date date)
        {
            return new DateTime(date.ToDateOnly(), new TimeOnly( 0, 0, 0 ) );
        }

        public static DateTime ToDateTime(this ARQ.Time.DateTime dateTime)
        {
            return new DateTime(dateTime.date().ToDateOnly(), new TimeOnly(dateTime.hour().val(), dateTime.minute().val(), dateTime.second().val(), dateTime.millisecond().val(), dateTime.microsecond().val()));
        }

        public static DateOnly ToDateOnly(this ARQ.Time.Date date) 
        {
            return new DateOnly(date.year().val(), (int)date.month(), date.day().val());
        }

        public static ARQ.Time.DateTime ToARQDateTime(this DateTime dateTime)
        {
            long ticksDiff = dateTime.Ticks- UNIX_EPOCH_TICKS;
            long microsecondDiff = ticksDiff / 10; // DateTime ticks are tenths of Microseconds (i.e. 100s of nanoseconds)
            return new ARQ.Time.DateTime(new ARQ.Time.Microseconds(microsecondDiff));
        }

        public static ARQ.Time.Date ToARQDate(this DateOnly dateOnly)
        {
            return new ARQ.Time.Date(new ARQ.Time.Year(dateOnly.Year), (ARQ.Time.Month)dateOnly.Month, new ARQ.Time.Day(dateOnly.Day));
        }

    }
}
