// AllocatorWithNul.h - written and placed in the public domain by Robin Raymond based secblock.h in Wei Dai source code.

//! \file AllocatorWithNul.h
//! \brief Classes and functions for secure memory allocations.

#ifndef CRYPTOPP_ALLOCATOR_WITH_NULL_H
#define CRYPTOPP_ALLOCATOR_WITH_NULL_H

#include "secblock.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4700)
# if (CRYPTOPP_MSC_VERSION >= 1400)
#  pragma warning(disable: 6386)
# endif
#else
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdocumentation"
#endif

NAMESPACE_BEGIN(CryptoPP)

// ************** secure memory allocation ***************

//! \class AllocatorWithNul
//! \brief Allocates a block of memory with a NUL terminator of type T with zero byte initialization
//!    and with cleanup
//! \tparam T class or type
//! \tparam T_Align16 boolean that determines whether allocations should be aligned on 16-byte boundaries
//! \details If T_Align16 is true, then AllocatorWithNul calls AlignedAllocate()
//!    for memory allocations. If T_Align16 is false, then AllocatorWithNul() calls
//!    UnalignedAllocate() for memory allocations.
//! \details Template parameter T_Align16 is effectively controlled by cryptlib.h and mirrors
//!    CRYPTOPP_BOOL_ALIGN16. CRYPTOPP_BOOL_ALIGN16 is often used as the template parameter.
template <class T, bool T_Align16 = false>
class AllocatorWithNul : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	//! \brief Allocates a block of memory
	//! \param ptr the size of the allocation
	//! \param size the size of the allocation, in elements
	//! \returns a memory block
	//! \throws InvalidArgument
	//! \details allocate() first checks the size of the request. If it is non-0
	//!   and less than max_size(), then an attempt is made to fulfill the request using either
	//!   AlignedAllocate() or UnalignedAllocate().
	//! \details AlignedAllocate() is used if T_Align16 is true.
	//!   UnalignedAllocate() used if T_Align16 is false.
	//! \details This is the C++ *Placement New* operator. ptr is not used, and the function
	//!   asserts in Debug builds if ptr is non-NULL.
	//! \sa CallNewHandler() for the methods used to recover from a failed
	//!   allocation attempt.
	//! \note size is the count of elements, and not the number of bytes
	pointer allocate(size_type size, const void *ptr = NULL)
	{
		CRYPTOPP_UNUSED(ptr); assert(ptr == NULL);
		this->CheckSize(size);
		if (size == 0)
			return NULL;

#if CRYPTOPP_BOOL_ALIGN16
		// TODO: should this need the test 'size*sizeof(T) >= 16'?
		if (T_Align16 && size*sizeof(T) >= 16) {
			auto result = (pointer)AlignedAllocate(size*sizeof(T)+sizeof(T));
      memset(result, 0, size*sizeof(T)+sizeof(T));
      return result;
    }
#endif

		auto result = (pointer)UnalignedAllocate(size*sizeof(T)+sizeof(T));
    memset(result, 0, size*sizeof(T)+sizeof(T));
    return result;
 }

	//! \brief Deallocates a block of memory
	//! \param ptr the pointer for the allocation
	//! \param size the size of the allocation, in elements
	//! \details Internally, SecureWipeArray() is called before deallocating the memory.
	//!   Once the memory block is wiped or zeroized, AlignedDeallocate() or
	//!   UnalignedDeallocate() is called.
	//! \details AlignedDeallocate() is used if T_Align16 is true.
	//!   UnalignedDeallocate() used if T_Align16 is false.
	void deallocate(void *ptr, size_type size)
	{
		assert((ptr && size) || !(ptr || size));
		SecureWipeArray((pointer)ptr, size);

#if CRYPTOPP_BOOL_ALIGN16
		if (T_Align16 && size*sizeof(T) >= 16)
			return AlignedDeallocate(ptr);
#endif

		UnalignedDeallocate(ptr);
	}

	//! \brief Reallocates a block of memory
	//! \param oldPtr the previous allocation
	//! \param oldSize the size of the previous allocation
	//! \param newSize the new, requested size
	//! \param preserve flag that indicates if the old allocation should be preserved
	//! \returns pointer to the new memory block
	//! \details Internally, reallocate() calls StandardReallocate().
	//! \details If preserve is true, then index 0 is used to begin copying the
	//!   old memory block to the new one. If the block grows, then the old array
	//!   is copied in its entirety. If the block shrinks, then only newSize
	//!   elements are copied from the old block to the new one.
	//! \note oldSize and newSize are the count of elements, and not the
	//!   number of bytes.
	pointer reallocate(T *oldPtr, size_type oldSize, size_type newSize, bool preserve)
	{
		assert((oldPtr && oldSize) || !(oldPtr || oldSize));
		return StandardReallocate(*this, oldPtr, oldSize, newSize, preserve);
	}

	//! \brief Template class memeber Rebind
	//! \tparam T allocated class or type
	//! \tparam T_Align16 boolean that determines whether allocations should be aligned on 16-byte boundaries
	//! \tparam U bound class or type
	//! \details Rebind allows a container class to allocate a different type of object
	//!   to store elements. For example, a std::list will allocate std::list_node to
	//!   store elements in the list.
	//! \details VS.NET STL enforces the policy of "All STL-compliant allocators
	//!   have to provide a template class member called rebind".
    template <class U> struct rebind { typedef AllocatorWithNul<U, T_Align16> other; };
#if _MSC_VER >= 1500
    AllocatorWithNul() {}
	template <class U, bool A> AllocatorWithNul(const AllocatorWithNul<U, A> &) {}
#endif
};

CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithNul<byte>;
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithNul<word16>;
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithNul<word32>;
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithNul<word64>;
#if defined(CRYPTOPP_WORD128_AVAILABLE)
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithNul<word128, true>; // for Integer
#endif
#if CRYPTOPP_BOOL_X86
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithNul<word, true>;	 // for Integer
#endif

NAMESPACE_END

NAMESPACE_BEGIN(std)


#if defined(_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE) || (defined(_STLPORT_VERSION) && !defined(_STLP_MEMBER_TEMPLATE_CLASSES))
// working for STLport 5.1.3 and MSVC 6 SP5
template <class _Tp1, class _Tp2>
inline CryptoPP::AllocatorWithNul<_Tp2>&
__stl_alloc_rebind(CryptoPP::AllocatorWithNul<_Tp1>& __a, const _Tp2*)
{
	return (CryptoPP::AllocatorWithNul<_Tp2>&)(__a);
}
#endif

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#else
# pragma GCC diagnostic pop
#endif

#endif
