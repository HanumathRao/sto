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

#ifndef CDSTEST_HDR_INTRUSIVE_FELDMAN_HASHSET_H
#define CDSTEST_HDR_INTRUSIVE_FELDMAN_HASHSET_H

#include "cppunit/cppunit_proxy.h"

// forward declaration
namespace cds {
    namespace intrusive {}
    namespace opt {}
}

namespace set {
    namespace ci = cds::intrusive;
    namespace co = cds::opt;

    class IntrusiveFeldmanHashSetHdrTest: public CppUnitMini::TestCase
    {
        template <typename Hash>
        struct Item
        {
            unsigned int nDisposeCount  ;   // count of disposer calling
            Hash hash;
            unsigned int nInsertCall;
            unsigned int nFindCall;
            unsigned int nEraseCall;
            mutable unsigned int nIteratorCall;

            Item()
                : nDisposeCount(0)
                , nInsertCall(0)
                , nFindCall(0)
                , nEraseCall(0)
                , nIteratorCall(0)
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
            Key const& operator()(Item<Key> const& i) const
            {
                return i.hash;
            }
        };

        struct item_disposer {
            template <typename Hash>
            void operator()( Item<Hash> * p )
            {
                ++p->nDisposeCount;
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
        };


        template <typename Set, typename Hash>
        void test_hp( size_t nHeadBits, size_t nArrayBits )
        {
            typedef typename Set::hash_type hash_type;
            typedef typename Set::value_type value_type;

            Hash hasher;

            size_t const arrCapacity = 1000;
            std::vector< value_type > arrValue;
            arrValue.reserve( arrCapacity );
            for ( size_t i = 0; i < arrCapacity; ++i ) {
                arrValue.emplace_back( value_type() );
                arrValue.back().hash = hasher( i );
            }
            CPPUNIT_ASSERT( arrValue.size() == arrCapacity );

            Set s( nHeadBits, nArrayBits );
            CPPUNIT_MSG("Array size: head=" << s.head_size() << ", array_node=" << s.array_node_size());
            CPPUNIT_ASSERT(s.head_size() >= (size_t(1) << nHeadBits));
            CPPUNIT_ASSERT(s.array_node_size() == (size_t(1) << nArrayBits));

            // insert() test
            CPPUNIT_ASSERT(s.size() == 0 );
            CPPUNIT_ASSERT(s.empty() );
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT( s.insert( el ));
                CPPUNIT_ASSERT(s.contains( el.hash ));
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity );
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT(s.contains( el.hash ));
                CPPUNIT_ASSERT( !s.insert( el ) );
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity );
            CPPUNIT_ASSERT( !s.empty() );

            // Iterator test
            {
                typedef typename Set::iterator iterator;
                for ( iterator it = s.begin(), itEnd = s.end(); it != itEnd; ++it )
                    ++(it->nIteratorCall);
                for ( auto& el : arrValue ) {
                    CPPUNIT_ASSERT( el.nIteratorCall == 1 );
                    el.nIteratorCall = 0;
                }
            }

            {
                // Const iterator test
                for ( typename Set::const_iterator it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it )
                    (*it).nIteratorCall += 1;
                for ( auto& el : arrValue ) {
                    CPPUNIT_ASSERT( el.nIteratorCall == 1 );
                    el.nIteratorCall = 0;
                }
            }

            {
                // Reverse iterator test
                for ( typename Set::reverse_iterator it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it )
                    it->nIteratorCall += 1;
                for ( auto& el : arrValue ) {
                    CPPUNIT_ASSERT( el.nIteratorCall == 1 );
                    el.nIteratorCall = 0;
                }
            }

            {
                // Reverse const iterator test
                for ( typename Set::const_reverse_iterator it = s.crbegin(), itEnd = s.crend(); it != itEnd; ++it ) {
                    (*it).nIteratorCall += 1;
                    it.release();
                }
                for ( auto& el : arrValue ) {
                    CPPUNIT_ASSERT( el.nIteratorCall == 1 );
                    el.nIteratorCall = 0;
                }
            }

            // update() exists test
            for ( auto& el : arrValue ) {
                bool bOp, bInsert;
                std::tie(bOp, bInsert) = s.update( el, false );
                CPPUNIT_ASSERT( bOp );
                CPPUNIT_ASSERT( !bInsert );
                CPPUNIT_ASSERT( el.nFindCall == 0 );
                CPPUNIT_ASSERT(s.find(el.hash, [](value_type& v) { v.nFindCall++; } ));
                CPPUNIT_ASSERT( el.nFindCall == 1 );
            }

            // unlink test
            CPPUNIT_ASSERT(s.size() == arrCapacity );
            for ( auto const& el : arrValue ) {
                CPPUNIT_ASSERT(s.unlink( el ));
                CPPUNIT_ASSERT(!s.contains( el.hash ));
            }
            CPPUNIT_ASSERT(s.size() == 0 );
            Set::gc::force_dispose();
            for ( auto const& el : arrValue ) {
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
            }

            // new hash values
            for ( auto& el : arrValue )
                el.hash = hasher( el.hash );

            // insert( func )
            CPPUNIT_ASSERT(s.size() == 0 );
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT( s.insert( el, []( value_type& v ) { ++v.nInsertCall; } ));
                CPPUNIT_ASSERT(s.contains( el.hash ));
                CPPUNIT_ASSERT( el.nInsertCall == 1 );
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity );
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT(s.contains( el.hash ));
                CPPUNIT_ASSERT( !s.insert( el ) );
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity );
            CPPUNIT_ASSERT( !s.empty() );

            for ( auto& el : arrValue )
                el.nDisposeCount = 0;

            s.clear();
            CPPUNIT_ASSERT(s.size() == 0 );
            Set::gc::force_dispose();
            for ( auto const& el : arrValue ) {
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
            }

            // new hash values
            for ( auto& el : arrValue )
                el.hash = hasher( el.hash );

            // update test
            for ( auto& el : arrValue ) {
                bool bOp, bInsert;
                std::tie(bOp, bInsert) = s.update( el, false );
                CPPUNIT_ASSERT( !bOp );
                CPPUNIT_ASSERT( !bInsert );
                CPPUNIT_ASSERT( !s.contains( el.hash ));

                std::tie(bOp, bInsert) = s.update( el, true );
                CPPUNIT_ASSERT( bOp );
                CPPUNIT_ASSERT( bInsert );
                CPPUNIT_ASSERT( s.contains( el.hash ));
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity );

            // erase test
            for ( auto& el : arrValue ) {
                el.nDisposeCount = 0;
                CPPUNIT_ASSERT( s.contains( el.hash ));
                CPPUNIT_ASSERT(s.erase( el.hash ));
                CPPUNIT_ASSERT( !s.contains( el.hash ));
                CPPUNIT_ASSERT( !s.erase( el.hash ));
            }
            CPPUNIT_ASSERT(s.size() == 0 );
            Set::gc::force_dispose();
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
                CPPUNIT_ASSERT(s.insert( el ));
            }

            // erase with functor, get() test
            for ( auto& el : arrValue ) {
                el.nDisposeCount = 0;
                CPPUNIT_ASSERT( s.contains( el.hash ) );
                {
                    typename Set::guarded_ptr gp{ s.get( el.hash ) };
                    CPPUNIT_ASSERT( gp );
                    CPPUNIT_ASSERT( gp->nEraseCall == 0);
                    CPPUNIT_ASSERT(s.erase( gp->hash, []( value_type& i ) { ++i.nEraseCall; } ));
                    CPPUNIT_ASSERT( gp->nEraseCall == 1);
                    Set::gc::force_dispose();
                    CPPUNIT_ASSERT( gp->nDisposeCount == 0 );
                }
                CPPUNIT_ASSERT( !s.contains( el.hash ));
                CPPUNIT_ASSERT( !s.erase( el.hash ));
                CPPUNIT_ASSERT( el.nEraseCall == 1 );
                Set::gc::force_dispose();
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
            }
            CPPUNIT_ASSERT(s.size() == 0 );

            // new hash values
            for ( auto& el : arrValue ) {
                el.hash = hasher( el.hash );
                el.nDisposeCount = 0;
                bool bOp, bInsert;
                std::tie(bOp, bInsert) = s.update( el );
                CPPUNIT_ASSERT( bOp );
                CPPUNIT_ASSERT( bInsert );
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity );

            // extract test
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT( s.contains( el.hash ) );
                typename Set::guarded_ptr gp = s.extract( el.hash );
                CPPUNIT_ASSERT( gp );
                Set::gc::force_dispose();
                CPPUNIT_ASSERT( el.nDisposeCount == 0 );
                CPPUNIT_ASSERT( gp->nDisposeCount == 0 );
                gp = s.get( el.hash );
                CPPUNIT_ASSERT( !gp );
                Set::gc::force_dispose();
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
                CPPUNIT_ASSERT( !s.contains( el.hash ) );
            }
            CPPUNIT_ASSERT(s.size() == 0 );
            CPPUNIT_ASSERT(s.empty() );

            // erase with iterator
            for ( auto& el : arrValue ) {
                el.nDisposeCount = 0;
                el.nIteratorCall = 0;
                CPPUNIT_ASSERT(s.insert( el ));
            }
            for ( auto it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                s.erase_at( it );
                it->nIteratorCall = 1;
            }
            CPPUNIT_ASSERT(s.size() == 0 );
            Set::gc::force_dispose();
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
                CPPUNIT_ASSERT( el.nIteratorCall == 1 );
            }
            CPPUNIT_ASSERT(s.empty() );

            // erase with reverse_iterator
            for ( auto& el : arrValue ) {
                el.nDisposeCount = 0;
                el.nIteratorCall = 0;
                CPPUNIT_ASSERT(s.insert( el ));
            }
            for ( auto it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it ) {
                s.erase_at( it );
                it->nIteratorCall = 1;
            }
            CPPUNIT_ASSERT(s.size() == 0 );
            Set::gc::force_dispose();
            for ( auto& el : arrValue ) {
                CPPUNIT_ASSERT( el.nDisposeCount == 1 );
                CPPUNIT_ASSERT( el.nIteratorCall == 1 );
            }
            CPPUNIT_ASSERT(s.empty() );

            CPPUNIT_MSG( s.statistics() );
        }

        template <typename Set, typename Hash>
        void test_rcu(size_t nHeadBits, size_t nArrayBits)
        {
            typedef typename Set::hash_type hash_type;
            typedef typename Set::value_type value_type;
            typedef typename Set::rcu_lock  rcu_lock;

            Hash hasher;

            size_t const arrCapacity = 1000;
            std::vector< value_type > arrValue;
            arrValue.reserve(arrCapacity);
            for (size_t i = 0; i < arrCapacity; ++i) {
                arrValue.emplace_back(value_type());
                arrValue.back().hash = hasher(i);
            }
            CPPUNIT_ASSERT(arrValue.size() == arrCapacity);

            Set s(nHeadBits, nArrayBits);
            CPPUNIT_MSG("Array size: head=" << s.head_size() << ", array_node=" << s.array_node_size());
            CPPUNIT_ASSERT(s.head_size() >= (size_t(1) << nHeadBits));
            CPPUNIT_ASSERT(s.array_node_size() == (size_t(1) << nArrayBits));

            // insert() test
            CPPUNIT_ASSERT(s.size() == 0);
            CPPUNIT_ASSERT(s.empty());
            for (auto& el : arrValue) {
                CPPUNIT_ASSERT(s.insert(el));
                CPPUNIT_ASSERT(s.contains(el.hash));
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity);
            for (auto& el : arrValue) {
                CPPUNIT_ASSERT(s.contains(el.hash));
                CPPUNIT_ASSERT(!s.insert(el));
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity);
            CPPUNIT_ASSERT(!s.empty());

            // Iterator test
            {
                rcu_lock l;

                typedef typename Set::iterator iterator;
                for (iterator it = s.begin(), itEnd = s.end(); it != itEnd; ++it)
                    ++(it->nIteratorCall);
                for (auto& el : arrValue) {
                    CPPUNIT_ASSERT(el.nIteratorCall == 1);
                    el.nIteratorCall = 0;
                }
            }

            {
                // Const iterator test
                rcu_lock l;

                for (typename Set::const_iterator it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it)
                    (*it).nIteratorCall += 1;
                for (auto& el : arrValue) {
                    CPPUNIT_ASSERT(el.nIteratorCall == 1);
                    el.nIteratorCall = 0;
                }
            }

            {
                // Reverse iterator test
                rcu_lock l;

                for (typename Set::reverse_iterator it = s.rbegin(), itEnd = s.rend(); it != itEnd; ++it)
                    it->nIteratorCall += 1;
                for (auto& el : arrValue) {
                    CPPUNIT_ASSERT(el.nIteratorCall == 1);
                    el.nIteratorCall = 0;
                }
            }

            {
                // Reverse const iterator test
                rcu_lock l;

                for (typename Set::const_reverse_iterator it = s.crbegin(), itEnd = s.crend(); it != itEnd; ++it) {
                    (*it).nIteratorCall += 1;
                }
                for (auto& el : arrValue) {
                    CPPUNIT_ASSERT(el.nIteratorCall == 1);
                    el.nIteratorCall = 0;
                }
            }

            // update() exists test
            for (auto& el : arrValue) {
                bool bOp, bInsert;
                std::tie(bOp, bInsert) = s.update(el, false);
                CPPUNIT_ASSERT(bOp);
                CPPUNIT_ASSERT(!bInsert);
                CPPUNIT_ASSERT(el.nFindCall == 0);
                CPPUNIT_ASSERT(s.find(el.hash, [](value_type& v) { v.nFindCall++; }));
                CPPUNIT_ASSERT(el.nFindCall == 1);
            }

            // unlink test
            CPPUNIT_ASSERT(s.size() == arrCapacity);
            for (auto const& el : arrValue) {
                CPPUNIT_ASSERT(s.unlink(el));
                CPPUNIT_ASSERT(!s.contains(el.hash));
            }
            CPPUNIT_ASSERT(s.size() == 0);
            Set::gc::force_dispose();
            for (auto const& el : arrValue) {
                CPPUNIT_ASSERT(el.nDisposeCount == 1);
            }

            // new hash values
            for (auto& el : arrValue)
                el.hash = hasher(el.hash);

            // insert( func )
            CPPUNIT_ASSERT(s.size() == 0);
            for (auto& el : arrValue) {
                CPPUNIT_ASSERT(s.insert(el, [](value_type& v) { ++v.nInsertCall; }));
                CPPUNIT_ASSERT(s.contains(el.hash));
                CPPUNIT_ASSERT(el.nInsertCall == 1);
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity);
            for (auto& el : arrValue) {
                CPPUNIT_ASSERT(s.contains(el.hash));
                CPPUNIT_ASSERT(!s.insert(el));
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity);
            CPPUNIT_ASSERT(!s.empty());

            for (auto& el : arrValue)
                el.nDisposeCount = 0;

            s.clear();
            CPPUNIT_ASSERT(s.size() == 0);
            Set::gc::force_dispose();
            for (auto const& el : arrValue) {
                CPPUNIT_ASSERT(el.nDisposeCount == 1);
            }

            // new hash values
            for (auto& el : arrValue)
                el.hash = hasher(el.hash);

            // update test
            for (auto& el : arrValue) {
                bool bOp, bInsert;
                std::tie(bOp, bInsert) = s.update(el, false);
                CPPUNIT_ASSERT(!bOp);
                CPPUNIT_ASSERT(!bInsert);
                CPPUNIT_ASSERT(!s.contains(el.hash));

                std::tie(bOp, bInsert) = s.update(el, true);
                CPPUNIT_ASSERT(bOp);
                CPPUNIT_ASSERT(bInsert);
                CPPUNIT_ASSERT(s.contains(el.hash));
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity);

            // erase test
            for (auto& el : arrValue) {
                el.nDisposeCount = 0;
                CPPUNIT_ASSERT(s.contains(el.hash));
                CPPUNIT_ASSERT(s.erase(el.hash));
                CPPUNIT_ASSERT(!s.contains(el.hash));
                CPPUNIT_ASSERT(!s.erase(el.hash));
            }
            CPPUNIT_ASSERT(s.size() == 0);
            Set::gc::force_dispose();
            for (auto& el : arrValue) {
                CPPUNIT_ASSERT(el.nDisposeCount == 1);
                CPPUNIT_ASSERT(s.insert(el));
            }

            // erase with functor, get() test
            for (auto& el : arrValue) {
                el.nDisposeCount = 0;
                CPPUNIT_ASSERT(s.contains(el.hash));
                value_type * p;
                {
                    rcu_lock l;
                    p = s.get(el.hash);
                    CPPUNIT_ASSERT(p);
                    CPPUNIT_ASSERT(p->nEraseCall == 0);
                }
                // This is single-threaded test with faked disposer
                // so we can dereference p outside RCU lock section
                CPPUNIT_ASSERT(s.erase(p->hash, [](value_type& i) { ++i.nEraseCall; }));
                CPPUNIT_ASSERT(p->nEraseCall == 1);
                Set::gc::force_dispose();
                CPPUNIT_ASSERT(p->nDisposeCount == 1);

                CPPUNIT_ASSERT(!s.contains(el.hash));
                CPPUNIT_ASSERT(!s.erase(el.hash));
                CPPUNIT_ASSERT(el.nEraseCall == 1);
                Set::gc::force_dispose();
                CPPUNIT_ASSERT(el.nDisposeCount == 1);
            }
            CPPUNIT_ASSERT(s.size() == 0);

            // new hash values
            for (auto& el : arrValue) {
                el.hash = hasher(el.hash);
                el.nDisposeCount = 0;
                bool bOp, bInsert;
                std::tie(bOp, bInsert) = s.update(el);
                CPPUNIT_ASSERT(bOp);
                CPPUNIT_ASSERT(bInsert);
            }
            CPPUNIT_ASSERT(s.size() == arrCapacity);

            // extract test
            for (auto& el : arrValue) {
                CPPUNIT_ASSERT(s.contains(el.hash));
                typename Set::exempt_ptr xp = s.extract(el.hash);
                CPPUNIT_ASSERT(xp);
                Set::gc::force_dispose();
                CPPUNIT_ASSERT(el.nDisposeCount == 0);
                CPPUNIT_ASSERT(xp->nDisposeCount == 0);
                xp.release();
                {
                    rcu_lock l;
                    value_type * p = s.get(el.hash);
                    CPPUNIT_ASSERT(!p);
                }
                Set::gc::force_dispose();
                CPPUNIT_ASSERT(el.nDisposeCount == 1);
                CPPUNIT_ASSERT(!s.contains(el.hash));
            }
            CPPUNIT_ASSERT(s.size() == 0);
            CPPUNIT_ASSERT(s.empty());

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

        CPPUNIT_TEST_SUITE(IntrusiveFeldmanHashSetHdrTest)
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

#endif // #ifndef CDSTEST_HDR_INTRUSIVE_FELDMAN_HASHSET_H
