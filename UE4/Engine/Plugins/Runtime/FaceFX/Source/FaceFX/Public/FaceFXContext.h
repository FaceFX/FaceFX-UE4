#pragma once

struct FFaceFXContext
{
	/**
	* Creates a new context object
	* @returns The new context
	*/
	static struct ffx_context_t CreateContext();

private:

	FFaceFXContext(){}

	/** The memory allocation callback */
	static inline void* AllocCallback(size_t ByteCount, size_t Alignment, void* UserDdata)
	{
		return FMemory::Malloc(ByteCount, Alignment);
	}

	/** The memory deallocation callback */
	static inline void FreeCallback(void* Ptr, size_t Alignment, void* UserData)
	{
		return FMemory::Free(Ptr);
	}
};