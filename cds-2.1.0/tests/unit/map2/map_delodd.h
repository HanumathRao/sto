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

#include "cppunit/thread.h"
#include "map2/map_type.h"
#include <cds/os/topology.h>

namespace map2 {

#   define TEST_CASE(TAG, X)  void X();

    namespace {
        struct key_thread
        {
            uint32_t  nKey;
            uint16_t  nThread;
            uint16_t  pad_;

            key_thread( size_t key, size_t threadNo )
                : nKey( static_cast<uint32_t>(key))
                , nThread( static_cast<uint16_t>(threadNo))
                , pad_(0)
            {}

            key_thread()
            {}
        };
    }

    template <>
    struct cmp<key_thread> {
        int operator ()(key_thread const& k1, key_thread const& k2) const
        {
            if ( k1.nKey < k2.nKey )
                return -1;
            if ( k1.nKey > k2.nKey )
                return 1;
            if ( k1.nThread < k2.nThread )
                return -1;
            if ( k1.nThread > k2.nThread )
                return 1;
            return 0;
        }
        int operator ()(key_thread const& k1, size_t k2) const
        {
            if ( k1.nKey < k2 )
                return -1;
            if ( k1.nKey > k2 )
                return 1;
            return 0;
        }
        int operator ()(size_t k1, key_thread const& k2) const
        {
            if ( k1 < k2.nKey )
                return -1;
            if ( k1 > k2.nKey )
                return 1;
            return 0;
        }
    };

} // namespace map2

namespace std {
    template <>
    struct less<map2::key_thread>
    {
        bool operator()(map2::key_thread const& k1, map2::key_thread const& k2) const
        {
            if ( k1.nKey <= k2.nKey )
                return k1.nKey < k2.nKey || k1.nThread < k2.nThread;
            return false;
        }
    };

    template <>
    struct hash<map2::key_thread>
    {
        typedef size_t              result_type;
        typedef map2::key_thread    argument_type;

        size_t operator()( map2::key_thread const& k ) const
        {
            return std::hash<size_t>()(k.nKey);
        }
        size_t operator()( size_t k ) const
        {
            return std::hash<size_t>()(k);
        }
    };
} // namespace std

namespace boost {
    inline size_t hash_value( map2::key_thread const& k )
    {
        return std::hash<size_t>()( k.nKey );
    }

    template <>
    struct hash<map2::key_thread>
    {
        typedef size_t              result_type;
        typedef map2::key_thread    argument_type;

        size_t operator()(map2::key_thread const& k) const
        {
            return boost::hash<size_t>()( k.nKey );
        }
        size_t operator()(size_t k) const
        {
            return boost::hash<size_t>()( k );
        }
    };
} // namespace boost

namespace map2 {

    class Map_DelOdd: public CppUnitMini::TestCase
    {
    public:
        size_t  c_nInsThreadCount = 4;      // insert thread count
        size_t  c_nDelThreadCount = 4;      // delete thread count
        size_t  c_nExtractThreadCount = 4;  // extract thread count
        size_t  c_nMapSize = 1000000;       // max map size
        size_t  c_nMaxLoadFactor = 8;       // maximum load factor

        size_t  c_nCuckooInitialSize = 1024;// initial size for CuckooMap
        size_t  c_nCuckooProbesetSize = 16; // CuckooMap probeset size (only for list-based probeset)
        size_t  c_nCuckooProbesetThreshold = 0; // CUckooMap probeset threshold (0 - use default)

        size_t c_nFeldmanMap_HeadBits = 10;
        size_t c_nFeldmanMap_ArrayBits = 4;

        bool    c_bPrintGCState = true;

        size_t  c_nLoadFactor;  // current load factor

    private:
        std::vector<size_t>     m_arrInsert;
        std::vector<size_t>     m_arrRemove;

    protected:
        typedef key_thread  key_type;
        typedef size_t      value_type;
        typedef std::pair<key_type const, value_type> pair_type;

        atomics::atomic<size_t>      m_nInsThreadCount;

        // Inserts keys from [0..N)
        template <class Map>
        class InsertThread: public CppUnitMini::TestThread
        {
            Map&     m_Map;

            virtual InsertThread *    clone()
            {
                return new InsertThread( *this );
            }

            struct ensure_func
            {
                template <typename Q>
                void operator()( bool /*bNew*/, Q const& )
                {}
                template <typename Q, typename V>
                void operator()( bool /*bNew*/, Q const&, V& )
                {}

                // FeldmanHashMap
                template <typename Q>
                void operator()( Q&, Q*)
                {}
            };
        public:
            size_t  m_nInsertSuccess;
            size_t  m_nInsertFailed;

        public:
            InsertThread( CppUnitMini::ThreadPool& pool, Map& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            InsertThread( InsertThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_DelOdd&  getTest()
            {
                return reinterpret_cast<Map_DelOdd&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                Map& rMap = m_Map;

                m_nInsertSuccess =
                    m_nInsertFailed = 0;

                std::vector<size_t>& arrData = getTest().m_arrInsert;
                for ( size_t i = 0; i < arrData.size(); ++i ) {
                    if ( rMap.insert( key_type( arrData[i], m_nThreadNo )))
                        ++m_nInsertSuccess;
                    else
                        ++m_nInsertFailed;
                }

                ensure_func f;
                for ( size_t i = arrData.size() - 1; i > 0; --i ) {
                    if ( arrData[i] & 1 ) {
                        rMap.update( key_type( arrData[i], m_nThreadNo ), f );
                    }
                }

                getTest().m_nInsThreadCount.fetch_sub( 1, atomics::memory_order_acquire );
            }
        };

        struct key_equal {
            bool operator()( key_type const& k1, key_type const& k2 ) const
            {
                return k1.nKey == k2.nKey;
            }
            bool operator()( size_t k1, key_type const& k2 ) const
            {
                return k1 == k2.nKey;
            }
            bool operator()( key_type const& k1, size_t k2 ) const
            {
                return k1.nKey == k2;
            }
        };

        struct key_less {
            bool operator()( key_type const& k1, key_type const& k2 ) const
            {
                return k1.nKey < k2.nKey;
            }
            bool operator()( size_t k1, key_type const& k2 ) const
            {
                return k1 < k2.nKey;
            }
            bool operator()( key_type const& k1, size_t k2 ) const
            {
                return k1.nKey < k2;
            }

            typedef key_equal equal_to;
        };

        // Deletes odd keys from [0..N)
        template <class Map>
        class DeleteThread: public CppUnitMini::TestThread
        {
            Map&     m_Map;

            virtual DeleteThread *    clone()
            {
                return new DeleteThread( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            DeleteThread( CppUnitMini::ThreadPool& pool, Map& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            DeleteThread( DeleteThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_DelOdd&  getTest()
            {
                return reinterpret_cast<Map_DelOdd&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            template <typename MapType, bool>
            struct eraser {
                static bool erase(MapType& map, size_t key, size_t /*insThread*/)
                {
                    return map.erase_with(key, key_less());
                }
            };

            template <typename MapType>
            struct eraser<MapType, true>
            {
                static bool erase(MapType& map, size_t key, size_t insThread)
                {
                    return map.erase(key_type(key, insThread));
                }
            };

            virtual void test()
            {
                Map& rMap = m_Map;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                size_t const nInsThreadCount = getTest().c_nInsThreadCount;

                for ( size_t pass = 0; pass < 2; pass++ ) {
                    std::vector<size_t>& arrData = getTest().m_arrRemove;
                    if ( m_nThreadNo & 1 ) {
                        for ( size_t k = 0; k < nInsThreadCount; ++k ) {
                            for ( size_t i = 0; i < arrData.size(); ++i ) {
                                if ( arrData[i] & 1 ) {
                                    if ( Map::c_bEraseExactKey ) {
                                        for (size_t key = 0; key < nInsThreadCount; ++key) {
                                            if ( eraser<Map, Map::c_bEraseExactKey>::erase( rMap, arrData[i], key ))
                                                ++m_nDeleteSuccess;
                                            else
                                                ++m_nDeleteFailed;
                                        }
                                    }
                                    else {
                                        if ( eraser<Map, Map::c_bEraseExactKey>::erase(rMap, arrData[i], 0))
                                            ++m_nDeleteSuccess;
                                        else
                                            ++m_nDeleteFailed;
                                    }
                                }
                            }
                            if ( getTest().m_nInsThreadCount.load( atomics::memory_order_acquire ) == 0 )
                                break;
                        }
                    }
                    else {
                        for ( size_t k = 0; k < nInsThreadCount; ++k ) {
                            for ( size_t i = arrData.size() - 1; i > 0; --i ) {
                                if ( arrData[i] & 1 ) {
                                    if ( Map::c_bEraseExactKey ) {
                                        for (size_t key = 0; key < nInsThreadCount; ++key) {
                                            if (eraser<Map, Map::c_bEraseExactKey>::erase(rMap, arrData[i], key))
                                                ++m_nDeleteSuccess;
                                            else
                                                ++m_nDeleteFailed;
                                        }
                                    }
                                    else {
                                        if (eraser<Map, Map::c_bEraseExactKey>::erase(rMap, arrData[i], 0))
                                            ++m_nDeleteSuccess;
                                        else
                                            ++m_nDeleteFailed;
                                    }
                                }
                            }
                            if ( getTest().m_nInsThreadCount.load( atomics::memory_order_acquire ) == 0 )
                                break;
                        }
                    }
                }
            }
        };

        // Deletes odd keys from [0..N)
        template <class GC, class Map >
        class ExtractThread: public CppUnitMini::TestThread
        {
            Map&     m_Map;

            virtual ExtractThread *    clone()
            {
                return new ExtractThread( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            ExtractThread( CppUnitMini::ThreadPool& pool, Map& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            ExtractThread( ExtractThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_DelOdd&  getTest()
            {
                return reinterpret_cast<Map_DelOdd&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            template <typename MapType, bool>
            struct extractor {
                static typename Map::guarded_ptr extract(MapType& map, size_t key, size_t /*insThread*/)
                {
                    return map.extract_with(key, key_less());
                }
            };

            template <typename MapType>
            struct extractor<MapType, true>
            {
                static typename Map::guarded_ptr extract(MapType& map, size_t key, size_t insThread)
                {
                    return map.extract(key_type(key, insThread));
                }
            };

            virtual void test()
            {
                Map& rMap = m_Map;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                typename Map::guarded_ptr gp;
                size_t const nInsThreadCount = getTest().c_nInsThreadCount;

                for ( size_t pass = 0; pass < 2; ++pass ) {
                    std::vector<size_t>& arrData = getTest().m_arrRemove;
                    if ( m_nThreadNo & 1 ) {
                        for ( size_t k = 0; k < nInsThreadCount; ++k ) {
                            for ( size_t i = 0; i < arrData.size(); ++i ) {
                                if ( arrData[i] & 1 ) {
                                    gp = extractor< Map, Map::c_bEraseExactKey >::extract( rMap, arrData[i], k );
                                    if ( gp )
                                        ++m_nDeleteSuccess;
                                    else
                                        ++m_nDeleteFailed;
                                    gp.release();
                                }
                            }
                            if ( getTest().m_nInsThreadCount.load( atomics::memory_order_acquire ) == 0 )
                                break;
                        }
                    }
                    else {
                        for ( size_t k = 0; k < nInsThreadCount; ++k ) {
                            for ( size_t i = arrData.size() - 1; i > 0; --i ) {
                                if ( arrData[i] & 1 ) {
                                    gp = extractor< Map, Map::c_bEraseExactKey >::extract( rMap, arrData[i], k);
                                    if ( gp )
                                        ++m_nDeleteSuccess;
                                    else
                                        ++m_nDeleteFailed;
                                    gp.release();
                                }
                            }
                            if ( getTest().m_nInsThreadCount.load( atomics::memory_order_acquire ) == 0 )
                                break;
                        }
                    }
                }
            }
        };

        template <class RCU, class Map >
        class ExtractThread< cds::urcu::gc<RCU>, Map > : public CppUnitMini::TestThread
        {
            Map&     m_Map;

            virtual ExtractThread *    clone()
            {
                return new ExtractThread( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            ExtractThread( CppUnitMini::ThreadPool& pool, Map& rMap )
                : CppUnitMini::TestThread( pool )
                , m_Map( rMap )
            {}
            ExtractThread( ExtractThread& src )
                : CppUnitMini::TestThread( src )
                , m_Map( src.m_Map )
            {}

            Map_DelOdd&  getTest()
            {
                return reinterpret_cast<Map_DelOdd&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            template <typename MapType, bool>
            struct extractor {
                static typename Map::exempt_ptr extract( MapType& map, size_t key, size_t /*insThread*/ )
                {
                    return map.extract_with( key, key_less());
                }
            };

            template <typename MapType>
            struct extractor<MapType, true>
            {
                static typename Map::exempt_ptr extract(MapType& map, size_t key, size_t insThread)
                {
                    return map.extract( key_type(key, insThread));
                }
            };

            virtual void test()
            {
                Map& rMap = m_Map;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                typename Map::exempt_ptr xp;
                size_t const nInsThreadCount = getTest().c_nInsThreadCount;

                std::vector<size_t>& arrData = getTest().m_arrRemove;
                if ( m_nThreadNo & 1 ) {
                    for ( size_t k = 0; k < nInsThreadCount; ++k ) {
                        for ( size_t i = 0; i < arrData.size(); ++i ) {
                            if ( arrData[i] & 1 ) {
                                if ( Map::c_bExtractLockExternal ) {
                                    {
                                        typename Map::rcu_lock l;
                                        xp = extractor<Map, Map::c_bEraseExactKey>::extract( rMap, arrData[i], k );
                                        if ( xp )
                                            ++m_nDeleteSuccess;
                                        else
                                            ++m_nDeleteFailed;
                                    }
                                }
                                else {
                                    xp = extractor<Map, Map::c_bEraseExactKey>::extract( rMap, arrData[i], k);
                                    if ( xp )
                                        ++m_nDeleteSuccess;
                                    else
                                        ++m_nDeleteFailed;
                                }
                                xp.release();
                            }
                        }
                        if ( getTest().m_nInsThreadCount.load( atomics::memory_order_acquire ) == 0 )
                            break;
                    }
                }
                else {
                    for ( size_t k = 0; k < nInsThreadCount; ++k ) {
                        for ( size_t i = arrData.size() - 1; i > 0; --i ) {
                            if ( arrData[i] & 1 ) {
                                if ( Map::c_bExtractLockExternal ) {
                                    {
                                        typename Map::rcu_lock l;
                                        xp = extractor<Map, Map::c_bEraseExactKey>::extract(rMap, arrData[i], k);
                                        if ( xp )
                                            ++m_nDeleteSuccess;
                                        else
                                            ++m_nDeleteFailed;
                                    }
                                }
                                else {
                                    xp = extractor<Map, Map::c_bEraseExactKey>::extract(rMap, arrData[i], k);
                                    if ( xp )
                                        ++m_nDeleteSuccess;
                                    else
                                        ++m_nDeleteFailed;
                                }
                                xp.release();
                            }
                        }
                        if ( getTest().m_nInsThreadCount.load( atomics::memory_order_acquire ) == 0 )
                            break;
                    }
                }
            }
        };

    protected:
        template <class Map>
        void do_test()
        {
            Map  testMap( *this );
            do_test_with( testMap );
        }

        template <class Map>
        void do_test_extract()
        {
            Map  testMap( *this );
            do_test_extract_with( testMap );
        }

        template <class Map>
        void do_test_with( Map& testMap )
        {
            typedef InsertThread<Map> insert_thread;
            typedef DeleteThread<Map> delete_thread;

            m_nInsThreadCount.store( c_nInsThreadCount, atomics::memory_order_release );

            CppUnitMini::ThreadPool pool( *this );
            pool.add( new insert_thread( pool, testMap ), c_nInsThreadCount );
            pool.add( new delete_thread( pool, testMap ), c_nDelThreadCount ? c_nDelThreadCount : cds::OS::topology::processor_count());
            pool.run();
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration());

            size_t nInsertSuccess = 0;
            size_t nInsertFailed = 0;
            size_t nDeleteSuccess = 0;
            size_t nDeleteFailed = 0;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                insert_thread * pThread = dynamic_cast<insert_thread *>( *it );
                if ( pThread ) {
                    nInsertSuccess += pThread->m_nInsertSuccess;
                    nInsertFailed += pThread->m_nInsertFailed;
                }
                else {
                    delete_thread * p = static_cast<delete_thread *>( *it );
                    nDeleteSuccess += p->m_nDeleteSuccess;
                    nDeleteFailed += p->m_nDeleteFailed;
                }
            }

            CPPUNIT_MSG( "  Totals (success/failed): \n\t"
                << "      Insert=" << nInsertSuccess << '/' << nInsertFailed << "\n\t"
                << "      Delete=" << nDeleteSuccess << '/' << nDeleteFailed << "\n\t"
                );
            CPPUNIT_CHECK( nInsertSuccess == c_nMapSize * c_nInsThreadCount );
            CPPUNIT_CHECK( nInsertFailed == 0 );

            analyze( testMap );
        }

        template <class Map>
        void do_test_extract_with( Map& testMap )
        {
            typedef InsertThread<Map> insert_thread;
            typedef DeleteThread<Map> delete_thread;
            typedef ExtractThread< typename Map::gc, Map > extract_thread;

            m_nInsThreadCount.store( c_nInsThreadCount, atomics::memory_order_release );

            CppUnitMini::ThreadPool pool( *this );
            pool.add( new insert_thread( pool, testMap ), c_nInsThreadCount );
            if ( c_nDelThreadCount )
                pool.add( new delete_thread( pool, testMap ), c_nDelThreadCount );
            if ( c_nExtractThreadCount )
                pool.add( new extract_thread( pool, testMap ), c_nExtractThreadCount );
            pool.run();
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration());

            size_t nInsertSuccess = 0;
            size_t nInsertFailed = 0;
            size_t nDeleteSuccess = 0;
            size_t nDeleteFailed = 0;
            size_t nExtractSuccess = 0;
            size_t nExtractFailed = 0;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                insert_thread * pThread = dynamic_cast<insert_thread *>( *it );
                if ( pThread ) {
                    nInsertSuccess += pThread->m_nInsertSuccess;
                    nInsertFailed += pThread->m_nInsertFailed;
                }
                else {
                    delete_thread * p = dynamic_cast<delete_thread *>( *it );
                    if ( p ) {
                        nDeleteSuccess += p->m_nDeleteSuccess;
                        nDeleteFailed += p->m_nDeleteFailed;
                    }
                    else {
                        extract_thread * pExtract = dynamic_cast<extract_thread *>( *it );
                        assert( pExtract );
                        nExtractSuccess += pExtract->m_nDeleteSuccess;
                        nExtractFailed += pExtract->m_nDeleteFailed;
                    }
                }
            }

            CPPUNIT_MSG( "  Totals (success/failed): \n\t"
                << "      Insert=" << nInsertSuccess << '/' << nInsertFailed << "\n\t"
                << "      Delete=" << nDeleteSuccess << '/' << nDeleteFailed << "\n\t"
                << "      Extract=" << nExtractSuccess << '/' << nExtractFailed << "\n\t"
                );
            CPPUNIT_CHECK( nInsertSuccess == c_nMapSize * c_nInsThreadCount );
            CPPUNIT_CHECK( nInsertFailed == 0 );

            analyze( testMap );
        }

        template <class Map>
        void analyze( Map& testMap )
        {
            cds::OS::Timer    timer;

            // All even keys must be in the map
            {
                size_t nErrorCount = 0;
                CPPUNIT_MSG( "  Check even keys..." );
                for ( size_t n = 0; n < c_nMapSize; n +=2 ) {
                    for ( size_t i = 0; i < c_nInsThreadCount; ++i ) {
                        if ( !testMap.contains( key_type(n, i))) {
                            if ( ++nErrorCount < 10 ) {
                                CPPUNIT_MSG( "key " << n << "-" << i << " is not found!");
                            }
                        }
                    }
                }
                CPPUNIT_CHECK_EX( nErrorCount == 0, "Totals: " << nErrorCount << " keys is not found");
            }

            print_stat(testMap);

            check_before_cleanup( testMap );

            CPPUNIT_MSG( "  Clear map (single-threaded)..." );
            timer.reset();
            testMap.clear();
            CPPUNIT_MSG( "   Duration=" << timer.duration());
            CPPUNIT_CHECK_EX( testMap.empty(), ((long long) testMap.size()));

            additional_check( testMap );
            print_stat( testMap );

            additional_cleanup( testMap );
        }

        template <class Map>
        void run_test()
        {
            static_assert( Map::c_bExtractSupported, "Map class must support extract() method" );

            CPPUNIT_MSG( "Thread count: insert=" << c_nInsThreadCount
                << ", delete=" << c_nDelThreadCount
                << ", extract=" << c_nExtractThreadCount
                << "; set size=" << c_nMapSize
                );
            if ( Map::c_bLoadFactorDepended ) {
                for ( c_nLoadFactor = 1; c_nLoadFactor <= c_nMaxLoadFactor; c_nLoadFactor *= 2 ) {
                    CPPUNIT_MSG( "Load factor=" << c_nLoadFactor );
                    do_test_extract<Map>();
                    if ( c_bPrintGCState )
                        print_gc_state();
                }
            }
            else
                do_test_extract<Map>();
        }

        template <class Map>
        void run_test_no_extract()
        {
            static_assert( !Map::c_bExtractSupported, "Map class must not support extract() method" );

            CPPUNIT_MSG( "Insert thread count=" << c_nInsThreadCount
                << " delete thread count=" << c_nDelThreadCount
                << " set size=" << c_nMapSize
                );
            if ( Map::c_bLoadFactorDepended ) {
                for ( c_nLoadFactor = 1; c_nLoadFactor <= c_nMaxLoadFactor; c_nLoadFactor *= 2 ) {
                    CPPUNIT_MSG( "Load factor=" << c_nLoadFactor );
                    do_test<Map>();
                    if ( c_bPrintGCState )
                        print_gc_state();
                }
            }
            else
                do_test<Map>();
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg );
        virtual void endTestCase();

#   include "map2/map_defs.h"
        CDSUNIT_DECLARE_MichaelMap
        CDSUNIT_DECLARE_SplitList
        CDSUNIT_DECLARE_SkipListMap
        CDSUNIT_DECLARE_EllenBinTreeMap
        CDSUNIT_DECLARE_BronsonAVLTreeMap
        CDSUNIT_DECLARE_FeldmanHashMap_fixed
        //CDSUNIT_DECLARE_FeldmanHashMap_city
        CDSUNIT_DECLARE_CuckooMap

        CPPUNIT_TEST_SUITE(Map_DelOdd)
            CDSUNIT_TEST_MichaelMap
            CDSUNIT_TEST_SplitList
            CDSUNIT_TEST_SkipListMap
            CDSUNIT_TEST_EllenBinTreeMap
            CDSUNIT_TEST_BronsonAVLTreeMap
            CDSUNIT_TEST_FeldmanHashMap_fixed
            //CDSUNIT_TEST_FeldmanHashMap_city
            CDSUNIT_TEST_CuckooMap
        CPPUNIT_TEST_SUITE_END();

        // Not implemented yet
        ////CDSUNIT_DECLARE_StripedMap
        ////CDSUNIT_DECLARE_RefinableMap
        ////CDSUNIT_DECLARE_StdMap
    };
} // namespace map2
