#include "FaceFx-0.9.10/facefx.h"
#include "FaceFx.h"
#include "FaceFxContext.h"

ffx_context_t FFaceFxContext::CreateFaceFxContext()
{
	ffx_context_t ctx;
	ctx.alloc_fn  = &AllocCallback;
	ctx.free_fn   = &FreeCallback;
	return ctx;
}

/** The memory allocation callback */
void* FFaceFxContext::AllocCallback(size_t byte_count, size_t alignment, void* user_data)
{
	return FMemory::Malloc(byte_count, alignment);
}

/** The memory deallocation callback */
void FFaceFxContext::FreeCallback(void* ptr, size_t alignment, void* user_data)
{
	return FMemory::Free(ptr);
}