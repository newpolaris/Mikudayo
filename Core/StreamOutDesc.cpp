#include "pch.h"
#include "StreamOutDesc.h"
#include "GraphicsCore.h"
#include <boost/functional/hash.hpp>

using namespace std;

std::size_t hash_value( const StreamOutDesc& Desc )
{
	using namespace boost;

	// Offset is fixed by Format
	size_t HashCode = hash_value( Desc.Stream );
	hash_combine( HashCode, Desc.SemanticName );
	hash_combine( HashCode, Desc.SemanticIndex );
	hash_combine( HashCode, Desc.StartComponent );
	hash_combine( HashCode, Desc.ComponentCount );
	hash_combine( HashCode, Desc.OutputSlot );

	return HashCode;
}
