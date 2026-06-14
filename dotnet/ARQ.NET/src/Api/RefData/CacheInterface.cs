namespace ARQ.RD;

public interface ICache
{
    IRecord getRecord(ARQ.ID.UUID id);
    IEnumerable<IRecord> getList();
}