using ARQ.RD;

namespace ARQ.Gateway.RefData.Repositories;

public class RecordMemberInfos
{
    public required IReadOnlyList<ARQ.RD.MemberInfo> data   { get; set; }
    public required IReadOnlyList<ARQ.RD.MemberInfo> header { get; set; }
}

interface IRefDataMetaRepository
{
    IEnumerable<string>            GetEntityTypes();
    RecordMemberInfos              GetRecordMemberInfosForEntity(string entityName);
    IEnumerable<ARQ.RD.MemberInfo> GetMemberInfosForEntity(string entityName);
}