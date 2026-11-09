#include "SweetSandy.h"

NAMESPACE_UPP

// U++'s generic Stream serialization framework (Stream::operator%, see
// Vec2::Serialize in SandyMath.h) composes automatically through nested
// containers once the leaf type is serializable - Vector<Vector<Vec2>>
// needs no serialization code of its own.
String WorldChunkContent::Serialize() const
{
	WorldChunkContent& self = const_cast<WorldChunkContent&>(*this);
	StringStream ss;
	ss.Create();
	ss % self.road_centerlines % self.blocks % self.road_mesh % self.vegetation % self.buildings % self.ground;
	return ss.GetResult();
}

WorldChunkContent WorldChunkContent::Deserialize(const String& data)
{
	WorldChunkContent c;
	StringStream ss;
	ss.Open(data);
	ss % c.road_centerlines % c.blocks % c.road_mesh % c.vegetation % c.buildings % c.ground;
	return c;
}

END_UPP_NAMESPACE
