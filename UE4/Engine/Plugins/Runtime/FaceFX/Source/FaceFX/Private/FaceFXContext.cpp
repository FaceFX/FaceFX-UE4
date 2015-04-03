#include FACEFX_RUNTIMEHEADER
#include "FaceFX.h"
#include "FaceFXContext.h"

ffx_context_t FFaceFXContext::CreateContext()
{
	ffx_context_t ctx;
	ctx.alloc_fn = &AllocCallback;
	ctx.free_fn = &FreeCallback;
	return ctx;
}