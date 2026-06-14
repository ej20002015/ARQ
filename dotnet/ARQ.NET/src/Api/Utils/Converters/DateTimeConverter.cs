using System.ComponentModel;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace ARQ.Converters
{

public class DateTimeConverter : JsonConverter<ARQ.Time.DateTime>
{
    public override ARQ.Time.DateTime Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        DateTime csharpDT = reader.GetDateTime();
        return ARQ.Time.DateTime.FromDateTime(csharpDT);
    }

    public override void Write(Utf8JsonWriter writer, ARQ.Time.DateTime value, JsonSerializerOptions options)
    {
        writer.WriteStringValue(value.ToString());
    }
}

}

namespace ARQ.Time
{

[JsonConverter(typeof(ARQ.Converters.DateTimeConverter))]
public partial class DateTime { }

}