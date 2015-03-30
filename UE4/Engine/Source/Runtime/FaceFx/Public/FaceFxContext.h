#pragma once

struct FFaceFxContext
{
	/**
	* Creates a new context object
	* @returns The new context
	*/
	struct ffx_context_t CreateFaceFxContext();

	/**
	* Gets the FaceFX context
	* @returns The context
	*/
	static FFaceFxContext& GetInstance()
	{
		static FFaceFxContext instance;
		return instance;
	}

private:

	FFaceFxContext(){}

	/** The memory allocation callback */
	static void* AllocCallback(size_t byte_count, size_t alignment, void* user_data);

	/** The memory deallocation callback */
	static void FreeCallback(void* ptr, size_t alignment, void* user_data);
};