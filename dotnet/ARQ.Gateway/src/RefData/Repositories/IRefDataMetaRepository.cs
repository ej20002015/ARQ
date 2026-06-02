using ARQ.RD;

namespace ARQ.Gateway.RefData.Repositories;

class RecordMemberInfos
{
    public required IReadOnlyList<ARQ.RD.MemberInfo> DataMemberInfos   { get; set; }
    public required IReadOnlyList<ARQ.RD.MemberInfo> HeaderMemberInfos { get; set; }
}

interface IRefDataMetaRepository
{
    IEnumerable<string>            GetEntityTypes();
    RecordMemberInfos              GetRecordMemberInfosForEntity(string entityName);
    IEnumerable<ARQ.RD.MemberInfo> GetMemberInfosForEntity(string entityName);
}