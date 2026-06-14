using ARQ.RD;

namespace ARQ.Gateway.RefData.Repositories;

interface IRefDataRepository
{
    public ICache? getCache(string entityName);
}