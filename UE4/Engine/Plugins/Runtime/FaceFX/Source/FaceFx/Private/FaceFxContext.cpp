#include FACEFX_RUNTIMEHEADER
#include "FaceFx.h"
#include "FaceFxContext.h"

ffx_context_t FFaceFxContext::CreateFaceFxContext()
{
	ffx_context_t ctx;
	ctx.alloc_fn = &AllocCallback;
	ctx.free_fn = &FreeCallback;
	return ctx;
}