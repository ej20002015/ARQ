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
        public static DateTime ToDateTime(this ARQ.Time.Date date)
        {
            return new DateTime(date.ToDateOnly(), new TimeOnly( 0, 0, 0 ) );
        }

        public static DateOnly ToDateOnly(this ARQ.Time.Date date) 
        {
            return new DateOnly(date.year().toInt(), (int)date.month(), date.day().toInt());
        }

        public static ARQ.Time.Date ToARQDate(this DateTime dateTime)
        {
            return DateOnly.FromDateTime(dateTime).ToARQDate();
        }

        public static ARQ.Time.Date ToARQDate(this DateOnly dateOnly)
        {
            return new ARQ.Time.Date(new ARQ.Time.Year(dateOnly.Year), (ARQ.Time.Month)dateOnly.Month, new ARQ.Time.Day(dateOnly.Day));
        }

    }
}
