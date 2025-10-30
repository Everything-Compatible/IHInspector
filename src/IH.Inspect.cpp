
#ifdef IHINSPECTOR

#include "InspectImpl.h"

void IHInspect_Type(JsonObject Args)
{
	IHInspect_Type_Impl(Args);
}


#else

#include "IH.Inspect.h"

void IHInspect_Type(JsonObject Args)
{
	ECDispatch(IHInspect_Type, IHInspectorLibName, "Type", IHInspectorVersion)(Args);
}

#endif // IHINSPECTOR