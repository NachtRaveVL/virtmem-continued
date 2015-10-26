#ifndef VIRTMEM_WRAPPER_UTILS_H
#define VIRTMEM_WRAPPER_UTILS_H

/**
  * @file
  * @brief This file contains several utilities for virtual pointers.
  */

#include "wrapper.h"

#include <string.h>

namespace virtmem {

// Generic NULL type
/**
 * @brief Generalized `NULL` pointer class
 *
 * This class can be used to assign a zero (`NULL`) value to both virtual and regular
 * pointers. This is useful, for instance, to write generic code where the type of a pointer is
 * unknown. The \ref virtmem namespace contains a global instance of the CNILL class: \ref NILL.
 * @sa NILL
 */
class CNILL
{
public:
    template <typename T> inline operator T*(void) const { return 0; }
    template <typename T, typename A> inline operator CVirtPtr<T, A>(void) const { return CVirtPtr<T, A>(); }
    inline operator CBaseVirtPtr(void) const { return CBaseVirtPtr(); }
    template <typename T, typename A> inline operator typename CVirtPtr<T, A>::CValueWrapper(void) const { return CVirtPtr<T, A>::CValueWrapper(0); }

};

extern const CNILL NILL;

/**
 * @var NILL
 * @brief Global instance of a CNILL class
 *
 * This variable can be used to assign a `NULL` pointer to both virtual and regular pointers.
 * Example:
 * @code
 * using namespace virtmem;
 * char *a = NILL;
 * CVirtPtr<char, CSdfatlibVirtMemAlloc> b = NILL;
 * CBaseVirtPtr c = NILL;
 * @endcode
 * @sa CNILL
 */

/**
 * @brief Creates a lock to some virtual data
 * @tparam TV Type of virtual pointer that points to data
 *
 * This class is used to create and release locks to virtual data. The use of locks
 * allows more efficient data handling and compatibility with code only accepting regular
 * pointers. For more information see \ref aLocking.
 *
 * The class uses a RAII (resource acquisition is initialization) approach: after a
 * lock has been acquired (usually during construction), it will be automatically released
 * as soon as the class variable goes out of scope.
 *
 * [Wrapped regular pointers](@ref aWrapping) are supported: if a given virtual pointer wraps a
 * regular pointer, this class will not attempt to make or release a lock and
 * operator *(void) will simply return the wrapped pointer.
 */
template <typename TV> class CVirtPtrLock
{
    typedef typename TV::TPtr TPtr;

    TV virtPtr;
    TPtr data;
    TVirtPageSize lockSize;
    bool readOnly;

public:
    /**
     * @brief Constructs a virtual data lock class and creates a lock to the given data.
     * @param v A \ref CVirtPtr "virtual pointer" to the data to be locked.
     * @param s Amount of bytes to lock. **Note**: the actual locked size may be smaller.
     * @param ro Whether locking should read-only (`true`) or not (`false`). If `ro` is
     * `false` (default), the locked data will always be synchronized after unlocking (even if unchanged).
     * Therefore, if no changes in data are expected, it is more efficient to set `ro` to `true`.
     * @sa getLockSize
     */
    CVirtPtrLock(const TV &v, TVirtPageSize s, bool ro=false) :
        virtPtr(v), lockSize(s), readOnly(ro) { lock(); }
    /**
     * @brief Default constructor. No locks are created.
     *
     * The \ref lock(const TV &v, TVirtPageSize s, bool ro) function should be used
     * to create a lock if this constructor is used.
     */
    CVirtPtrLock(void) : data(0), lockSize(0), readOnly(false) { }
    ~CVirtPtrLock(void) { unlock(); } //!< Unlocks data if locked.
    // add extra lock on copying
    CVirtPtrLock(const CVirtPtrLock &other) :
        virtPtr(other.virtPtr), lockSize(other.lockSize), readOnly(other.readOnly) { lock(); }

    /**
     * @brief Recreates a virtual data lock after \ref unlock was called.
     * @note This function will re-use the parameters for locking set by \ref CVirtPtrLock(const TV &v, TVirtPageSize s, bool ro)
     * or \ref lock(const TV &v, TVirtPageSize s, bool ro).
     */
    void lock(void)
    {
#ifdef VIRTMEM_WRAP_CPOINTERS
        if (virtPtr.isWrapped())
            data = virtPtr.unwrap();
        else
#endif
            data = static_cast<TPtr>(TV::getAlloc()->makeFittingLock(virtPtr.ptr, lockSize, readOnly));
    }

    /**
     * @brief Locks data. Parameters are described \ref CVirtPtrLock(const TV &v, TVirtPageSize s, bool ro) "here".
     */
    void lock(const TV &v, TVirtPageSize s, bool ro=false)
    {
        virtPtr = v; lockSize = s; readOnly = ro;
        lock();
    }

    /**
     * @brief Unlocks data (if locked). Automatically called during destruction.
     */
    void unlock(void)
    {
#ifdef VIRTMEM_WRAP_CPOINTERS
        if (!virtPtr.isWrapped() && data)
#else
        if (data)
#endif
        {
            TV::getAlloc()->releaseLock(virtPtr.ptr);
            data = 0;
        }
    }

    TPtr operator *(void) { return data; } //!< Provides access to the data.
    /**
     * @brief Returns the actual size locked.
     *
     * The **actual** size that was locked may be smaller then what was requested.
     * If the requested size was too large, for instance larger then the largest memory page size,
     * the locked size will be adjusted accordingly. For this reason, it is *very important* to
     * use this function after a lock has been created.
     */
    TVirtPageSize getLockSize(void) const { return lockSize; }
};

// Shortcut
/**
 * @brief Creates a virtual lock (shortcut)
 *
 * This function is a shortcut for making a virtual lock: unlike using CVirtPtrLock,
 * you don't need to specify the virtual pointer type as a template parameter. The
 * function parameters are the same as CVirtPtrLock::CVirtPtrLock.
 * @sa CVirtPtr and @ref aLocking
 */
template <typename T> CVirtPtrLock<T> makeVirtPtrLock(const T &w, TVirtPageSize s, bool ro=false)
{ return CVirtPtrLock<T>(w, s, ro); }

namespace private_utils {
// Ugly hack from http://stackoverflow.com/a/12141673
// a null pointer of T is used to get the offset of m. The char & is to avoid any dereference operator overloads and the
// char * subtraction to get the actual offset.
template <typename C, typename M> ptrdiff_t getMembrOffset(const M C::*m)
{ return ((char *)&((char &)((C *)(0)->*m)) - (char *)(0)); }
}

/**
 * @brief Obtains a virtual pointer to a data member that is stored in virtual memory.
 *
 * @tparam C Type of virtual data (e.g. the structure)
 * @tparam M Type of the data member
 * @tparam A Type of the virtual memory allocator
 * @param c The virtual pointer of the structure (or class) to which the data member belongs
 * @param m A pointer to the member (e.g. &MyStruct::x)
 * @sa [section about virtual pointers to data members in manual](@ref aPointStructMem)
 */
template <typename C, typename M, typename A> inline CVirtPtr<M, A> getMembrPtr(const CVirtPtr<C, A> &c, const M C::*m)
{ CVirtPtr<M, A> ret; ret.setRawNum(c.getRawNum() + private_utils::getMembrOffset(m)); return ret; }

/**
 * @brief Obtains a virtual pointer to a nested data member that is stored in virtual memory.
 *
 * @tparam C Type of virtual data (e.g. the structure)
 * @tparam M Type of the data member
 * @tparam NC Type of the nested structure
 * @tparam NM Type of nested data member
 * @tparam A Type of the virtual memory allocator
 * @param c The virtual pointer of the structure (or class) to which the data member belongs
 * @param m A pointer to the nested structure/class (e.g. &MyStruct::MyOtherStruct)
 * @param nm A pointer to the nested data member (e.g. &MyStruct::MyOtherStruct::y)
 * @sa [section about virtual pointers to data members in manual](@ref aPointStructMem)
 */
template <typename C, typename M, typename NC, typename NM, typename A> inline CVirtPtr<NM, A> getMembrPtr(const CVirtPtr<C, A> &c, const M C::*m, const NM NC::*nm)
{ CVirtPtr<NM, A> ret = static_cast<CVirtPtr<NM, A> >(static_cast<CVirtPtr<char, A> >(getMembrPtr(c, m)) + private_utils::getMembrOffset(nm)); return ret; }

}

#include "utils.hpp"

#endif // VIRTMEM_WRAPPER_UTILS_H
