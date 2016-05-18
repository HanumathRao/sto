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

#ifndef CDSTEST_HDR_CUCKOO_SET_H
#define CDSTEST_HDR_CUCKOO_SET_H

#include "cppunit/cppunit_proxy.h"
#include "size_check.h"

#include <cds/opt/hash.h>
#include <cds/os/timer.h>
#include <functional>   // ref

// forward namespace declaration
namespace cds {
    namespace container {}
    namespace opt {}
}

namespace set {
    using misc::check_size;

    namespace cc = cds::container;
    namespace co = cds::opt;


    class CuckooSetHdrTest: public CppUnitMini::TestCase
    {
    public:
        struct stat
        {
            unsigned int nFindCount     ;   // count of find-functor calling
            unsigned int nUpdateNewCount;
            unsigned int nUpdateCount;

            stat()
            {
                memset( this, 0, sizeof(*this));
            }

            void copy( stat const& s )
            {
                nFindCount = s.nFindCount;
                nUpdateCount = s.nUpdateCount;
                nUpdateNewCount = s.nUpdateNewCount;
            }
        };

        struct item: public stat
        {
            int nKey;
            int nVal;

            item()
            {}

            item( int key )
                : nKey( key )
                , nVal( key )
            {}

            item (int key, int val )
                : nKey(key)
                , nVal( val )
            {}

            item( std::pair<int,int> const& p )
                : nKey( p.first )
                , nVal( p.second )
            {}

            item( item const& i )
                : nKey( i.nKey )
                , nVal( i.nVal )
            {}

            item& operator=(item const& i)
            {
                nKey = i.nKey;
                nVal = i.nVal;
                stat::copy(i);

                return *this;
            }

            item( item&& i )
                : nKey( i.nKey )
                , nVal( i.nVal )
            {}

            int key() const
            {
                return nKey;
            }

            int val() const
            {
                return nVal;
            }
        };

        struct hash1 {
            size_t operator()( int i ) const
            {
                return co::v::hash<int>()( i );
            }

            size_t operator()( std::pair<int,int> const& i ) const
            {
                return co::v::hash<int>()( i.first );
            }

            template <typename Item>
            size_t operator()( Item const& i ) const
            {
                return (*this)( i.key() );
            }
        };

        struct hash2: private hash1
        {
            typedef hash1 base_class;

            size_t operator()( int i ) const
            {
                return ~( base_class::operator()(i));
            }
            template <typename Item>
            size_t operator()( const Item& i ) const
            {
                return ~( base_class::operator()(i));
            }
            size_t operator()( std::pair<int,int> const& i ) const
            {
                return ~( base_class::operator()(i));
            }
        };

        struct simple_item_counter {
            size_t  m_nCount;

            simple_item_counter()
                : m_nCount(0)
            {}

            size_t operator ++()
            {
                return ++m_nCount;
            }

            size_t operator --()
            {
                return --m_nCount;
            }

            void reset()
            {
                m_nCount = 0;
            }

            operator size_t() const
            {
                return m_nCount;
            }
        };

        template <typename T>
        struct less
        {
            bool operator ()(const T& v1, const T& v2 ) const
            {
                return v1.key() < v2.key();
            }

            template <typename Q>
            bool operator ()(const T& v1, const Q& v2 ) const
            {
                return v1.key() < v2;
            }

            template <typename Q>
            bool operator ()(const Q& v1, const T& v2 ) const
            {
                return v1 < v2.key();
            }

            bool operator ()( std::pair<int, int> const& v1, const T& v2 ) const
            {
                return v1.first < v2.key();
            }

            bool operator ()(const T& v1, std::pair<int, int> const& v2 ) const
            {
                return v1.key() < v2.first;
            }
        };

        template <typename T>
        struct cmp {
            int operator ()(const T& v1, const T& v2 ) const
            {
                if ( v1.key() < v2.key() )
                    return -1;
                return v1.key() > v2.key() ? 1 : 0;
            }

            template <typename Q>
            int operator ()(const T& v1, const Q& v2 ) const
            {
                if ( v1.key() < v2 )
                    return -1;
                return v1.key() > v2 ? 1 : 0;
            }

            template <typename Q>
            int operator ()(const Q& v1, const T& v2 ) const
            {
                if ( v1 < v2.key() )
                    return -1;
                return v1 > v2.key() ? 1 : 0;
            }

            int operator()( std::pair<int,int> const& v1, T const& v2 ) const
            {
                if ( v1.first < v2.key() )
                    return -1;
                return v1.first > v2.key() ? 1 : 0;
            }

            int operator()( T const& v1, std::pair<int,int> const& v2 ) const
            {
                if ( v1.key() < v2.first )
                    return -1;
                return v1.key() > v2.first ? 1 : 0;
            }
        };

        template <typename T>
        struct equal
        {
            bool operator ()(const T& v1, const T& v2 ) const
            {
                return v1.key() == v2.key();
            }

            template <typename Q>
            bool operator ()(const T& v1, const Q& v2 ) const
            {
                return v1.key() == v2;
            }

            template <typename Q>
            bool operator ()(const Q& v1, const T& v2 ) const
            {
                return v1 == v2.key();
            }

            bool operator ()( std::pair<int, int> const& v1, const T& v2 ) const
            {
                return v1.first == v2.key();
            }

            bool operator ()(const T& v1, std::pair<int, int> const& v2 ) const
            {
                return v1.key() == v2.first;
            }
        };

        struct find_functor
        {
            template <typename Item, typename T>
            void operator()( Item& i, T& /*val*/ )
            {
                ++i.nFindCount;
            }
            template <typename Item, typename T>
            void operator()( Item& i, T const& /*val*/ )
            {
                ++i.nFindCount;
            }
        };

        template <typename Item>
        struct copy_found
        {
            Item    m_found;

            template <typename T>
            void operator()( Item& i, T& /*val*/ )
            {
                m_found = i;
            }

            void operator()( Item const& i )
            {
                m_found = i;
            }
        };

        struct insert_functor
        {
            template <typename Item>
            void operator()(Item& i )
            {
                i.nVal = i.nKey * 100;
            }
        };

        template <typename Item, typename Q>
        static void update_func( bool bNew, Item& i, Q& /*val*/ )
        {
            if ( bNew )
                ++i.nUpdateNewCount;
            else
                ++i.nUpdateCount;
        }

        struct update_functor
        {
            template <typename Item, typename Q>
            void operator()( bool bNew, Item& i, Q& val )
            {
                update_func( bNew, i, val );
            }
        };

        template <class Set, typename Predicate>
        void test_int_with( Set& s, Predicate pred )
        {
            typedef typename Set::value_type    value_type;

            item itm;
            int key;

            // insert/find test
            CPPUNIT_ASSERT( !s.contains( 10 ) );
            CPPUNIT_ASSERT( s.insert( 10 ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 1 ));
            CPPUNIT_ASSERT( s.contains( 10 ) );

            CPPUNIT_ASSERT( !s.insert( 10 ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 1 ));

            CPPUNIT_ASSERT( !s.contains( 20, pred ) );
            CPPUNIT_ASSERT( s.insert( std::make_pair(20, 25) ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 2 ));
            CPPUNIT_ASSERT( s.contains( 10, pred ) );
            CPPUNIT_ASSERT( s.contains( key = 20 ) );
            CPPUNIT_ASSERT( s.find_with( key, pred, find_functor() ) );
            {
                copy_found<item> f;
                key = 20;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 20 );
                CPPUNIT_ASSERT( f.m_found.nVal == 25 );
                CPPUNIT_ASSERT( f.m_found.nFindCount == 1 );
            }
            {
                copy_found<item> f;
                key = 20;
                CPPUNIT_ASSERT( s.find_with( key, pred, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 20 );
                CPPUNIT_ASSERT( f.m_found.nVal == 25 );
                CPPUNIT_ASSERT( f.m_found.nFindCount == 1 );
            }
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 2 ));

            CPPUNIT_ASSERT( !s.contains( 25 ) );
            CPPUNIT_ASSERT( s.insert( std::make_pair(25, -1), insert_functor() ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 3 ));
            {
                copy_found<item> f;
                key = 25;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 25 );
                CPPUNIT_ASSERT( f.m_found.nVal == 2500 );
            }

            // update() test
            key = 10;
            {
                copy_found<item> f;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 10 );
                CPPUNIT_ASSERT( f.m_found.nVal == 10 );
                CPPUNIT_ASSERT( f.m_found.nUpdateCount == 0 );
                CPPUNIT_ASSERT( f.m_found.nUpdateNewCount == 0 );
            }
            std::pair<bool, bool> updateResult = s.update( key, update_functor() );
            CPPUNIT_ASSERT( updateResult.first && !updateResult.second );
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 3 ));
            {
                copy_found<item> f;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 10 );
                CPPUNIT_ASSERT( f.m_found.nVal == 10 );
                CPPUNIT_ASSERT( f.m_found.nUpdateCount == 1 );
                CPPUNIT_ASSERT( f.m_found.nUpdateNewCount == 0 );
            }

            updateResult = s.update(std::make_pair(13, 1300), update_functor(), false);
            CPPUNIT_ASSERT(!updateResult.first && !updateResult.second);

            updateResult = s.update( std::make_pair(13, 1300), update_functor() );
            CPPUNIT_ASSERT( updateResult.first && updateResult.second );
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 4 ));
            {
                copy_found<item> f;
                key = 13;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 13 );
                CPPUNIT_ASSERT( f.m_found.nVal == 1300 );
                CPPUNIT_ASSERT( f.m_found.nUpdateCount == 0 );
                CPPUNIT_ASSERT( f.m_found.nUpdateNewCount == 1 );
            }

            // erase test
            CPPUNIT_ASSERT( s.erase(13) );
            CPPUNIT_ASSERT( !s.contains( 13 ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 3 ));
            CPPUNIT_ASSERT( !s.erase(13) );
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 3 ));

            CPPUNIT_ASSERT( s.contains( 10 ));
            CPPUNIT_ASSERT( s.erase_with( 10, pred ));
            CPPUNIT_ASSERT( !s.contains( 10 ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 2 ));
            CPPUNIT_ASSERT( !s.erase_with(10, pred) );
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 2 ));

            CPPUNIT_ASSERT( s.contains(20) );
            {
                copy_found<item> f;
                CPPUNIT_ASSERT( s.erase( 20, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 20 );
                CPPUNIT_ASSERT( f.m_found.nVal == 25 );

                CPPUNIT_ASSERT( s.insert(235))
                    CPPUNIT_ASSERT( s.erase_with( 235, pred, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 235 );
                CPPUNIT_ASSERT( f.m_found.nVal == 235 );
            }
            CPPUNIT_ASSERT( !s.contains( 20 ));
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 1 ));

            s.clear();
            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT( check_size( s, 0 ));

            // emplace test
            CPPUNIT_ASSERT( s.emplace( 151 )) ;  // key = 151,  val = 151
            CPPUNIT_ASSERT( s.emplace( 174, 471 )) ;    // key = 174, val = 471
            CPPUNIT_ASSERT( s.emplace( std::make_pair( 190, 91 ) )) ; // key == 190, val = 91
            CPPUNIT_ASSERT( !s.empty() );
            CPPUNIT_ASSERT( check_size( s, 3 ));

            CPPUNIT_ASSERT( s.contains(151));
            CPPUNIT_ASSERT( s.contains(174, pred ));
            CPPUNIT_ASSERT( s.contains(190));

            {
                copy_found<item> f;
                key = 151;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 151 );
                CPPUNIT_ASSERT( f.m_found.nVal == 151 );

                key = 174;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 174 );
                CPPUNIT_ASSERT( f.m_found.nVal == 471 );

                key = 190;
                CPPUNIT_ASSERT( s.find( key, std::ref( f ) ) );
                CPPUNIT_ASSERT( f.m_found.nKey == 190 );
                CPPUNIT_ASSERT( f.m_found.nVal == 91 );
            }

            s.clear();
            CPPUNIT_ASSERT( s.empty() );
            CPPUNIT_ASSERT( check_size( s, 0 ));
        }

        template <class Set, class Predicate>
        void test_int()
        {
            Set s( 32, 4, 3 );
            CPPUNIT_ASSERT( s.bucket_count() == 32 );
            CPPUNIT_ASSERT( s.lock_count() == 32 );

            cds::OS::Timer    timer;

            test_int_with( s, Predicate() );

            // Resizing test
            for ( int i = 0; i < 10000; i++ ) {
                s.insert( i );
            }

            CPPUNIT_MSG( "   Duration=" << timer.duration() );
        }

    public:
        void Cuckoo_Striped_list_unord();
        void Cuckoo_Striped_list_unord_storehash();
        void Cuckoo_Striped_list_cmp();
        void Cuckoo_Striped_list_cmp_storehash();
        void Cuckoo_Striped_list_less();
        void Cuckoo_Striped_list_less_storehash();
        void Cuckoo_Striped_list_less_cmp();
        void Cuckoo_Striped_list_less_cmp_storehash();
        void Cuckoo_Striped_list_less_cmp_eq();
        void Cuckoo_Striped_list_less_cmp_eq_storehash();

        void Cuckoo_Striped_vector_unord();
        void Cuckoo_Striped_vector_unord_storehash();
        void Cuckoo_Striped_vector_cmp();
        void Cuckoo_Striped_vector_cmp_storehash();
        void Cuckoo_Striped_vector_less();
        void Cuckoo_Striped_vector_less_storehash();
        void Cuckoo_Striped_vector_less_cmp();
        void Cuckoo_Striped_vector_less_cmp_storehash();
        void Cuckoo_Striped_vector_less_cmp_eq();
        void Cuckoo_Striped_vector_less_cmp_eq_storehash();

        void Cuckoo_Refinable_list_unord();
        void Cuckoo_Refinable_list_unord_storehash();
        void Cuckoo_Refinable_list_cmp();
        void Cuckoo_Refinable_list_cmp_storehash();
        void Cuckoo_Refinable_list_less();
        void Cuckoo_Refinable_list_less_storehash();
        void Cuckoo_Refinable_list_less_cmp();
        void Cuckoo_Refinable_list_less_cmp_storehash();
        void Cuckoo_Refinable_list_less_cmp_eq();
        void Cuckoo_Refinable_list_less_cmp_eq_storehash();

        void Cuckoo_Refinable_vector_unord();
        void Cuckoo_Refinable_vector_unord_storehash();
        void Cuckoo_Refinable_vector_cmp();
        void Cuckoo_Refinable_vector_cmp_storehash();
        void Cuckoo_Refinable_vector_less();
        void Cuckoo_Refinable_vector_less_storehash();
        void Cuckoo_Refinable_vector_less_cmp();
        void Cuckoo_Refinable_vector_less_cmp_storehash();
        void Cuckoo_Refinable_vector_less_cmp_eq();
        void Cuckoo_Refinable_vector_less_cmp_eq_storehash();

        CPPUNIT_TEST_SUITE(CuckooSetHdrTest)
            CPPUNIT_TEST( Cuckoo_Striped_list_unord)
            CPPUNIT_TEST( Cuckoo_Striped_list_unord_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_list_cmp)
            CPPUNIT_TEST( Cuckoo_Striped_list_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_list_less)
            CPPUNIT_TEST( Cuckoo_Striped_list_less_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_list_less_cmp)
            CPPUNIT_TEST( Cuckoo_Striped_list_less_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_list_less_cmp_eq)
            CPPUNIT_TEST( Cuckoo_Striped_list_less_cmp_eq_storehash)

            CPPUNIT_TEST( Cuckoo_Striped_vector_unord)
            CPPUNIT_TEST( Cuckoo_Striped_vector_unord_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_vector_cmp)
            CPPUNIT_TEST( Cuckoo_Striped_vector_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_vector_less)
            CPPUNIT_TEST( Cuckoo_Striped_vector_less_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_vector_less_cmp)
            CPPUNIT_TEST( Cuckoo_Striped_vector_less_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Striped_vector_less_cmp_eq)
            CPPUNIT_TEST( Cuckoo_Striped_vector_less_cmp_eq_storehash)

            CPPUNIT_TEST( Cuckoo_Refinable_list_unord)
            CPPUNIT_TEST( Cuckoo_Refinable_list_unord_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_list_cmp)
            CPPUNIT_TEST( Cuckoo_Refinable_list_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_list_less)
            CPPUNIT_TEST( Cuckoo_Refinable_list_less_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_list_less_cmp)
            CPPUNIT_TEST( Cuckoo_Refinable_list_less_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_list_less_cmp_eq)
            CPPUNIT_TEST( Cuckoo_Refinable_list_less_cmp_eq_storehash)

            CPPUNIT_TEST( Cuckoo_Refinable_vector_unord)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_unord_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_cmp)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_less)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_less_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_less_cmp)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_less_cmp_storehash)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_less_cmp_eq)
            CPPUNIT_TEST( Cuckoo_Refinable_vector_less_cmp_eq_storehash)
        CPPUNIT_TEST_SUITE_END()
    };

} // namespace set

#endif // #ifndef CDSTEST_HDR_CUCKOO_SET_H
