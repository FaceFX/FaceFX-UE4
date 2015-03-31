#pragma once

struct FFaceFxContext
{
	/**
	* Creates a new context object
	* @returns The new context
	*/
	static struct ffx_context_t CreateFaceFxContext();

private:

	FFaceFxContext(){}

	/** The memory allocation callback */
	static inline void* AllocCallback(size_t byte_count, size_t alignment, void* user_data)
	{
		return FMemory::Malloc(byte_count, alignment);
	}

	/** The memory deallocation callback */
	static inline void FreeCallback(void* ptr, size_t alignment, void* user_data)
	{
		return FMemory::Free(ptr);
	}
};