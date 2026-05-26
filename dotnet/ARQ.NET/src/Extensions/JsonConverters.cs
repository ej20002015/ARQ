using System.ComponentModel;
using System.Text.Json.Serialization;

namespace ARQ.ID
{
    [JsonConverter(typeof(ARQ.Converters.UUIDConverter))]
    public partial class UUID { }
}

namespace ARQ.Time
{
    [JsonConverter(typeof(ARQ.Converters.DateTimeConverter))]
    public partial class DateTime { }
}
