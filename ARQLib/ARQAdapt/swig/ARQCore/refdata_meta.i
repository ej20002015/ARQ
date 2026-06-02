#pragma once

%{
#include <ARQCore/refdata_meta.h>
%}

%nspacemove(ARQ::RD::Meta) std::vector<ARQ::RD::Meta::EntityMetadata>;
%template(EntityMetadataVector) std::vector<ARQ::RD::Meta::EntityMetadata>;
%nspacemove(ARQ::RD::Meta) std::vector<ARQ::RD::MemberInfo>;
%template(MemberInfoVector) std::vector<ARQ::RD::MemberInfo>;

namespace ARQ
{
namespace RD
{
namespace Meta
{

struct EntityMetadata
{
	std::string_view        name;
	std::vector<MemberInfo> membersInfo;
};

}
}
}

%nspacemove(ARQ::RD::Meta) Funcs;
%rename(Meta) RDMetaWrapper;

%inline %{
class Funcs {
public:
    static const ARQ::RD::Meta::EntityMetadata&              get( const std::string_view entityName ) { return ARQ::RD::Meta::get( entityName ); }
    static const std::vector<ARQ::RD::Meta::EntityMetadata>& getAll()                                 { return ARQ::RD::Meta::getAll(); }
    static const std::vector<std::string_view>&              getAllNames()                            { return ARQ::RD::Meta::getAllNames(); }
    static const std::vector<ARQ::RD::MemberInfo>&           getHeaderMemberInfos()                   { return ARQ::RD::Meta::getHeaderMemberInfos(); }

private:
    Funcs() {} 
};
%}
