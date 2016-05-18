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

#include "map2/map_type.h"
#include "cppunit/thread.h"
#include <vector>

namespace map2 {

#define TEST_CASE(TAG, X)  void X();

    class Map_InsDelFind: public CppUnitMini::TestCase
    {
    public:
        size_t  c_nMapSize = 500000;          // initial map size
        size_t  c_nThreadCount = 8;      // thread count
        size_t  c_nMaxLoadFactor = 8;    // maximum load factor
        unsigned int c_nInsertPercentage = 5;
        unsigned int c_nDeletePercentage = 5;
        unsigned int c_nDuration = 30;    // test duration, seconds
        bool    c_bPrintGCState = true;

        size_t c_nCuckooInitialSize = 1024;// initial size for CuckooMap
        size_t c_nCuckooProbesetSize = 16; // CuckooMap probeset size (only for list-based probeset)
        size_t c_nCuckooProbesetThreshold = 0; // CUckooMap probeset threshold (o - use default)

        size_t c_nFeldmanMap_HeadBits = 10;
        size_t c_nFeldmanMap_ArrayBits = 4;

        size_t  c_nLoadFactor = 2;  // current load factor

    public:
        enum actions
        {
            do_find,
            do_insert,
            do_delete
        };
        static const unsigned int c_nShuffleSize = 100;
        actions m_arrShuffle[c_nShuffleSize];

    protected:
        typedef size_t  key_type;
        typedef size_t  value_type;

        template <class Map>
        class WorkThread: public CppUnitMini::TestThread
        {
            Map&     m_Map;

            virtual WorkThread *    clone()
            {
                return new WorkThread( *this );
            }
        public:
            size_t  m_nInsertSuccess;
            size_t  m_nInsertFailed;
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;
            size_t  m_nFindSuccess;
            size_t  m_nFindFailed;

        public:
            WorkThread( CppUnitMini::ThreadPool& pool, Map& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            WorkThread( WorkThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_InsDelFind&  getTest()
            {
                return reinterpret_cast<Map_InsDelFind&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            typedef std::pair< key_type const, value_type > map_value_type;

            struct update_functor {
                template <typename Q>
                void operator()( bool /*bNew*/, map_value_type& /*cur*/, Q const& /*val*/ )
                {}

                // FeldmanHashMap
                void operator()( map_value_type& /*cur*/, map_value_type * /*old*/)
                {}

                // MichaelMap
                void operator()( bool /*bNew*/, map_value_type& /*cur*/ )
                {}

                // BronsonAVLTreeMap
                void operator()( bool /*bNew*/, key_type /*key*/, value_type& /*val*/ )
                {}
            };

            virtual void test()
            {
                Map& rMap = m_Map;

                m_nInsertSuccess =
                    m_nInsertFailed =
                    m_nDeleteSuccess =
                    m_nDeleteFailed =
                    m_nFindSuccess =
                    m_nFindFailed = 0;

                actions * pAct = getTest().m_arrShuffle;
                unsigned int i = 0;
                size_t const nNormalize = size_t(-1) / (getTest().c_nMapSize * 2);

                size_t nRand = 0;
                while ( !time_elapsed() ) {
                    nRand = cds::bitop::RandXorShift(nRand);
                    size_t n = nRand / nNormalize;
                    switch ( pAct[i] ) {
                    case do_find:
                        if ( rMap.contains( n ))
                            ++m_nFindSuccess;
                        else
                            ++m_nFindFailed;
                        break;
                    case do_insert:
                        if ( n % 2 ) {
                            if ( rMap.insert( n, n ))
                                ++m_nInsertSuccess;
                            else
                                ++m_nInsertFailed;
                        }
                        else {
                            if ( rMap.update(n, update_functor(), true ).first )
                                ++m_nInsertSuccess;
                            else
                                ++m_nInsertFailed;
                        }
                        break;
                    case do_delete:
                        if ( rMap.erase( n ))
                            ++m_nDeleteSuccess;
                        else
                            ++m_nDeleteFailed;
                        break;
                    }

                    if ( ++i >= c_nShuffleSize )
                        i = 0;
                }
            }
        };

    protected:
        template <class Map>
        void do_test( Map& testMap )
        {
            typedef WorkThread<Map> work_thread;
            cds::OS::Timer    timer;

            // fill map - only odd number
            {
                std::vector<size_t> arr;
                arr.reserve( c_nMapSize );
                for ( size_t i = 0; i < c_nMapSize; ++i )
                    arr.push_back( i * 2 + 1);
                shuffle( arr.begin(), arr.end() );
                for ( size_t i = 0; i < c_nMapSize; ++i )
                    testMap.insert( arr[i], arr[i] );
            }
            CPPUNIT_MSG( "   Insert " << c_nMapSize << " items time (single-threaded)=" << timer.duration() );

            timer.reset();
            CppUnitMini::ThreadPool pool( *this );
            pool.add( new work_thread( pool, testMap ), c_nThreadCount );
            pool.run( c_nDuration );
            //CPPUNIT_MSG( "   Duration=" << pool.avgDuration() );

            size_t nInsertSuccess = 0;
            size_t nInsertFailed = 0;
            size_t nDeleteSuccess = 0;
            size_t nDeleteFailed = 0;
            size_t nFindSuccess = 0;
            size_t nFindFailed = 0;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                work_thread * pThread = static_cast<work_thread *>( *it );
                assert( pThread != nullptr );
                nInsertSuccess += pThread->m_nInsertSuccess;
                nInsertFailed += pThread->m_nInsertFailed;
                nDeleteSuccess += pThread->m_nDeleteSuccess;
                nDeleteFailed += pThread->m_nDeleteFailed;
                nFindSuccess += pThread->m_nFindSuccess;
                nFindFailed += pThread->m_nFindFailed;
            }

            size_t nTotalOps = nInsertSuccess + nInsertFailed + nDeleteSuccess + nDeleteFailed + nFindSuccess + nFindFailed;

            CPPUNIT_MSG( "  Totals (success/failed): \n\t"
                      << "      Insert=" << nInsertSuccess << '/' << nInsertFailed << "\n\t"
                      << "      Delete=" << nDeleteSuccess << '/' << nDeleteFailed << "\n\t"
                      << "        Find=" << nFindSuccess   << '/' << nFindFailed   << "\n\t"
                      << "       Speed=" << (nFindSuccess + nFindFailed) / c_nDuration << " find/sec\n\t"
                      << "             " << (nInsertSuccess + nDeleteSuccess) / c_nDuration << " modify/sec\n\t"
                      << "   Total ops=" << nTotalOps << "\n\t"
                      << "       speed=" << nTotalOps / c_nDuration << " ops/sec\n\t"
                      << "      Map size=" << testMap.size()
                );


            check_before_cleanup( testMap );

            CPPUNIT_MSG( "  Clear map (single-threaded)..." );
            timer.reset();
            testMap.clear();
            CPPUNIT_MSG( "   Duration=" << timer.duration() );
            CPPUNIT_CHECK_EX( testMap.empty(), "size=" << ((long long) testMap.size()) );

            additional_check( testMap );
            print_stat( testMap );
            additional_cleanup( testMap );
        }

        template <class Map>
        void run_test()
        {
            CPPUNIT_MSG( "Thread count=" << c_nThreadCount
                << " initial map size=" << c_nMapSize
                << " insert=" << c_nInsertPercentage << '%'
                << " delete=" << c_nDeletePercentage << '%'
                << " duration=" << c_nDuration << "s"
                );

            if ( Map::c_bLoadFactorDepended ) {
                for ( c_nLoadFactor = 1; c_nLoadFactor <= c_nMaxLoadFactor; c_nLoadFactor *= 2 ) {
                    CPPUNIT_MSG( "Load factor=" << c_nLoadFactor );
                    Map  testMap( *this );
                    do_test( testMap );
                    if ( c_bPrintGCState )
                        print_gc_state();
                }
            }
            else {
                Map testMap( *this );
                do_test( testMap );
                if ( c_bPrintGCState )
                    print_gc_state();
            }
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg );

#   include "map2/map_defs.h"
        CDSUNIT_DECLARE_MichaelMap
        CDSUNIT_DECLARE_SplitList
        CDSUNIT_DECLARE_SkipListMap
        CDSUNIT_DECLARE_EllenBinTreeMap
        CDSUNIT_DECLARE_BronsonAVLTreeMap
        CDSUNIT_DECLARE_FeldmanHashMap_fixed
        CDSUNIT_DECLARE_FeldmanHashMap_city
        CDSUNIT_DECLARE_StripedMap
        CDSUNIT_DECLARE_RefinableMap
        CDSUNIT_DECLARE_CuckooMap
        CDSUNIT_DECLARE_StdMap

        CPPUNIT_TEST_SUITE(Map_InsDelFind)
            CDSUNIT_TEST_MichaelMap
            CDSUNIT_TEST_SplitList
            CDSUNIT_TEST_SkipListMap
            CDSUNIT_TEST_EllenBinTreeMap
            CDSUNIT_TEST_BronsonAVLTreeMap
            CDSUNIT_TEST_FeldmanHashMap_fixed
            CDSUNIT_TEST_FeldmanHashMap_city
            CDSUNIT_TEST_CuckooMap
            CDSUNIT_TEST_StripedMap
            CDSUNIT_TEST_RefinableMap
            CDSUNIT_TEST_StdMap
        CPPUNIT_TEST_SUITE_END();
    };
} // namespace map2
