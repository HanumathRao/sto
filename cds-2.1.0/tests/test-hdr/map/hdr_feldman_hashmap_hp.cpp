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

#include "map/hdr_feldman_hashmap.h"
#include <cds/container/feldman_hashmap_hp.h>
#include "unit/print_feldman_hashset_stat.h"

namespace map {
    namespace {
        typedef cds::gc::HP gc_type;
    } // namespace

    void FeldmanHashMapHdrTest::hp_nohash()
    {
        typedef cc::FeldmanHashMap< gc_type, size_t, Item > map_type;

        test_hp<map_type>(4, 2);
    }

    void FeldmanHashMapHdrTest::hp_stdhash()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef std::hash<size_t> hash;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;

        test_hp<map_type>(4, 2);
    }

    void FeldmanHashMapHdrTest::hp_hash128()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef hash128::make hash;
            typedef hash128::less less;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(4, 2);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::hash< hash128::make >
                , co::less< hash128::less >
            >::type
        > map_type2;
        test_hp<map_type2>(4, 2);
    }

    void FeldmanHashMapHdrTest::hp_nohash_stat()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef cc::feldman_hashmap::stat<> stat;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(4, 2);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::stat< cc::feldman_hashmap::stat<>>
            >::type
        > map_type2;
        test_hp<map_type2>(4, 2);
    }

    void FeldmanHashMapHdrTest::hp_stdhash_stat()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef std::hash<size_t> hash;
            typedef cc::feldman_hashmap::stat<> stat;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(4, 2);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::stat< cc::feldman_hashmap::stat<>>
                ,co::hash< std::hash<size_t>>
            >::type
        > map_type2;
        test_hp<map_type2>(4, 2);
    }

    void FeldmanHashMapHdrTest::hp_hash128_stat()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef cc::feldman_hashmap::stat<> stat;
            typedef hash128::make hash;
            typedef hash128::cmp compare;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(4, 2);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::stat< cc::feldman_hashmap::stat<>>
                , co::hash< hash128::make >
                , co::compare< hash128::cmp >
            >::type
        > map_type2;
        test_hp<map_type2>(4, 2);
    }

    void FeldmanHashMapHdrTest::hp_nohash_5_3()
    {
        typedef cc::FeldmanHashMap< gc_type, size_t, Item > map_type;

        test_hp<map_type>(5, 3);
    }


    void FeldmanHashMapHdrTest::hp_stdhash_5_3()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef std::hash<size_t> hash;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;

        test_hp<map_type>(5, 3);
    }

    void FeldmanHashMapHdrTest::hp_nohash_5_3_stat()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef cc::feldman_hashmap::stat<> stat;
            typedef cds::backoff::empty back_off;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(5, 3);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
            co::stat< cc::feldman_hashmap::stat<>>
            , co::back_off< cds::backoff::empty >
            >::type
        > map_type2;
        test_hp<map_type2>(5, 3);
    }

    void FeldmanHashMapHdrTest::hp_stdhash_5_3_stat()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef cc::feldman_hashmap::stat<> stat;
            typedef cds::backoff::empty back_off;
            typedef std::hash<size_t> hash;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(5, 3);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::stat< cc::feldman_hashmap::stat<>>
                ,co::back_off< cds::backoff::empty >
                ,co::hash< std::hash<size_t>>
            >::type
        > map_type2;
        test_hp<map_type2>(5, 3);
    }

    void FeldmanHashMapHdrTest::hp_hash128_4_3()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef hash128::make hash;
            typedef hash128::less less;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(4, 3);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::hash< hash128::make >
                , co::less< hash128::less >
            >::type
        > map_type2;
        test_hp<map_type2>(4, 3);
    }

    void FeldmanHashMapHdrTest::hp_hash128_4_3_stat()
    {
        struct traits : public cc::feldman_hashmap::traits {
            typedef hash128::make hash;
            typedef hash128::less less;
            typedef cc::feldman_hashmap::stat<> stat;
            typedef co::v::sequential_consistent memory_model;
        };
        typedef cc::FeldmanHashMap< gc_type, size_t, Item, traits > map_type;
        test_hp<map_type>(4, 3);

        typedef cc::FeldmanHashMap< gc_type, size_t, Item,
            typename cc::feldman_hashmap::make_traits<
                co::hash< hash128::make >
                , co::less< hash128::less >
                , co::stat< cc::feldman_hashmap::stat<>>
                , co::memory_model< co::v::sequential_consistent >
            >::type
        > map_type2;
        test_hp<map_type2>(4, 3);
    }
} // namespace map

CPPUNIT_TEST_SUITE_REGISTRATION(map::FeldmanHashMapHdrTest);
