using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using TMQ = TMQLib.TMQ;


namespace TMQLib.Extensions
{
    public static class Extensions
    {
        public static DateTime ToDateTime(this TMQ.Time.Date date)
        {
            return new DateTime(date.ToDateOnly(), new TimeOnly( 0, 0, 0 ) );
        }

        public static DateOnly ToDateOnly(this TMQ.Time.Date date) 
        {
            return new DateOnly(date.year().toInt(), (int)date.month(), date.day().toInt());
        }

        public static TMQ.Time.Date ToTMQDate(this DateTime dateTime)
        {
            return DateOnly.FromDateTime(dateTime).ToTMQDate();
        }

        public static TMQ.Time.Date ToTMQDate(this DateOnly dateOnly)
        {
            return new TMQ.Time.Date(new TMQ.Time.Year(dateOnly.Year), (TMQ.Time.Month)dateOnly.Month, new TMQ.Time.Day(dateOnly.Day));
        }

    }
}
