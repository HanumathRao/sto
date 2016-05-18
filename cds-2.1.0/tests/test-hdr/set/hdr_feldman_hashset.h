/*
    This file is a part of libcds - Concurrent Data Structures library

    (C) Copyright Maxim Khizhinsky (libcds.dev@gmail.com) 2006-2016

    Source code repo: http://github.com/khizmax/libcds/
    Download: http://sourceforge.net/projects/libcds/files/

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CDSTEST_HDR_FELDMAN_HASHSET_H
#define CDSTEST_HDR_FELDMAN_HASHSET_H

#include "cppunit/cppunit_proxy.h"

// forward declaration
namespace cds {
    namespace container {}
    namespace opt {}
}

namespace set {
    namespace cc = cds::container;
    namespace co = cds::opt;

    class FeldmanHashSetHdrTest : public CppUnitMini::TestCase
    {
        template <typename Hash>
        struct Arg
        {
            size_t key;
            Hash hash;

            Arg( size_t k, Hash const& h )
                : key( k )
                , hash( h )
            {}
        };

        template <typename Hash>
        struct Item
        {
            unsigned int nInsertCall;
            unsigned int nFindCall;
            unsigned int nEraseCall;
            mutable unsigned int nIteratorCall;
            Hash hash;
            size_t key;

            Item( size_t k, Hash const& h )
                : nInsertCall(0)
                , nFindCall(0)
                , nEraseCall(0)
                , nIteratorCall(0)
                , hash( h )
                , key( k )
            {}

            explicit Item( Arg<Hash> const& arg )
                : nInsertCall(0)
                , nFindCall(0)
                , nEraseCall(0)
                , nIteratorCall(0)
                , hash( arg.hash )
                , key( arg.key )
            {}

            Item( Item const& i )
                : nInsertCall(0)
                , nFindCall(0)
                , nEraseCall(0)
                , nIteratorCall(0)
                , hash( i.hash )
                , key( i.key )
            {}
        };

        template <typename Hash>
        struct get_hash
        {
            Hash const& operator()( Item<Hash> const& i ) const
            {
                return i.hash;
            }
        };

        template <typename Key>
        struct get_key
        {
            Key operator()(Item<Key> const& i)const
            {
                return i.hash;
            }
        };

        template <typename Key>
        struct nohash {
            Key operator()(Key k) const
            {
                return k;
            }
        };

        struct hash128
        {
            size_t lo;
            size_t hi;

            hash128() {}
            hash128(size_t l, size_t h) : lo(l), hi(h) {}
            hash128( hash128 const& h) : lo(h.lo), hi(h.hi) {}

            struct make {
                hash128 operator()( size_t n ) const
                {
                    return hash128( std::hash<size_t>()( n ), std::hash<size_t>()( ~n ));
                }
                hash128 operator()( hash128 const& n ) const
                {
                    return hash128( std::hash<size_t>()( n.lo ), std::hash<size_t>()( ~n.hi ));
                }
            };

            struct less {
                bool operator()( hash128 const& lhs, hash128 const& rhs ) const
                {
                    if ( lhs.hi != rhs.hi )
                        return lhs.hi < rhs.hi;
                    return lhs.lo < rhs.lo;
                }
            };

            struct cmp {
                int operator()( hash128 const& lhs, hash128 const& rhs ) const
                {
                    if ( lhs.hi != rhs.hi )
                        return lhs.hi < rhs.hi ? -1 : 1;
                    return lhs.lo < rhs.lo ? -1 : lhs.lo == rhs.lo ? 0 : 1;
                }
            };

            friend bool operator==( hash128 const& lhs, hash128 const& rhs )
            {
                return cmp()( lhs, rhs ) == 0;
            }
            friend bool operator!=(hash128 const& lhs, hash128 const& rhs)
            {
                return !( lhs == rhs );
            }
        };

        template <typename Set, typename Hasher>
        void test_hp( size_t nHeadBits, size_t nArrayBits )
        {
            typedef typename Set::hash_type hash_type;
            typedef typename Set::value_type value_type;
            typedef Arg<hash_type> arg_type;
            typedef typename Set::guarded_ptr guarded_ptr;

            Hasher hasher;

            size_t const capacity = 1000;

            Set s( nHeadBits, nArrayBits );
            CPPUNIT_MSG("Array size: head=" << s.head_size() << ", array_node=" << s.array_node_size());
            CPPUNIT_ASSERT(s.head_size() >= (size_t(1) << nHeadBits));
            CPPUNIT_ASSERT(s.array_node_size() == (size_t(1) << nArrayBits));

            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT(s.size() == 0);

            // insert test
            for ( size_t i = 0; i < capacity; ++i ) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT( !s.contains( h ));
                CPPUNIT_ASSERT( s.insert( value_type( i, h )));
                CPPUNIT_ASSERT(s.contains( h ));

                CPPUNIT_ASSERT( !s.empty() );
                CPPUNIT_ASSERT( s.size() == i + 1);

                CPPUNIT_ASSERT( !s.insert( arg_type(i, h) ));
                CPPUNIT_ASSERT( s.size() == i + 1);
            }

            // update existing test
            for ( size_t i = 0; i < capacity; ++i ) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT( s.contains( h ));
                std::pair<bool, bool> ret = s.update( arg_type( i, h ),
                    [](value_type& i, value_type * prev ) {
                        CPPUNIT_ASSERT_CURRENT( prev != nullptr );
                        CPPUNIT_ASSERT_CURRENT( i.key == prev->key );
                        CPPUNIT_ASSERT_CURRENT( i.hash == prev->hash );
                        i.nInsertCall += 1;
                    }, false );
                CPPUNIT_ASSERT( ret.first );
                CPPUNIT_ASSERT( !ret.second );
                CPPUNIT_ASSERT( s.contains( h ));
                CPPUNIT_ASSERT( s.size() == capacity );

                guarded_ptr gp(s.get( h ));
                CPPUNIT_ASSERT( gp );
                CPPUNIT_ASSERT( gp->nInsertCall == 1 );
                CPPUNIT_ASSERT( gp->key == i );
                CPPUNIT_ASSERT( gp->hash == h );
            }

            // erase test
            for ( size_t i = 0; i < capacity; ++i ) {
                CPPUNIT_ASSERT( !s.empty() );
                CPPUNIT_ASSERT( s.size() == capacity - i );
                CPPUNIT_ASSERT(s.find(hasher(i), []( value_type &) {}));
                CPPUNIT_ASSERT( s.erase(hasher(i)) );
                CPPUNIT_ASSERT( !s.find(hasher(i), []( value_type &) {}));
                CPPUNIT_ASSERT( s.size() == capacity - i - 1);
            }
            CPPUNIT_ASSERT( s.empty() );

            // Iterators on empty set
            CPPUNIT_ASSERT(s.begin() == s.end());
            CPPUNIT_ASSERT(s.cbegin() == s.cend());
            CPPUNIT_ASSERT(s.rbegin() == s.rend());
            CPPUNIT_ASSERT(s.crbegin() == s.crend());

            // insert with functor
            for ( size_t i = capacity; i > 0; --i ) {
                CPPUNIT_ASSERT( s.size() == capacity - i );
                CPPUNIT_ASSERT(s.insert( arg_type( i, hasher(i)), []( value_type& val ) { val.nInsertCall += 1; } ));
                CPPUNIT_ASSERT( s.size() == capacity - i + 1 );
                CPPUNIT_ASSERT( !s.empty() );

                CPPUNIT_ASSERT(s.find( hasher(i), []( value_type& val ) {
                    CPPUNIT_ASSERT_CURRENT( val.nInsertCall == 1 );
                    val.nFindCall += 1;
                } ));
            }
                CPPUNIT_ASSERT( s.size() == capacity );

            // for-each iterator test
            for ( auto& el : s ) {
                CPPUNIT_ASSERT( el.nInsertCall == 1 );
                CPPUNIT_ASSERT( el.nFindCall == 1 );
                el.nFindCall += 1;
            }

            // iterator test
            for ( auto it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it->nInsertCall == 1 );
                CPPUNIT_ASSERT( it->nFindCall == 2 );
                it->nFindCall += 1;
            }

            // reverse iterator test
            for ( auto it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it->nInsertCall == 1 );
                CPPUNIT_ASSERT( it->nFindCall == 3 );
                it->nFindCall += 1;
            }

            // const iterator test
            for ( auto it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it->nInsertCall == 1 );
                CPPUNIT_ASSERT( it->nFindCall == 4 );
                it->nIteratorCall += 1;
            }

            // const reverse iterator test
            for ( auto it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it->nInsertCall == 1 );
                CPPUNIT_ASSERT( it->nFindCall == 4 );
                CPPUNIT_ASSERT( it->nIteratorCall == 1 );
                it->nIteratorCall += 1;
            }

            // check completeness
            for ( size_t i = 1; i <= capacity; ++i ) {
                CPPUNIT_ASSERT( s.find( hasher( i ), []( value_type const& el ) {
                    CPPUNIT_ASSERT_CURRENT( el.nInsertCall == 1 );
                    CPPUNIT_ASSERT_CURRENT( el.nFindCall == 4 );
                    CPPUNIT_ASSERT_CURRENT( el.nIteratorCall == 2 );
                } ));
            }

            // erase with functor test
            {
                size_t nSum = 0;
                for ( size_t i = 1; i <= capacity; ++i ) {
                    CPPUNIT_ASSERT( s.size() == capacity - i + 1 );
                    CPPUNIT_ASSERT(s.erase(hasher(i), [&nSum]( value_type const& val ) {
                        CPPUNIT_ASSERT_CURRENT( val.nInsertCall == 1 );
                        CPPUNIT_ASSERT_CURRENT( val.nFindCall == 4 );
                        CPPUNIT_ASSERT_CURRENT( val.nIteratorCall == 2 );
                        nSum += val.key;
                    } ))
                    CPPUNIT_ASSERT( s.size() == capacity - i );
                    CPPUNIT_ASSERT( !s.erase(hasher(i), [&nSum]( value_type const& val ) { nSum += val.key; } ))
                }
                CPPUNIT_ASSERT(s.empty() );
                CPPUNIT_ASSERT(nSum == (1 + capacity) * capacity / 2 );
            }

            // update test with insert allowing
            for ( size_t i = 0; i < capacity; ++i ) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT( !s.contains( h ));
                guarded_ptr gp(s.get( h ));
                CPPUNIT_ASSERT( !gp );
                std::pair<bool, bool> ret = s.update( arg_type( i, h ),
                    [](value_type& i, value_type * prev ) {
                        CPPUNIT_ASSERT_CURRENT( prev == nullptr );
                        i.nInsertCall += 1;
                    });
                CPPUNIT_ASSERT( ret.first );
                CPPUNIT_ASSERT( ret.second );
                CPPUNIT_ASSERT( s.contains( h ));
                CPPUNIT_ASSERT( s.size() == i + 1 );

                gp = s.get( h );
                CPPUNIT_ASSERT( gp );
                CPPUNIT_ASSERT( gp->nInsertCall == 1 );
                CPPUNIT_ASSERT( gp->key == i );
                CPPUNIT_ASSERT( gp->hash == h );
            }
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT(s.size() == capacity );

            // erase_at( iterator ) test
            for ( auto it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( s.erase_at( it ));
            }
            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT( s.size() == 0 );

            // emplace test
            for ( size_t i = 0; i < capacity; ++i ) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT( !s.contains( h ));
                CPPUNIT_ASSERT( s.emplace( i, hasher(i) ));
                CPPUNIT_ASSERT(s.contains( h ));

                CPPUNIT_ASSERT( !s.empty() );
                CPPUNIT_ASSERT( s.size() == i + 1);

                CPPUNIT_ASSERT( !s.emplace( arg_type(i, h) ));
                CPPUNIT_ASSERT( s.size() == i + 1);
            }
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT(s.size() == capacity );

            // erase_at( reverse_iterator ) test
            for ( auto it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( s.erase_at( it ));
            }
            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT( s.size() == 0 );

            // extract test
            for ( size_t i = 0; i < capacity; ++i ) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT( !s.contains( h ));
                CPPUNIT_ASSERT( s.emplace( arg_type( i, hasher(i) )));
                CPPUNIT_ASSERT(s.contains( h ));

                CPPUNIT_ASSERT( !s.empty() );
                CPPUNIT_ASSERT( s.size() == i + 1);

                CPPUNIT_ASSERT( !s.emplace( i, h ));
                CPPUNIT_ASSERT( s.size() == i + 1);
            }
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT(s.size() == capacity );

            for ( size_t i = capacity; i != 0; --i ) {
                CPPUNIT_ASSERT( !s.empty() );
                CPPUNIT_ASSERT( s.size() == i );

                guarded_ptr gp{ s.extract( hasher(i-1)) };
                CPPUNIT_ASSERT( gp );
                CPPUNIT_ASSERT( gp->key == i - 1);
                CPPUNIT_ASSERT(gp->hash == hasher(i-1));
                CPPUNIT_ASSERT( !s.contains(hasher(i-1)));

                gp = s.get(hasher(i-1));
                CPPUNIT_ASSERT( !gp );

                CPPUNIT_ASSERT( s.size() == i - 1 );
            }
            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT(s.size() == 0 );

            // clear test
            for ( size_t i = 0; i < capacity; ++i ) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT( !s.contains( h ));
                CPPUNIT_ASSERT( s.emplace( arg_type( i, hasher(i) )));
                CPPUNIT_ASSERT(s.contains( h ));

                CPPUNIT_ASSERT( !s.empty() );
                CPPUNIT_ASSERT( s.size() == i + 1);

                CPPUNIT_ASSERT( !s.emplace( i, h ));
                CPPUNIT_ASSERT( s.size() == i + 1);
            }
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT(s.size() == capacity );

            s.clear();
            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT(s.size() == 0 );

            CPPUNIT_MSG( s.statistics() );
        }

        template <typename Set, typename Hasher>
        void test_rcu(size_t nHeadBits, size_t nArrayBits)
        {
            typedef typename Set::hash_type hash_type;
            typedef typename Set::value_type value_type;
            typedef Arg<hash_type> arg_type;
            typedef typename Set::exempt_ptr exempt_ptr;
            typedef typename Set::rcu_lock rcu_lock;

            Hasher hasher;

            size_t const capacity = 1000;

            Set s(nHeadBits, nArrayBits);
            CPPUNIT_MSG("Array size: head=" << s.head_size() << ", array_node=" << s.array_node_size());
            CPPUNIT_ASSERT(s.head_size() >= (size_t(1) << nHeadBits));
            CPPUNIT_ASSERT(s.array_node_size() == (size_t(1) << nArrayBits));

            CPPUNIT_ASSERT(s.empty());
            CPPUNIT_ASSERT(s.size() == 0);

            // insert test
            for (size_t i = 0; i < capacity; ++i) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT(!s.contains(h));
                CPPUNIT_ASSERT(s.insert(value_type(i, h)));
                CPPUNIT_ASSERT(s.contains(h));

                CPPUNIT_ASSERT(!s.empty());
                CPPUNIT_ASSERT(s.size() == i + 1);

                CPPUNIT_ASSERT(!s.insert(arg_type(i, h)));
                CPPUNIT_ASSERT(s.size() == i + 1);
            }

            // update existing test
            for (size_t i = 0; i < capacity; ++i) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT(s.contains(h));
                std::pair<bool, bool> ret = s.update(arg_type(i, h),
                    [](value_type& i, value_type * prev) {
                    CPPUNIT_ASSERT_CURRENT(prev != nullptr);
                    CPPUNIT_ASSERT_CURRENT(i.key == prev->key);
                    CPPUNIT_ASSERT_CURRENT(i.hash == prev->hash);
                    i.nInsertCall += 1;
                }, false);
                CPPUNIT_ASSERT(ret.first);
                CPPUNIT_ASSERT(!ret.second);
                CPPUNIT_ASSERT(s.contains(h));
                CPPUNIT_ASSERT(s.size() == capacity);

                {
                    rcu_lock l;
                    value_type * p = s.get(h);
                    CPPUNIT_ASSERT(p);
                    CPPUNIT_ASSERT(p->nInsertCall == 1);
                    CPPUNIT_ASSERT(p->key == i);
                    CPPUNIT_ASSERT(p->hash == h);
                }
            }

            // erase test
            for (size_t i = 0; i < capacity; ++i) {
                CPPUNIT_ASSERT(!s.empty());
                CPPUNIT_ASSERT(s.size() == capacity - i);
                CPPUNIT_ASSERT(s.find(hasher(i), [](value_type &) {}));
                CPPUNIT_ASSERT(s.erase(hasher(i)));
                CPPUNIT_ASSERT(!s.find(hasher(i), [](value_type &) {}));
                CPPUNIT_ASSERT(s.size() == capacity - i - 1);
            }
            CPPUNIT_ASSERT(s.empty());

            // Iterators on empty set
            {
                rcu_lock l;
                CPPUNIT_ASSERT(s.begin() == s.end());
                CPPUNIT_ASSERT(s.cbegin() == s.cend());
                CPPUNIT_ASSERT(s.rbegin() == s.rend());
                CPPUNIT_ASSERT(s.crbegin() == s.crend());
            }

            // insert with functor
            for (size_t i = capacity; i > 0; --i) {
                CPPUNIT_ASSERT(s.size() == capacity - i);
                CPPUNIT_ASSERT(s.insert(arg_type(i, hasher(i)), [](value_type& val) { val.nInsertCall += 1; }));
                CPPUNIT_ASSERT(s.size() == capacity - i + 1);
                CPPUNIT_ASSERT(!s.empty());

                CPPUNIT_ASSERT(s.find(hasher(i), [](value_type& val) {
                    CPPUNIT_ASSERT_CURRENT(val.nInsertCall == 1);
                    val.nFindCall += 1;
                }));
            }
            CPPUNIT_ASSERT(s.size() == capacity);

            // for-each iterator test
            {
                rcu_lock l;
                for (auto& el : s) {
                    CPPUNIT_ASSERT(el.nInsertCall == 1);
                    CPPUNIT_ASSERT(el.nFindCall == 1);
                    el.nFindCall += 1;
                }
            }

            // iterator test
            {
                rcu_lock l;
                for (auto it = s.begin(), itEnd = s.end(); it != itEnd; ++it) {
                    CPPUNIT_ASSERT(it->nInsertCall == 1);
                    CPPUNIT_ASSERT(it->nFindCall == 2);
                    it->nFindCall += 1;
                }
            }

            // reverse iterator test
            {
                rcu_lock l;
                for (auto it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it) {
                    CPPUNIT_ASSERT(it->nInsertCall == 1);
                    CPPUNIT_ASSERT(it->nFindCall == 3);
                    it->nFindCall += 1;
                }
            }

            // const iterator test
            {
                rcu_lock l;
                for (auto it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it) {
                    CPPUNIT_ASSERT(it->nInsertCall == 1);
                    CPPUNIT_ASSERT(it->nFindCall == 4);
                    it->nIteratorCall += 1;
                }
            }

            // const reverse iterator test
            {
                rcu_lock l;
                for (auto it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it) {
                    CPPUNIT_ASSERT(it->nInsertCall == 1);
                    CPPUNIT_ASSERT(it->nFindCall == 4);
                    CPPUNIT_ASSERT(it->nIteratorCall == 1);
                    it->nIteratorCall += 1;
                }
            }

            // check completeness
            for (size_t i = 1; i <= capacity; ++i) {
                CPPUNIT_ASSERT(s.find(hasher(i), [](value_type const& el) {
                    CPPUNIT_ASSERT_CURRENT(el.nInsertCall == 1);
                    CPPUNIT_ASSERT_CURRENT(el.nFindCall == 4);
                    CPPUNIT_ASSERT_CURRENT(el.nIteratorCall == 2);
                }));
            }

            // erase with functor test
            {
                size_t nSum = 0;
                for (size_t i = 1; i <= capacity; ++i) {
                    CPPUNIT_ASSERT(s.size() == capacity - i + 1);
                    CPPUNIT_ASSERT(s.erase(hasher(i), [&nSum](value_type const& val) {
                        CPPUNIT_ASSERT_CURRENT(val.nInsertCall == 1);
                        CPPUNIT_ASSERT_CURRENT(val.nFindCall == 4);
                        CPPUNIT_ASSERT_CURRENT(val.nIteratorCall == 2);
                        nSum += val.key;
                    }));
                    CPPUNIT_ASSERT(s.size() == capacity - i);
                    CPPUNIT_ASSERT(!s.erase(hasher(i), [&nSum](value_type const& val) { nSum += val.key; }))
                }
                CPPUNIT_ASSERT(s.empty());
                CPPUNIT_ASSERT(nSum == (1 + capacity) * capacity / 2);
            }

            // update test with insert allowing
            for (size_t i = 0; i < capacity; ++i) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT(!s.contains(h));

                {
                    rcu_lock l;
                    value_type * p = s.get(h);
                    CPPUNIT_ASSERT(!p);
                }
                std::pair<bool, bool> ret = s.update(arg_type(i, h),
                    [](value_type& i, value_type * prev) {
                    CPPUNIT_ASSERT_CURRENT(prev == nullptr);
                    i.nInsertCall += 1;
                });
                CPPUNIT_ASSERT(ret.first);
                CPPUNIT_ASSERT(ret.second);
                CPPUNIT_ASSERT(s.contains(h));
                CPPUNIT_ASSERT(s.size() == i + 1);

                {
                    rcu_lock l;
                    value_type * p = s.get(h);
                    CPPUNIT_ASSERT(p);
                    CPPUNIT_ASSERT(p->nInsertCall == 1);
                    CPPUNIT_ASSERT(p->key == i);
                    CPPUNIT_ASSERT(p->hash == h);
                }
            }
            CPPUNIT_ASSERT(!s.empty());
            CPPUNIT_ASSERT(s.size() == capacity);

            s.clear();
            CPPUNIT_ASSERT(s.empty());
            CPPUNIT_ASSERT(s.size() == 0);

            // emplace test
            for (size_t i = 0; i < capacity; ++i) {
                hash_type h = hasher(i);
                CPPUNIT_ASSERT(!s.contains(h));
                CPPUNIT_ASSERT(s.emplace(i, hasher(i)));
                CPPUNIT_ASSERT(s.contains(h));

                CPPUNIT_ASSERT(!s.empty());
                CPPUNIT_ASSERT(s.size() == i + 1);

                CPPUNIT_ASSERT(!s.emplace(arg_type(i, h)));
                CPPUNIT_ASSERT(s.size() == i + 1);
            }
            CPPUNIT_ASSERT(!s.empty());
            CPPUNIT_ASSERT(s.size() == capacity);

            // extract test
            for (size_t i = capacity; i != 0; --i) {
                CPPUNIT_ASSERT(!s.empty());
                CPPUNIT_ASSERT(s.size() == i);

                exempt_ptr gp{ s.extract(hasher(i - 1)) };
                CPPUNIT_ASSERT(gp);
                CPPUNIT_ASSERT(gp->key == i - 1);
                CPPUNIT_ASSERT(gp->hash == hasher(i - 1));
                CPPUNIT_ASSERT(!s.contains(hasher(i - 1)));

                {
                    rcu_lock l;
                    value_type * p = s.get(hasher(i - 1));
                    CPPUNIT_ASSERT( p == nullptr );
                }
                CPPUNIT_ASSERT(s.size() == i - 1);
            }
            CPPUNIT_ASSERT(s.empty());
            CPPUNIT_ASSERT(s.size() == 0);

            CPPUNIT_MSG(s.statistics());
        }

        void hp_nohash();
        void hp_nohash_stat();
        void hp_nohash_5_3();
        void hp_nohash_5_3_stat();
        void hp_stdhash();
        void hp_stdhash_stat();
        void hp_stdhash_5_3();
        void hp_stdhash_5_3_stat();
        void hp_hash128();
        void hp_hash128_stat();
        void hp_hash128_4_3();
        void hp_hash128_4_3_stat();

        void dhp_nohash();
        void dhp_nohash_stat();
        void dhp_nohash_5_3();
        void dhp_nohash_5_3_stat();
        void dhp_stdhash();
        void dhp_stdhash_stat();
        void dhp_stdhash_5_3();
        void dhp_stdhash_5_3_stat();
        void dhp_hash128();
        void dhp_hash128_stat();
        void dhp_hash128_4_3();
        void dhp_hash128_4_3_stat();

        void rcu_gpi_nohash();
        void rcu_gpi_nohash_stat();
        void rcu_gpi_nohash_5_3();
        void rcu_gpi_nohash_5_3_stat();
        void rcu_gpi_stdhash();
        void rcu_gpi_stdhash_stat();
        void rcu_gpi_stdhash_5_3();
        void rcu_gpi_stdhash_5_3_stat();
        void rcu_gpi_hash128();
        void rcu_gpi_hash128_stat();
        void rcu_gpi_hash128_4_3();
        void rcu_gpi_hash128_4_3_stat();

        void rcu_gpb_nohash();
        void rcu_gpb_nohash_stat();
        void rcu_gpb_nohash_5_3();
        void rcu_gpb_nohash_5_3_stat();
        void rcu_gpb_stdhash();
        void rcu_gpb_stdhash_stat();
        void rcu_gpb_stdhash_5_3();
        void rcu_gpb_stdhash_5_3_stat();
        void rcu_gpb_hash128();
        void rcu_gpb_hash128_stat();
        void rcu_gpb_hash128_4_3();
        void rcu_gpb_hash128_4_3_stat();

        void rcu_gpt_nohash();
        void rcu_gpt_nohash_stat();
        void rcu_gpt_nohash_5_3();
        void rcu_gpt_nohash_5_3_stat();
        void rcu_gpt_stdhash();
        void rcu_gpt_stdhash_stat();
        void rcu_gpt_stdhash_5_3();
        void rcu_gpt_stdhash_5_3_stat();
        void rcu_gpt_hash128();
        void rcu_gpt_hash128_stat();
        void rcu_gpt_hash128_4_3();
        void rcu_gpt_hash128_4_3_stat();

        void rcu_shb_nohash();
        void rcu_shb_nohash_stat();
        void rcu_shb_nohash_5_3();
        void rcu_shb_nohash_5_3_stat();
        void rcu_shb_stdhash();
        void rcu_shb_stdhash_stat();
        void rcu_shb_stdhash_5_3();
        void rcu_shb_stdhash_5_3_stat();
        void rcu_shb_hash128();
        void rcu_shb_hash128_stat();
        void rcu_shb_hash128_4_3();
        void rcu_shb_hash128_4_3_stat();

        void rcu_sht_nohash();
        void rcu_sht_nohash_stat();
        void rcu_sht_nohash_5_3();
        void rcu_sht_nohash_5_3_stat();
        void rcu_sht_stdhash();
        void rcu_sht_stdhash_stat();
        void rcu_sht_stdhash_5_3();
        void rcu_sht_stdhash_5_3_stat();
        void rcu_sht_hash128();
        void rcu_sht_hash128_stat();
        void rcu_sht_hash128_4_3();
        void rcu_sht_hash128_4_3_stat();

        CPPUNIT_TEST_SUITE(FeldmanHashSetHdrTest)
            CPPUNIT_TEST(hp_nohash)
            CPPUNIT_TEST(hp_nohash_stat)
            CPPUNIT_TEST(hp_nohash_5_3)
            CPPUNIT_TEST(hp_nohash_5_3_stat)
            CPPUNIT_TEST(hp_stdhash)
            CPPUNIT_TEST(hp_stdhash_stat)
            CPPUNIT_TEST(hp_stdhash_5_3)
            CPPUNIT_TEST(hp_stdhash_5_3_stat)
            CPPUNIT_TEST(hp_hash128)
            CPPUNIT_TEST(hp_hash128_stat)
            CPPUNIT_TEST(hp_hash128_4_3)
            CPPUNIT_TEST(hp_hash128_4_3_stat)

            CPPUNIT_TEST(dhp_nohash)
            CPPUNIT_TEST(dhp_nohash_stat)
            CPPUNIT_TEST(dhp_nohash_5_3)
            CPPUNIT_TEST(dhp_nohash_5_3_stat)
            CPPUNIT_TEST(dhp_stdhash)
            CPPUNIT_TEST(dhp_stdhash_stat)
            CPPUNIT_TEST(dhp_stdhash_5_3)
            CPPUNIT_TEST(dhp_stdhash_5_3_stat)
            CPPUNIT_TEST(dhp_hash128)
            CPPUNIT_TEST(dhp_hash128_stat)
            CPPUNIT_TEST(dhp_hash128_4_3)
            CPPUNIT_TEST(dhp_hash128_4_3_stat)

            CPPUNIT_TEST(rcu_gpi_nohash)
            CPPUNIT_TEST(rcu_gpi_nohash_stat)
            CPPUNIT_TEST(rcu_gpi_nohash_5_3)
            CPPUNIT_TEST(rcu_gpi_nohash_5_3_stat)
            CPPUNIT_TEST(rcu_gpi_stdhash)
            CPPUNIT_TEST(rcu_gpi_stdhash_stat)
            CPPUNIT_TEST(rcu_gpi_stdhash_5_3)
            CPPUNIT_TEST(rcu_gpi_stdhash_5_3_stat)
            CPPUNIT_TEST(rcu_gpi_hash128)
            CPPUNIT_TEST(rcu_gpi_hash128_stat)
            CPPUNIT_TEST(rcu_gpi_hash128_4_3)
            CPPUNIT_TEST(rcu_gpi_hash128_4_3_stat)

            CPPUNIT_TEST(rcu_gpb_nohash)
            CPPUNIT_TEST(rcu_gpb_nohash_stat)
            CPPUNIT_TEST(rcu_gpb_nohash_5_3)
            CPPUNIT_TEST(rcu_gpb_nohash_5_3_stat)
            CPPUNIT_TEST(rcu_gpb_stdhash)
            CPPUNIT_TEST(rcu_gpb_stdhash_stat)
            CPPUNIT_TEST(rcu_gpb_stdhash_5_3)
            CPPUNIT_TEST(rcu_gpb_stdhash_5_3_stat)
            CPPUNIT_TEST(rcu_gpb_hash128)
            CPPUNIT_TEST(rcu_gpb_hash128_stat)
            CPPUNIT_TEST(rcu_gpb_hash128_4_3)
            CPPUNIT_TEST(rcu_gpb_hash128_4_3_stat)

            CPPUNIT_TEST(rcu_gpt_nohash)
            CPPUNIT_TEST(rcu_gpt_nohash_stat)
            CPPUNIT_TEST(rcu_gpt_nohash_5_3)
            CPPUNIT_TEST(rcu_gpt_nohash_5_3_stat)
            CPPUNIT_TEST(rcu_gpt_stdhash)
            CPPUNIT_TEST(rcu_gpt_stdhash_stat)
            CPPUNIT_TEST(rcu_gpt_stdhash_5_3)
            CPPUNIT_TEST(rcu_gpt_stdhash_5_3_stat)
            CPPUNIT_TEST(rcu_gpt_hash128)
            CPPUNIT_TEST(rcu_gpt_hash128_stat)
            CPPUNIT_TEST(rcu_gpt_hash128_4_3)
            CPPUNIT_TEST(rcu_gpt_hash128_4_3_stat)

            CPPUNIT_TEST(rcu_shb_nohash)
            CPPUNIT_TEST(rcu_shb_nohash_stat)
            CPPUNIT_TEST(rcu_shb_nohash_5_3)
            CPPUNIT_TEST(rcu_shb_nohash_5_3_stat)
            CPPUNIT_TEST(rcu_shb_stdhash)
            CPPUNIT_TEST(rcu_shb_stdhash_stat)
            CPPUNIT_TEST(rcu_shb_stdhash_5_3)
            CPPUNIT_TEST(rcu_shb_stdhash_5_3_stat)
            CPPUNIT_TEST(rcu_shb_hash128)
            CPPUNIT_TEST(rcu_shb_hash128_stat)
            CPPUNIT_TEST(rcu_shb_hash128_4_3)
            CPPUNIT_TEST(rcu_shb_hash128_4_3_stat)

            CPPUNIT_TEST(rcu_sht_nohash)
            CPPUNIT_TEST(rcu_sht_nohash_stat)
            CPPUNIT_TEST(rcu_sht_nohash_5_3)
            CPPUNIT_TEST(rcu_sht_nohash_5_3_stat)
            CPPUNIT_TEST(rcu_sht_stdhash)
            CPPUNIT_TEST(rcu_sht_stdhash_stat)
            CPPUNIT_TEST(rcu_sht_stdhash_5_3)
            CPPUNIT_TEST(rcu_sht_stdhash_5_3_stat)
            CPPUNIT_TEST(rcu_sht_hash128)
            CPPUNIT_TEST(rcu_sht_hash128_stat)
            CPPUNIT_TEST(rcu_sht_hash128_4_3)
            CPPUNIT_TEST(rcu_sht_hash128_4_3_stat)
        CPPUNIT_TEST_SUITE_END()
    };

} // namespace set

#endif // #ifndef CDSTEST_HDR_FELDMAN_HASHSET_H
