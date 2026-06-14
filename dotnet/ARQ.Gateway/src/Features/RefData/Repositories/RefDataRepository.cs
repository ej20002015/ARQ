using ARQ.RD;

namespace ARQ.Gateway.RefData.Repositories;

public class RefDataRepository : IRefDataRepository
{
    private readonly ARQ.RD.Repository _repo;

    public RefDataRepository(string dsh)
    {
        _repo = new ARQ.RD.Repository(dsh);
    }

    public ICache? getCache(string entityName) => _repo.getByName(entityName);
}