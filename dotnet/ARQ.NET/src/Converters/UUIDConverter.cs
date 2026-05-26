using System.Text.Json;
using System.Text.Json.Serialization;

namespace ARQ.Converters;

public class UUIDConverter : JsonConverter<ARQ.ID.UUID>
{
    public override ARQ.ID.UUID Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        System.Guid guid = reader.GetGuid();
        return ARQ.ID.UUID.FromGuid(guid);
    }

    public override void Write(Utf8JsonWriter writer, ARQ.ID.UUID value, JsonSerializerOptions options)
    {
        System.Guid guid = value;
        writer.WriteStringValue(guid.ToString());
    }
}