#ifndef _CardEngine_ArrayData_h_
#define _CardEngine_ArrayData_h_

#include <Core/Core.h>

NAMESPACE_UPP

class ArrayData
{
public:
	static Vector<Vector<int>> getHandChancePreflop(int handCode);
};

END_UPP_NAMESPACE

#endif
