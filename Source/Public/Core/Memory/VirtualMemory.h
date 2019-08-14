#pragma once
#include "Core/Memory/Memory.h"

namespace Zn
{
    class VirtualMemory
    {
    public:
        static void* Reserve(size_t size);

        static void* Allocate(size_t size);

        static bool Release(void* address);

        static bool Commit(void* address, size_t size);

        static bool Decommit(void* address, size_t size);

        static size_t GetPageSize();

        static size_t AlignToPageSize(size_t size);
    };

	struct MemoryResource
	{
	public:

		MemoryResource()		= default;

		MemoryResource(size_t capacity, size_t alignment);

		MemoryResource(MemoryResource&& other);

		~MemoryResource();

		MemoryResource(const MemoryResource&) = delete;

		MemoryResource& operator=(const MemoryResource&) = delete;
		
		void* operator*() const { return m_Resource; }

		operator bool() const { return m_Resource != nullptr; }

		size_t Size() const		{ return m_Range.Size(); }

		const MemoryRange& Range() const { return m_Range; }

	private:

		void* m_Resource		= nullptr;

		MemoryRange m_Range;
	};
}
