namespace ARQ.Gateway.RefData.Repositories;

class RefDataMetaRepository : IRefDataMetaRepository
{
    private readonly IReadOnlyList<string>                                _entityTypes;
    private readonly Dictionary<string, IReadOnlyList<ARQ.RD.MemberInfo>> _memberInfos;
    private readonly Dictionary<string, RecordMemberInfos>                _recordMemberInfos;

    public RefDataMetaRepository()
    {
        var entityMetadata = ARQ.RD.Meta.Funcs.getAll();
        _entityTypes = entityMetadata.Select(m => m.name).ToList();
        _memberInfos = entityMetadata.ToDictionary<ARQ.RD.Meta.EntityMetadata, string, IReadOnlyList<ARQ.RD.MemberInfo>>(
                m => m.name,
                m => m.membersInfo.ToList(),
                StringComparer.OrdinalIgnoreCase
        );

        var recordHeaderMemberInfos = ARQ.RD.Meta.Funcs.getHeaderMemberInfos().ToList();
        _recordMemberInfos = _memberInfos.ToDictionary(
                kvp => kvp.Key,
                kvp => new RecordMemberInfos
                {
                    DataMemberInfos   = kvp.Value,
                    HeaderMemberInfos = recordHeaderMemberInfos
                },
                StringComparer.OrdinalIgnoreCase
        );
    }
    public IEnumerable<string> GetEntityTypes()
    {
        return _entityTypes;
    }

    public RecordMemberInfos GetRecordMemberInfosForEntity(string entityName)
    {
        if (_recordMemberInfos.TryGetValue(entityName, out var memberInfos))
            return memberInfos;
        else
            throw new ArgumentException($"Entity '{entityName}' not found in RefData metadata repository");
    }

    public IEnumerable<ARQ.RD.MemberInfo> GetMemberInfosForEntity(string entityName)
    {
        if (_memberInfos.TryGetValue(entityName, out var memberInfos))
            return memberInfos;
        else
            throw new ArgumentException($"Entity '{entityName}' not found in RefData metadata repository");
    }
}