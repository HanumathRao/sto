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

#include "set2/set_type.h"
#include "cppunit/thread.h"

#include <vector>

namespace set2 {

#define TEST_CASE(TAG, X)  void X();

    class Set_InsDel_string: public CppUnitMini::TestCase
    {
    public:
        size_t c_nSetSize = 1000000;            // set size
        size_t c_nInsertThreadCount = 4;  // count of insertion thread
        size_t c_nDeleteThreadCount = 4;  // count of deletion thread
        size_t c_nThreadPassCount = 4;    // pass count for each thread
        size_t c_nMaxLoadFactor = 8;      // maximum load factor
        bool   c_bPrintGCState = true;

        size_t  c_nCuckooInitialSize = 1024;// initial size for CuckooSet
        size_t  c_nCuckooProbesetSize = 16; // CuckooSet probeset size (only for list-based probeset)
        size_t  c_nCuckooProbesetThreshold = 0; // CUckooSet probeset threshold (0 - use default)

        size_t c_nFeldmanSet_HeadBits = 10;
        size_t c_nFeldmanSet_ArrayBits = 4;

        size_t c_nLoadFactor = 2;

    private:
        typedef std::string key_type;
        typedef size_t      value_type;

        const std::vector<std::string> *  m_parrString;

        template <class Set>
        class Inserter: public CppUnitMini::TestThread
        {
            Set&     m_Set;
            typedef typename Set::value_type    keyval_type;

            virtual Inserter *    clone()
            {
                return new Inserter( *this );
            }
        public:
            size_t  m_nInsertSuccess;
            size_t  m_nInsertFailed;

        public:
            Inserter( CppUnitMini::ThreadPool& pool, Set& rSet )
                : CppUnitMini::TestThread( pool )
                , m_Set( rSet )
            {}
            Inserter( Inserter& src )
                : CppUnitMini::TestThread( src )
                , m_Set( src.m_Set )
            {}

            Set_InsDel_string&  getTest()
            {
                return reinterpret_cast<Set_InsDel_string&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                Set& rSet = m_Set;

                m_nInsertSuccess =
                    m_nInsertFailed = 0;

                const std::vector<std::string>& arrString = *getTest().m_parrString;
                size_t nArrSize = arrString.size();
                size_t const nSetSize = getTest().c_nSetSize;
                size_t const nPassCount = getTest().c_nThreadPassCount;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < nSetSize; ++nItem ) {
                            if ( rSet.insert( keyval_type(arrString[nItem % nArrSize], nItem * 8) ) )
                                ++m_nInsertSuccess;
                            else
                                ++m_nInsertFailed;
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = nSetSize; nItem > 0; --nItem ) {
                            if ( rSet.insert( keyval_type( arrString[nItem % nArrSize], nItem * 8) ) )
                                ++m_nInsertSuccess;
                            else
                                ++m_nInsertFailed;
                        }
                    }
                }
            }
        };

        template <class Set>
        class Deleter: public CppUnitMini::TestThread
        {
            Set&     m_Set;

            virtual Deleter *    clone()
            {
                return new Deleter( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            Deleter( CppUnitMini::ThreadPool& pool, Set& rSet )
                : CppUnitMini::TestThread( pool )
                , m_Set( rSet )
            {}
            Deleter( Deleter& src )
                : CppUnitMini::TestThread( src )
                , m_Set( src.m_Set )
            {}

            Set_InsDel_string&  getTest()
            {
                return reinterpret_cast<Set_InsDel_string&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                Set& rSet = m_Set;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                const std::vector<std::string>& arrString = *getTest().m_parrString;
                size_t nArrSize = arrString.size();
                size_t const nSetSize = getTest().c_nSetSize;
                size_t const nPassCount = getTest().c_nThreadPassCount;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < nSetSize; ++nItem ) {
                            if ( rSet.erase( arrString[nItem % nArrSize] ) )
                                ++m_nDeleteSuccess;
                            else
                                ++m_nDeleteFailed;
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = nSetSize; nItem > 0; --nItem ) {
                            if ( rSet.erase( arrString[nItem % nArrSize] ) )
                                ++m_nDeleteSuccess;
                            else
                                ++m_nDeleteFailed;
                        }
                    }
                }
            }
        };

        template <typename GC, class Set>
        class Extractor: public CppUnitMini::TestThread
        {
            Set&     m_Set;

            virtual Extractor *    clone()
            {
                return new Extractor( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            Extractor( CppUnitMini::ThreadPool& pool, Set& rSet )
                : CppUnitMini::TestThread( pool )
                , m_Set( rSet )
            {}
            Extractor( Extractor& src )
                : CppUnitMini::TestThread( src )
                , m_Set( src.m_Set )
            {}

            Set_InsDel_string&  getTest()
            {
                return reinterpret_cast<Set_InsDel_string&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                Set& rSet = m_Set;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                typename Set::guarded_ptr gp;

                const std::vector<std::string>& arrString = *getTest().m_parrString;
                size_t nArrSize = arrString.size();
                size_t const nSetSize = getTest().c_nSetSize;
                size_t const nPassCount = getTest().c_nThreadPassCount;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < nSetSize; ++nItem ) {
                            gp = rSet.extract( arrString[nItem % nArrSize]);
                            if (  gp )
                                ++m_nDeleteSuccess;
                            else
                                ++m_nDeleteFailed;
                            gp.release();
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = nSetSize; nItem > 0; --nItem ) {
                            gp = rSet.extract( arrString[nItem % nArrSize]);
                            if ( gp )
                                ++m_nDeleteSuccess;
                            else
                                ++m_nDeleteFailed;
                            gp.release();
                        }
                    }
                }
            }
        };

        template <typename RCU, class Set>
        class Extractor<cds::urcu::gc<RCU>, Set >: public CppUnitMini::TestThread
        {
            Set&     m_Set;

            virtual Extractor *    clone()
            {
                return new Extractor( *this );
            }
        public:
            size_t  m_nDeleteSuccess;
            size_t  m_nDeleteFailed;

        public:
            Extractor( CppUnitMini::ThreadPool& pool, Set& rSet )
                : CppUnitMini::TestThread( pool )
                , m_Set( rSet )
            {}
            Extractor( Extractor& src )
                : CppUnitMini::TestThread( src )
                , m_Set( src.m_Set )
            {}

            Set_InsDel_string&  getTest()
            {
                return reinterpret_cast<Set_InsDel_string&>( m_Pool.m_Test );
            }

            virtual void init() { cds::threading::Manager::attachThread()   ; }
            virtual void fini() { cds::threading::Manager::detachThread()   ; }

            virtual void test()
            {
                Set& rSet = m_Set;

                m_nDeleteSuccess =
                    m_nDeleteFailed = 0;

                typename Set::exempt_ptr xp;

                const std::vector<std::string>& arrString = *getTest().m_parrString;
                size_t nArrSize = arrString.size();
                size_t const nSetSize = getTest().c_nSetSize;
                size_t const nPassCount = getTest().c_nThreadPassCount;

                if ( m_nThreadNo & 1 ) {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = 0; nItem < nSetSize; ++nItem ) {
                            if ( Set::c_bExtractLockExternal ) {
                                {
                                    typename Set::rcu_lock l;
                                    xp = rSet.extract( arrString[nItem % nArrSize] );
                                    if ( xp )
                                        ++m_nDeleteSuccess;
                                    else
                                        ++m_nDeleteFailed;
                                }
                            }
                            else {
                                xp = rSet.extract( arrString[nItem % nArrSize] );
                                if ( xp )
                                    ++m_nDeleteSuccess;
                                else
                                    ++m_nDeleteFailed;
                            }
                            xp.release();
                        }
                    }
                }
                else {
                    for ( size_t nPass = 0; nPass < nPassCount; ++nPass ) {
                        for ( size_t nItem = nSetSize; nItem > 0; --nItem ) {
                            if ( Set::c_bExtractLockExternal ) {
                                {
                                    typename Set::rcu_lock l;
                                    xp = rSet.extract( arrString[nItem % nArrSize] );
                                    if ( xp )
                                        ++m_nDeleteSuccess;
                                    else
                                        ++m_nDeleteFailed;
                                }
                            }
                            else {
                                xp = rSet.extract( arrString[nItem % nArrSize] );
                                if ( xp )
                                    ++m_nDeleteSuccess;
                                else
                                    ++m_nDeleteFailed;
                            }
                            xp.release();
                        }
                    }
                }
            }
        };

    protected:
        template <class Set>
        void do_test( Set& testSet )
        {
            typedef Inserter<Set>       InserterThread;
            typedef Deleter<Set>        DeleterThread;
            cds::OS::Timer    timer;

            CppUnitMini::ThreadPool pool( *this );
            pool.add( new InserterThread( pool, testSet ), c_nInsertThreadCount );
            pool.add( new DeleterThread( pool, testSet ), c_nDeleteThreadCount );
            pool.run();
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() );

            size_t nInsertSuccess = 0;
            size_t nInsertFailed = 0;
            size_t nDeleteSuccess = 0;
            size_t nDeleteFailed = 0;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                InserterThread * pThread = dynamic_cast<InserterThread *>( *it );
                if ( pThread ) {
                    nInsertSuccess += pThread->m_nInsertSuccess;
                    nInsertFailed += pThread->m_nInsertFailed;
                }
                else {
                    DeleterThread * p = static_cast<DeleterThread *>( *it );
                    nDeleteSuccess += p->m_nDeleteSuccess;
                    nDeleteFailed += p->m_nDeleteFailed;
                }
            }

            CPPUNIT_MSG( "    Totals: Ins succ=" << nInsertSuccess
                << " Del succ=" << nDeleteSuccess << "\n"
                      << "          : Ins fail=" << nInsertFailed
                << " Del fail=" << nDeleteFailed
                << " Set size=" << testSet.size()
                );


            CPPUNIT_MSG( "  Clear set (single-threaded)..." );
            timer.reset();
            for ( size_t i = 0; i < m_parrString->size(); ++i )
                testSet.erase( (*m_parrString)[i] );
            CPPUNIT_MSG( "   Duration=" << timer.duration() );
            CPPUNIT_ASSERT( testSet.empty() );

            additional_check( testSet );
            print_stat(  testSet  );
            additional_cleanup( testSet );
        }

        template <class Set>
        void do_test_extract( Set& testSet )
        {
            typedef Inserter<Set>       InserterThread;
            typedef Deleter<Set>        DeleterThread;
            typedef Extractor<typename Set::gc, Set> ExtractThread;

            size_t nDelThreadCount = c_nDeleteThreadCount / 2;

            CppUnitMini::ThreadPool pool( *this );
            pool.add( new InserterThread( pool, testSet ), c_nInsertThreadCount );
            pool.add( new DeleterThread( pool, testSet ), nDelThreadCount );
            pool.add( new ExtractThread( pool, testSet ), c_nDeleteThreadCount - nDelThreadCount );
            pool.run();
            CPPUNIT_MSG( "   Duration=" << pool.avgDuration() );

            size_t nInsertSuccess = 0;
            size_t nInsertFailed = 0;
            size_t nDeleteSuccess = 0;
            size_t nDeleteFailed = 0;
            size_t nExtractSuccess = 0;
            size_t nExtractFailed = 0;
            for ( CppUnitMini::ThreadPool::iterator it = pool.begin(); it != pool.end(); ++it ) {
                InserterThread * pThread = dynamic_cast<InserterThread *>( *it );
                if ( pThread ) {
                    nInsertSuccess += pThread->m_nInsertSuccess;
                    nInsertFailed += pThread->m_nInsertFailed;
                }
                else {
                    DeleterThread * p = dynamic_cast<DeleterThread *>( *it );
                    if ( p ) {
                        nDeleteSuccess += p->m_nDeleteSuccess;
                        nDeleteFailed += p->m_nDeleteFailed;
                    }
                    else {
                        ExtractThread * pExtract = dynamic_cast<ExtractThread *>( *it );
                        assert( pExtract );
                        nExtractSuccess += pExtract->m_nDeleteSuccess;
                        nExtractFailed += pExtract->m_nDeleteFailed;
                    }
                }
            }

            CPPUNIT_MSG( "    Totals: Ins succ=" << nInsertSuccess
                << " Del succ=" << nDeleteSuccess
                << " Extract succ= " << nExtractSuccess << "\n"
                << "          : Ins fail=" << nInsertFailed
                << " Del fail=" << nDeleteFailed
                << " Extract fail=" << nExtractFailed
                << " Set size=" << testSet.size()
                );


            CPPUNIT_MSG( "  Clear set (single-threaded)..." );
            cds::OS::Timer    timer;
            for ( size_t i = 0; i < m_parrString->size(); ++i )
                testSet.erase( (*m_parrString)[i] );
            CPPUNIT_MSG( "   Duration=" << timer.duration() );
            CPPUNIT_ASSERT( testSet.empty() );

            additional_check( testSet );
            print_stat(  testSet  );
            additional_cleanup( testSet );
        }

        template <class Set>
        void run_test()
        {
            m_parrString = &CppUnitMini::TestCase::getTestStrings();

            CPPUNIT_MSG( "Thread count: insert=" << c_nInsertThreadCount
                << " delete=" << c_nDeleteThreadCount
                << " pass count=" << c_nThreadPassCount
                << " set size=" << c_nSetSize
                );

            if ( Set::c_bLoadFactorDepended ) {
                for ( c_nLoadFactor = 1; c_nLoadFactor <= c_nMaxLoadFactor; c_nLoadFactor *= 2 ) {
                    CPPUNIT_MSG("  LoadFactor = " << c_nLoadFactor );
                    Set s( *this );
                    do_test( s );
                    if ( c_bPrintGCState )
                        print_gc_state();
                }
            }
            else {
                Set s( *this );
                do_test( s );
                if ( c_bPrintGCState )
                    print_gc_state();
            }
        }

        template <class Set>
        void run_test_extract()
        {
            m_parrString = &CppUnitMini::TestCase::getTestStrings();

            CPPUNIT_MSG( "Thread count: insert=" << c_nInsertThreadCount
                << " delete=" << c_nDeleteThreadCount
                << " pass count=" << c_nThreadPassCount
                << " set size=" << c_nSetSize
                );

            if ( Set::c_bLoadFactorDepended ) {
                for ( c_nLoadFactor = 1; c_nLoadFactor <= c_nMaxLoadFactor; c_nLoadFactor *= 2 ) {
                    CPPUNIT_MSG("  LoadFactor = " << c_nLoadFactor );
                    Set s( *this );
                    do_test_extract( s );
                    if ( c_bPrintGCState )
                        print_gc_state();
                }
            }
            else {
                Set s( *this );
                do_test_extract( s );
                if ( c_bPrintGCState )
                    print_gc_state();
            }
        }

        void setUpParams( const CppUnitMini::TestCfg& cfg );

#   include "set2/set_defs.h"
        CDSUNIT_DECLARE_MichaelSet
        CDSUNIT_DECLARE_SplitList
        CDSUNIT_DECLARE_StripedSet
        CDSUNIT_DECLARE_RefinableSet
        CDSUNIT_DECLARE_CuckooSet
        CDSUNIT_DECLARE_SkipListSet
        CDSUNIT_DECLARE_EllenBinTreeSet
        CDSUNIT_DECLARE_FeldmanHashSet_stdhash
        CDSUNIT_DECLARE_FeldmanHashSet_city
        CDSUNIT_DECLARE_StdSet

        CPPUNIT_TEST_SUITE_(Set_InsDel_string, "Map_InsDel_func")
            CDSUNIT_TEST_MichaelSet
            CDSUNIT_TEST_SplitList
            CDSUNIT_TEST_SkipListSet
            CDSUNIT_TEST_FeldmanHashSet_stdhash
            CDSUNIT_TEST_FeldmanHashSet_city
            CDSUNIT_TEST_EllenBinTreeSet
            CDSUNIT_TEST_StripedSet
            CDSUNIT_TEST_RefinableSet
            CDSUNIT_TEST_CuckooSet
            CDSUNIT_TEST_StdSet
        CPPUNIT_TEST_SUITE_END();
    };
} // namespace set2
