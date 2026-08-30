using ARQ.Gateway.RefData.Endpoints;
using ARQ.Gateway.RefData.Repositories;
using ARQ.RD;
using Microsoft.AspNetCore.Http;

namespace ARQ.Gateway.Tests.Features.RefData.Endpoints;

public class RefDataEndpointsTests
{
    [Fact]
    public void GetRecordsReturnsNotFoundForUnsupportedEntity()
    {
        var result = RefDataEndpoints.GetRecords("unknown", new FakeRefDataRepository());

        AssertStatusCode(StatusCodes.Status404NotFound, result);
    }

    [Fact]
    public void GetRecordsReturnsRecordsFromTheSelectedCache()
    {
        IReadOnlyList<IRecord> records = [new FakeRecord()];
        var repository = new FakeRefDataRepository(new FakeCache(records));

        var result = RefDataEndpoints.GetRecords("currency", repository);

        AssertStatusCode(StatusCodes.Status200OK, result);
        Assert.Same(records, AssertValue<IEnumerable<IRecord>>(result));
        Assert.Equal("currency", repository.RequestedEntity);
    }

    [Fact]
    public void GetRecordReportsUnsupportedEntity()
    {
        var result = RefDataEndpoints.GetRecord("unknown", "00000000-0000-0000-0000-000000000001", new FakeRefDataRepository());

        AssertStatusCode(StatusCodes.Status404NotFound, result);
        Assert.Equal(
            "Reference data entity 'unknown' is not supported.",
            GetError(result));
    }

    [Fact]
    public void GetMetadataMapsUnknownEntityToNotFound()
    {
        var repository = new FakeRefDataMetaRepository
        {
            MetadataException = new ArgumentException("Unknown metadata entity")
        };

        var result = RefDataEndpoints.GetMetadata("unknown", repository);

        AssertStatusCode(StatusCodes.Status404NotFound, result);
        Assert.Equal("Unknown metadata entity", GetError(result));
    }

    [Fact]
    public void GetEntitiesReturnsRepositoryEntityTypes()
    {
        IReadOnlyList<string> entityTypes = ["Currency", "User"];
        var repository = new FakeRefDataMetaRepository
        {
            EntityTypes = entityTypes
        };

        var result = RefDataEndpoints.GetEntities(repository);

        AssertStatusCode(StatusCodes.Status200OK, result);
        Assert.Same(entityTypes, AssertValue<IEnumerable<string>>(result));
    }

    private static void AssertStatusCode(int expected, IResult result)
    {
        var statusResult = Assert.IsAssignableFrom<IStatusCodeHttpResult>(result);
        Assert.Equal(expected, statusResult.StatusCode);
    }

    private static T AssertValue<T>(IResult result)
    {
        var valueResult = Assert.IsAssignableFrom<IValueHttpResult>(result);
        return Assert.IsAssignableFrom<T>(valueResult.Value);
    }

    private static string GetError(IResult result)
    {
        var valueResult = Assert.IsAssignableFrom<IValueHttpResult>(result);
        Assert.NotNull(valueResult.Value);

        var errorProperty = valueResult.Value.GetType().GetProperty("Error");
        Assert.NotNull(errorProperty);

        return Assert.IsType<string>(errorProperty.GetValue(valueResult.Value));
    }

    private sealed class FakeRecord : IRecord
    {
    }

    private sealed class FakeCache(IReadOnlyList<IRecord> records) : ICache
    {
        public IRecord getRecord(ARQ.ID.UUID id) => throw new NotSupportedException();

        public IEnumerable<IRecord> getList() => records;
    }

    private sealed class FakeRefDataRepository(ICache? cache = null) : IRefDataRepository
    {
        public string? RequestedEntity { get; private set; }

        public ICache? getCache(string entityName)
        {
            RequestedEntity = entityName;
            return cache;
        }
    }

    private sealed class FakeRefDataMetaRepository : IRefDataMetaRepository
    {
        public IReadOnlyList<string> EntityTypes { get; init; } = [];
        public ArgumentException? MetadataException { get; init; }

        public IEnumerable<string> GetEntityTypes() => EntityTypes;

        public RecordMemberInfos GetRecordMemberInfosForEntity(string entityName)
        {
            if (MetadataException != null)
                throw MetadataException;

            return new RecordMemberInfos
            {
                data = [],
                header = []
            };
        }

        public IEnumerable<ARQ.RD.MemberInfo> GetMemberInfosForEntity(string entityName) => [];
    }
}
