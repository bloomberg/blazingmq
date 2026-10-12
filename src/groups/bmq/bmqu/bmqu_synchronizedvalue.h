// Copyright 2026 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_BMQU_SYNCHRONIZEDVALUE
#define INCLUDED_BMQU_SYNCHRONIZEDVALUE

///@PURPOSE: Provide utility class for synchronized access to a value.
///
///@CLASSES:
/// bmqu::SynchronizedValue: A container that associates a mutex with a value.
///
///@DESCRIPTION:
/// It is difficult to use mutexes. Mutex often do not explicitly associate
/// with the data they protect. This often leads to programmer errors failing
/// to lock the mutexes around access to the data they are supposed to protect.
/// `SynchronizedValue` attempts to address this by associating a mutex to a
/// container type which restricts access to the associated data except by
/// constructing a lock.
///
/// Example
/// =======
///
/// ```c++
/// bmqu::SynchronizedValue<int> count(0);
///
/// // You can't directly access count
/// // BMQTST_ASSERT_EQ(0, count);
///
/// // Get a read-only LockedPtr to the underlying value
/// bmqu::SynchronizedValue<int>::ReadLockedPtr lockedPtr(&count);
/// BMQTST_ASSERT_EQ(0, *lockedPtr);
/// ```
///
/// Usage
/// =====
///
/// `SynchronizedValue<T, M>` has two input template parameters:
///
/// 1. `T` => The type of the data being protected by synchronized access
/// 2. `M` => The type of the mutex to use for locking.
///
/// There are no restrictions on the choice of `T`.
///
/// `SynchronizedValue<T, M>` is neither copyable nor movable because of the
/// behavior of the associated mutex. This puts a fairly strict restriction on
/// places `T` would otherwise be used. It is expected that most uses of
/// `SynchronizedValue<T, M>` will be managed by a pointer type such as
/// `bsl::shared_ptr<SynchronizedValue<T, M> >` or
/// `bslma::ManagedPtr<SynchronizedValue<T, M> >`.
///
/// Only two mutex types from the `bslmt` package are supported:
/// `ReaderWriterMutex` and `Mutex`. Additionally, `SynchronizedValue<T, M>` is
/// specialized depending on the behavior of `M`. The default choice for `M` is
/// `bslmt::ReaderWriterLock`.
///
/// Shared Locks
/// ------------
///
/// If `M` supports lock sharing (e.g. `ReaderWriterMutex::lockRead`) then
/// `SynchronizedValue<T, M>` will have four associated types:
///
/// 1. `ReadLockedPtr`
/// 2. `ConstReadLockedPtr`
/// 3. `WriteLockedPtr`
/// 4. `ConstWriteLockedPtr`
///
/// These types can be used to acquire a lock for the underlying value as shown
/// in the example above. The `ReadLockedPtr` variants share the lock between
/// potentially multiple readers. The `WriteLockedPtr` variants require
/// exclusive access.
///
/// `SynchronizedValuve<T, M>` also supports member functions lockRead and
/// lockWrite to acquire a locked pointer on modern C++ standards.
///
/// Unique Locks
/// ------------
///
/// If `M` doesn't support lock sharing (e.g. `Mutex::lock`)
/// then `SynchronizedValue<T, M>` will have two associated types:
///
/// 1. `LockedPtr`
/// 2. `ConstLockedPtr`
///
/// These types can be used to acquire a lock for the underlying value as shown
/// in the example above. Both variants require exclusive access to the lock.
///
/// Modern C++ Features
/// -------------------
///
/// `SynchronizedValuve<T, M>` also supports member functions `lockRead` and
/// `lockWrite` (in the case of a shareable mutex) or `lock` (in the case of a
/// unique mutex) to acquire a locked pointer.
///
/// Safety
/// ------
///
/// This component is designed to help programmers naturally express thread
/// safety properties. As a result, it is critical that access to the
/// underlying value is managed by the `LockedPtr` objects. Holding onto a
/// reference to the underlying value beyond the lifetime of a `LockedPtr` can
/// potentially lead to undefined behavior.

#include <bsl_algorithm.h>
#include <bslmf_addconst.h>
#include <bslmf_conditional.h>
#include <bslmf_isconst.h>
#include <bslmf_movableref.h>
#include <bslmt_lockguard.h>
#include <bslmt_mutex.h>
#include <bslmt_readerwritermutex.h>
#include <bslmt_readlockguard.h>
#include <bslmt_writelockguard.h>
#include <bsls_compilerfeatures.h>
#include <bsls_keyword.h>

namespace BloombergLP {
namespace bmqu {

template <typename t_VALUE, typename t_MUTEX>
class SynchronizedValue;

/// This is a dumb adapter to help with template specialization of locking
/// behavior
struct SynchronizedValue_LockUtil {
    /// Acquire a lock (`mutex->lock()`)
    template <typename t_MUTEX>
    static void lockUnique(t_MUTEX* lock);

    /// Acquire a lock (`mutex->unlock()`)
    template <typename t_MUTEX>
    static void unlockUnique(t_MUTEX* lock);
};

/// This is a type that adapts mutex types from bslmt that have a lockRead
/// method.
template <typename t_MUTEX>
class SynchronizedValue_SharedLock {
  public:
    // TYPES
    typedef t_MUTEX MutexType;

  private:
    // DATA
    MutexType* d_mutex_p;
    bool       d_isOwned;

  public:
    /// An empty lock
    SynchronizedValue_SharedLock();

    /// Acquire a read lock on the mutex.
    /// @pre mutex != NULL
    explicit SynchronizedValue_SharedLock(MutexType* mutex);

    /// Move the lock managed by other into a new object.
    SynchronizedValue_SharedLock(
        bslmf::MovableRef<SynchronizedValue_SharedLock> other)
        BSLS_KEYWORD_NOEXCEPT;

    /// Move the lock managed by other into a this object.
    SynchronizedValue_SharedLock&
    operator=(bslmf::MovableRef<SynchronizedValue_SharedLock> other)
        BSLS_KEYWORD_NOEXCEPT;

    ~SynchronizedValue_SharedLock() BSLS_KEYWORD_NOEXCEPT;

  private:
    SynchronizedValue_SharedLock(const SynchronizedValue_SharedLock& other)
        BSLS_KEYWORD_DELETED;
    SynchronizedValue_SharedLock&
    operator=(const SynchronizedValue_SharedLock& other) BSLS_KEYWORD_DELETED;

  public:
    // ACCESSORS

    /// Returns a pointer to the associated mutex
    MutexType* mutex() const BSLS_KEYWORD_NOEXCEPT;

    /// Return whether this lock owns the associated mutex
    bool ownsLock() const BSLS_KEYWORD_NOEXCEPT;

    // MANIPULATORS

    /// Acquire a read lock (`mutex()->lockRead()`)
    void lock();

    /// Release a read lock (`mutex()->lockRead()`).
    /// @pre The lock must already be acquired.
    void unlock();

    /// Disassociate the mutex from this lock
    MutexType* release() BSLS_KEYWORD_NOEXCEPT;

    /// Swap the values of this lock and other
    void swap(SynchronizedValue_SharedLock& other) BSLS_KEYWORD_NOEXCEPT;
};

/// This is a type that adapts mutex types from bslmt that have a lock
/// method.
template <typename t_MUTEX>
class SynchronizedValue_UniqueLock {
  public:
    // TYPES
    typedef t_MUTEX MutexType;

  private:
    // DATA
    MutexType* d_mutex_p;
    bool       d_isOwned;

  public:
    /// An empty lock
    SynchronizedValue_UniqueLock();

    /// Acquire a read lock on the mutex.
    /// @pre mutex != NULL
    explicit SynchronizedValue_UniqueLock(MutexType* mutex);

    /// Move the lock managed by other into a new object.
    SynchronizedValue_UniqueLock(
        bslmf::MovableRef<SynchronizedValue_UniqueLock> other)
        BSLS_KEYWORD_NOEXCEPT;

    /// Move the lock managed by other into a this object.
    SynchronizedValue_UniqueLock&
    operator=(bslmf::MovableRef<SynchronizedValue_UniqueLock> other)
        BSLS_KEYWORD_NOEXCEPT;

    ~SynchronizedValue_UniqueLock() BSLS_KEYWORD_NOEXCEPT;

  private:
    SynchronizedValue_UniqueLock(const SynchronizedValue_UniqueLock& other)
        BSLS_KEYWORD_DELETED;
    SynchronizedValue_UniqueLock&
    operator=(const SynchronizedValue_UniqueLock& other) BSLS_KEYWORD_DELETED;

  public:
    // ACCESSORS

    /// Returns a pointer to the associated mutex
    MutexType* mutex() const BSLS_KEYWORD_NOEXCEPT;

    /// Return whether this lock owns the associated mutex
    bool ownsLock() const BSLS_KEYWORD_NOEXCEPT;

    // MANIPULATORS

    /// Acquire a read lock (`mutex()->lockRead()`)
    void lock();

    /// Acquire a read lock (`mutex()->lockRead()`)
    void unlock();

    /// Disassociate the mutex from this lock
    MutexType* release() BSLS_KEYWORD_NOEXCEPT;

    /// Swap the values of this lock and other
    void swap(SynchronizedValue_UniqueLock& other) BSLS_KEYWORD_NOEXCEPT;
};

struct SynchronizedValue_LockPolicy {
    enum LockPolicy { k_UNIQUE, k_SHARED };
};

/// Get the LockGuard for a given t_MUTEX type
template <int t_POLICY, typename t_MUTEX>
struct SynchronizedValue_LockType {};

template <>
struct SynchronizedValue_LockType<SynchronizedValue_LockPolicy::k_SHARED,
                                  bslmt::ReaderWriterMutex> {
    typedef SynchronizedValue_SharedLock<bslmt::ReaderWriterMutex> Type;
};

template <>
struct SynchronizedValue_LockType<SynchronizedValue_LockPolicy::k_UNIQUE,
                                  bslmt::ReaderWriterMutex> {
    typedef SynchronizedValue_UniqueLock<bslmt::ReaderWriterMutex> Type;
};

template <>
struct SynchronizedValue_LockType<SynchronizedValue_LockPolicy::k_UNIQUE,
                                  bslmt::Mutex> {
    typedef SynchronizedValue_UniqueLock<bslmt::Mutex> Type;
};

/// Get the broadest policy for a given mutex type.
template <typename t_MUTEX>
struct SynchronizedValue_MutexTraits {};

/// The lock policy for ReaderWriterMutex is shared, even though the lock on
/// this type can be held exclusively (by lockWrite() methods).
template <>
struct SynchronizedValue_MutexTraits<bslmt::ReaderWriterMutex> {
    enum { k_LOCK_POLICY = SynchronizedValue_LockPolicy::k_SHARED };
};

/// The lock policy for Mutex is always exclusive.
template <>
struct SynchronizedValue_MutexTraits<bslmt::Mutex> {
    enum { k_LOCK_POLICY = SynchronizedValue_LockPolicy::k_UNIQUE };
};

/// A helper for determining the access type for a synchronized value based on
/// the access policy. If the access policy is not unique or the synchronized
/// value is const, only allow const access to the underlying value.
template <typename t_SYNCHRONIZED, int t_POLICY>
struct SynchronizedValue_AccessValueType
: public bsl::conditional<
      t_POLICY != SynchronizedValue_LockPolicy::k_UNIQUE ||
          bsl::is_const<t_SYNCHRONIZED>::value,
      typename bsl::add_const<typename t_SYNCHRONIZED::ValueType>::type,
      typename t_SYNCHRONIZED::ValueType> {};

template <typename t_SYNCHRONIZED, int t_POLICY>
class SynchronizedValue_LockedPtr {
  public:
    // TYPES

    typedef t_SYNCHRONIZED SynchronizedType;
    typedef typename SynchronizedValue_AccessValueType<SynchronizedType,
                                                       t_POLICY>::type
                                                 AccessValueType;
    typedef typename SynchronizedType::ValueType ValueType;
    typedef typename SynchronizedType::MutexType MutexType;
    typedef typename SynchronizedValue_LockType<t_POLICY, MutexType>::Type
        LockType;

  private:
    // DATA

    SynchronizedType* d_value_p;  // Borrowed
    LockType          d_lock;

  public:
    // CREATORS
    explicit SynchronizedValue_LockedPtr(SynchronizedType* value);

    SynchronizedValue_LockedPtr(bslmf::MovableRef<SynchronizedValue_LockedPtr>
                                    other) BSLS_KEYWORD_NOEXCEPT;

    SynchronizedValue_LockedPtr&
    operator=(bslmf::MovableRef<SynchronizedValue_LockedPtr> other)
        BSLS_KEYWORD_NOEXCEPT;

    ~SynchronizedValue_LockedPtr() BSLS_KEYWORD_NOEXCEPT;

  private:
    SynchronizedValue_LockedPtr(const SynchronizedValue_LockedPtr& other)
        BSLS_KEYWORD_DELETED;

    SynchronizedValue_LockedPtr&
    operator=(const SynchronizedValue_LockedPtr& other) BSLS_KEYWORD_DELETED;

  public:
    // ACCESSORS

    /// Return true if this object is managing a lock on a valid value.
    bool ownsLock() const;

    // MANIPULATORS

    /// Return the address of the object under management by this LockedPtr, or
    /// NULL if nothing is managed.
    ///
    /// @pre This object is managing a valid lock
    AccessValueType* operator->() const;

    /// Return a reference to the object under management by this LockedPtr, or
    /// NULL if nothing is managed.
    ///
    /// @pre This object is managing a valid lock
    AccessValueType& operator*() const;

    /// Swap the managed lock of this LockedPtr and other.
    void swap(SynchronizedValue_LockedPtr& other) BSLS_KEYWORD_NOEXCEPT;

    /// Release management by this LockedPtr.
    void unlock() BSLS_KEYWORD_NOEXCEPT;
};

template <typename t_SUBCLASS, int t_POLICY>
class SynchronizedValue_Base;

template <typename t_SUBCLASS>
class SynchronizedValue_Base<t_SUBCLASS,
                             SynchronizedValue_LockPolicy::k_SHARED> {
  public:
    typedef SynchronizedValue_LockedPtr<t_SUBCLASS,
                                        SynchronizedValue_LockPolicy::k_UNIQUE>
        WriteLockedPtr;
    typedef SynchronizedValue_LockedPtr<const t_SUBCLASS,
                                        SynchronizedValue_LockPolicy::k_UNIQUE>
        ConstWriteLockedPtr;
    typedef SynchronizedValue_LockedPtr<t_SUBCLASS,
                                        SynchronizedValue_LockPolicy::k_SHARED>
        ReadLockedPtr;
    typedef SynchronizedValue_LockedPtr<const t_SUBCLASS,
                                        SynchronizedValue_LockPolicy::k_SHARED>
        ConstReadLockedPtr;

#ifdef BSLS_COMPILERFEATURES_GUARANTEED_COPY_ELISION
    // MANIPULATORS

    /// Acquire a read lock
    ReadLockedPtr      lockRead();
    ConstReadLockedPtr lockRead() const;

    /// Acquire a write lock
    WriteLockedPtr      lockWrite();
    ConstWriteLockedPtr lockWrite() const;
#endif
};

template <typename t_SUBCLASS>
class SynchronizedValue_Base<t_SUBCLASS,
                             SynchronizedValue_LockPolicy::k_UNIQUE> {
  public:
    typedef SynchronizedValue_LockedPtr<t_SUBCLASS,
                                        SynchronizedValue_LockPolicy::k_UNIQUE>
        LockedPtr;
    typedef SynchronizedValue_LockedPtr<const t_SUBCLASS,
                                        SynchronizedValue_LockPolicy::k_UNIQUE>
        ConstLockedPtr;

#ifdef BSLS_COMPILERFEATURES_GUARANTEED_COPY_ELISION
    // MANIPULATORS

    // Acquire a lock
    LockedPtr      lock();
    ConstLockedPtr lock() const;
#endif
};

/// A utility class that associates a mutex directly with a value.
template <typename t_VALUE, typename t_MUTEX = bslmt::ReaderWriterMutex>
class SynchronizedValue
: public SynchronizedValue_Base<
      SynchronizedValue<t_VALUE, t_MUTEX>,
      SynchronizedValue_MutexTraits<t_MUTEX>::k_LOCK_POLICY> {
  public:
    // TYPES

    typedef t_MUTEX MutexType;
    typedef t_VALUE ValueType;

  private:
    // DATA

    ValueType         d_value;
    mutable MutexType d_mutex;

    // FRIENDS
    template <typename t_SYNCHRONIZED, int t_POLICY>
    friend class SynchronizedValue_LockedPtr;

  public:
    // CREATORS

    /// Construct this object by copying value
    SynchronizedValue(const ValueType& value);

    /// Construct this object by moving value
    SynchronizedValue(bslmf::MovableRef<ValueType> value);
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

// struct SynchronizedValue_LockUtil

template <typename t_MUTEX>
inline void SynchronizedValue_LockUtil::lockUnique(t_MUTEX* lock)
{
    BSLS_ASSERT(lock);
    lock->lock();
}

/// This is a specialization for ReaderWriterMutex, which doesn't support
/// lock() as a way to acquire unique ownership of the lock.
template <>
inline void SynchronizedValue_LockUtil::lockUnique<bslmt::ReaderWriterMutex>(
    bslmt::ReaderWriterMutex* lock)
{
    BSLS_ASSERT(lock);
    lock->lockWrite();
}

template <typename t_MUTEX>
inline void SynchronizedValue_LockUtil::unlockUnique(t_MUTEX* unlock)
{
    BSLS_ASSERT(unlock);
    unlock->unlock();
}

/// This is a specialization for ReaderWriterMutex, which has a more optimal
/// unlockWrite()
template <>
inline void SynchronizedValue_LockUtil::unlockUnique<bslmt::ReaderWriterMutex>(
    bslmt::ReaderWriterMutex* unlock)
{
    BSLS_ASSERT(unlock);
    unlock->unlockWrite();
}

// class SynchronizedValue_SharedLock

template <typename t_MUTEX>
inline SynchronizedValue_SharedLock<t_MUTEX>::SynchronizedValue_SharedLock()
: d_mutex_p(NULL)
, d_isOwned(false)
{
}

template <typename t_MUTEX>
inline SynchronizedValue_SharedLock<t_MUTEX>::SynchronizedValue_SharedLock(
    MutexType* mutex)
: d_mutex_p(mutex)
, d_isOwned(true)
{
    BSLS_ASSERT(mutex);
    d_mutex_p->lockRead();
}

template <typename t_MUTEX>
inline SynchronizedValue_SharedLock<t_MUTEX>::SynchronizedValue_SharedLock(
    bslmf::MovableRef<SynchronizedValue_SharedLock> other)
    BSLS_KEYWORD_NOEXCEPT : d_mutex_p(NULL),
                            d_isOwned(false)
{
    this->swap(other);
}

template <typename t_MUTEX>
inline SynchronizedValue_SharedLock<t_MUTEX>&
SynchronizedValue_SharedLock<t_MUTEX>::operator=(
    bslmf::MovableRef<SynchronizedValue_SharedLock> other)
    BSLS_KEYWORD_NOEXCEPT
{
    SynchronizedValue_SharedLock(bslmf::MovableRefUtil::move(other))
        .swap(*this);
    return *this;
}

template <typename t_MUTEX>
inline SynchronizedValue_SharedLock<t_MUTEX>::~SynchronizedValue_SharedLock()
    BSLS_KEYWORD_NOEXCEPT
{
    if (d_isOwned) {
        d_mutex_p->unlockRead();
    }
}

template <typename t_MUTEX>
inline typename SynchronizedValue_SharedLock<t_MUTEX>::MutexType*
SynchronizedValue_SharedLock<t_MUTEX>::mutex() const BSLS_KEYWORD_NOEXCEPT
{
    return d_mutex_p;
}

template <typename t_MUTEX>
inline bool
SynchronizedValue_SharedLock<t_MUTEX>::ownsLock() const BSLS_KEYWORD_NOEXCEPT
{
    return d_isOwned;
}

template <typename t_MUTEX>
inline void SynchronizedValue_SharedLock<t_MUTEX>::lock()
{
    BSLS_ASSERT(d_mutex_p);
    BSLS_ASSERT(!d_isOwned);
    d_mutex_p->lockRead();
    d_isOwned = true;
}

template <typename t_MUTEX>
inline void SynchronizedValue_SharedLock<t_MUTEX>::unlock()
{
    BSLS_ASSERT(d_isOwned);
    d_mutex_p->unlockRead();
    d_isOwned = false;
}

template <typename t_MUTEX>
inline typename SynchronizedValue_SharedLock<t_MUTEX>::MutexType*
SynchronizedValue_SharedLock<t_MUTEX>::release() BSLS_KEYWORD_NOEXCEPT
{
    d_isOwned        = false;
    MutexType* mutex = NULL;
    bsl::swap(mutex, d_mutex_p);
    return mutex;
}

template <typename t_MUTEX>
inline void SynchronizedValue_SharedLock<t_MUTEX>::swap(
    SynchronizedValue_SharedLock& other) BSLS_KEYWORD_NOEXCEPT
{
    bsl::swap(d_mutex_p, other.d_mutex_p);
    bsl::swap(d_isOwned, other.d_isOwned);
}

// class SynchronizedValue_UniqueLock

template <typename t_MUTEX>
inline SynchronizedValue_UniqueLock<t_MUTEX>::SynchronizedValue_UniqueLock()
: d_mutex_p(NULL)
, d_isOwned(false)
{
}

template <typename t_MUTEX>
inline SynchronizedValue_UniqueLock<t_MUTEX>::SynchronizedValue_UniqueLock(
    MutexType* mutex)
: d_mutex_p(mutex)
, d_isOwned(true)
{
    BSLS_ASSERT(mutex);
    SynchronizedValue_LockUtil::lockUnique(mutex);
}

template <typename t_MUTEX>
inline SynchronizedValue_UniqueLock<t_MUTEX>::SynchronizedValue_UniqueLock(
    bslmf::MovableRef<SynchronizedValue_UniqueLock> other)
    BSLS_KEYWORD_NOEXCEPT : d_mutex_p(NULL),
                            d_isOwned(false)
{
    this->swap(other);
}

template <typename t_MUTEX>
inline SynchronizedValue_UniqueLock<t_MUTEX>&
SynchronizedValue_UniqueLock<t_MUTEX>::operator=(
    bslmf::MovableRef<SynchronizedValue_UniqueLock> other)
    BSLS_KEYWORD_NOEXCEPT
{
    SynchronizedValue_UniqueLock(bslmf::MovableRefUtil::move(other))
        .swap(*this);
    return *this;
}

template <typename t_MUTEX>
inline SynchronizedValue_UniqueLock<t_MUTEX>::~SynchronizedValue_UniqueLock()
    BSLS_KEYWORD_NOEXCEPT
{
    if (d_isOwned) {
        SynchronizedValue_LockUtil::unlockUnique(d_mutex_p);
    }
}

template <typename t_MUTEX>
inline typename SynchronizedValue_UniqueLock<t_MUTEX>::MutexType*
SynchronizedValue_UniqueLock<t_MUTEX>::mutex() const BSLS_KEYWORD_NOEXCEPT
{
    return d_mutex_p;
}

template <typename t_MUTEX>
inline bool
SynchronizedValue_UniqueLock<t_MUTEX>::ownsLock() const BSLS_KEYWORD_NOEXCEPT
{
    return d_isOwned;
}

template <typename t_MUTEX>
inline void SynchronizedValue_UniqueLock<t_MUTEX>::lock()
{
    BSLS_ASSERT(d_mutex_p);
    BSLS_ASSERT(!d_isOwned);
    SynchronizedValue_LockUtil::lockUnique(d_mutex_p);
    d_isOwned = true;
}

template <typename t_MUTEX>
inline void SynchronizedValue_UniqueLock<t_MUTEX>::unlock()
{
    BSLS_ASSERT(d_isOwned);
    SynchronizedValue_LockUtil::unlockUnique(d_mutex_p);
    d_isOwned = false;
}

template <typename t_MUTEX>
inline typename SynchronizedValue_UniqueLock<t_MUTEX>::MutexType*
SynchronizedValue_UniqueLock<t_MUTEX>::release() BSLS_KEYWORD_NOEXCEPT
{
    d_isOwned        = false;
    MutexType* mutex = NULL;
    bsl::swap(mutex, d_mutex_p);
    return mutex;
}

template <typename t_MUTEX>
inline void SynchronizedValue_UniqueLock<t_MUTEX>::swap(
    SynchronizedValue_UniqueLock& other) BSLS_KEYWORD_NOEXCEPT
{
    bsl::swap(d_mutex_p, other.d_mutex_p);
    bsl::swap(d_isOwned, other.d_isOwned);
}

// class SynchronizedValue_LockedPtr

template <typename t_SYNCHRONIZED, int t_POLICY>
inline SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::
    SynchronizedValue_LockedPtr(SynchronizedType* value)
: d_value_p(value)
, d_lock(&value->d_mutex)
{
    BSLS_ASSERT(value != NULL);
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>&
SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::operator=(
    bslmf::MovableRef<SynchronizedValue_LockedPtr> other) BSLS_KEYWORD_NOEXCEPT
{
    SynchronizedValue_LockedPtr& otherRef = other;
    if (&otherRef == this) {
        return *this;
    }

    SynchronizedValue_LockedPtr(bslmf::MovableRefUtil::move(other))
        .swap(*this);

    return *this;
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::
    SynchronizedValue_LockedPtr(
        bslmf::MovableRef<SynchronizedValue_LockedPtr> other)
        BSLS_KEYWORD_NOEXCEPT : d_value_p(NULL),
                                d_lock()
{
    this->swap(other);
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline SynchronizedValue_LockedPtr<t_SYNCHRONIZED,
                                   t_POLICY>::~SynchronizedValue_LockedPtr()
    BSLS_KEYWORD_NOEXCEPT
{
    // NOTHING
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline bool
SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::ownsLock() const
{
    return d_lock.ownsLock();
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline typename SynchronizedValue_LockedPtr<t_SYNCHRONIZED,
                                            t_POLICY>::AccessValueType*
SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::operator->() const
{
    BSLS_ASSERT(ownsLock());
    return &d_value_p->d_value;
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline typename SynchronizedValue_LockedPtr<t_SYNCHRONIZED,
                                            t_POLICY>::AccessValueType&
SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::operator*() const
{
    BSLS_ASSERT(ownsLock());
    return d_value_p->d_value;
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline void SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::swap(
    SynchronizedValue_LockedPtr& other) BSLS_KEYWORD_NOEXCEPT
{
    bsl::swap(d_value_p, other.d_value_p);
    bsl::swap(d_lock, other.d_lock);
}

template <typename t_SYNCHRONIZED, int t_POLICY>
inline void SynchronizedValue_LockedPtr<t_SYNCHRONIZED, t_POLICY>::unlock()
    BSLS_KEYWORD_NOEXCEPT
{
    d_value_p = NULL;
    d_lock    = LockType();
}

// class SynchronizedValue_Base

#ifdef BSLS_COMPILERFEATURES_GUARANTEED_COPY_ELISION

template <typename t_SUBCLASS>
inline typename SynchronizedValue_Base<
    t_SUBCLASS,
    SynchronizedValue_LockPolicy::k_SHARED>::ReadLockedPtr
SynchronizedValue_Base<t_SUBCLASS,
                       SynchronizedValue_LockPolicy::k_SHARED>::lockRead()
{
    return ReadLockedPtr(static_cast<t_SUBCLASS*>(this));
}

template <typename t_SUBCLASS>
inline typename SynchronizedValue_Base<
    t_SUBCLASS,
    SynchronizedValue_LockPolicy::k_SHARED>::ConstReadLockedPtr
SynchronizedValue_Base<t_SUBCLASS,
                       SynchronizedValue_LockPolicy::k_SHARED>::lockRead()
    const
{
    return ConstReadLockedPtr(static_cast<const t_SUBCLASS*>(this));
}

template <typename t_SUBCLASS>
inline typename SynchronizedValue_Base<
    t_SUBCLASS,
    SynchronizedValue_LockPolicy::k_SHARED>::WriteLockedPtr
SynchronizedValue_Base<t_SUBCLASS,
                       SynchronizedValue_LockPolicy::k_SHARED>::lockWrite()
{
    return WriteLockedPtr(static_cast<t_SUBCLASS*>(this));
}

template <typename t_SUBCLASS>
inline typename SynchronizedValue_Base<
    t_SUBCLASS,
    SynchronizedValue_LockPolicy::k_SHARED>::ConstWriteLockedPtr
SynchronizedValue_Base<t_SUBCLASS,
                       SynchronizedValue_LockPolicy::k_SHARED>::lockWrite()
    const
{
    return ConstWriteLockedPtr(static_cast<const t_SUBCLASS*>(this));
}

template <typename t_SUBCLASS>
inline typename SynchronizedValue_Base<
    t_SUBCLASS,
    SynchronizedValue_LockPolicy::k_UNIQUE>::LockedPtr
SynchronizedValue_Base<t_SUBCLASS,
                       SynchronizedValue_LockPolicy::k_UNIQUE>::lock()
{
    return LockedPtr(static_cast<t_SUBCLASS*>(this));
}

template <typename t_SUBCLASS>
inline typename SynchronizedValue_Base<
    t_SUBCLASS,
    SynchronizedValue_LockPolicy::k_UNIQUE>::ConstLockedPtr
SynchronizedValue_Base<t_SUBCLASS,
                       SynchronizedValue_LockPolicy::k_UNIQUE>::lock() const
{
    return ConstLockedPtr(static_cast<const t_SUBCLASS*>(this));
}

#endif

// class SynchronizedValue

template <typename t_VALUE, typename t_MUTEX>
inline SynchronizedValue<t_VALUE, t_MUTEX>::SynchronizedValue(
    const ValueType& value)
: d_value(value)
, d_mutex()
{
}

template <typename t_VALUE, typename t_MUTEX>
inline SynchronizedValue<t_VALUE, t_MUTEX>::SynchronizedValue(
    bslmf::MovableRef<ValueType> value)
: d_value(bslmf::MovableRefUtil::move(value))
, d_mutex()
{
}

}
}

#endif
