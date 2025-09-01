using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ARQ = ARQLib.ARQ;

namespace ARQLib.Extensions
{
    public static class MDEntitiesExtensions
    {
        public static ARQ.MDEntities.FXRate Clone(this ARQ.MDEntities.FXRate original)
        {
            if (original == null)
                throw new ArgumentNullException(nameof(original));
            // Create a new FXRate instance and copy properties
            var clone = new ARQ.MDEntities.FXRate()
            {
                ID = original.ID,
                bid = original.bid,
                ask = original.ask,
                mid = original.mid,
                asofTs = original.asofTs,
                source = original.source,
                _isActive = original._isActive,
                _lastUpdatedBy = original._lastUpdatedBy,
                _lastUpdatedTs = original._lastUpdatedTs
            };
            return clone;
        }
    }
}
