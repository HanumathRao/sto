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

#include "map/hdr_map.h"
#include <cds/container/michael_list_dhp.h>
#include <cds/container/split_list_map.h>

namespace map {

    namespace {
        struct DHP_cmp_traits: public cc::split_list::traits
        {
            typedef cc::michael_list_tag                ordered_list;
            typedef HashMapHdrTest::hash_int            hash;
            typedef HashMapHdrTest::simple_item_counter item_counter;
            typedef cc::opt::v::relaxed_ordering        memory_model;
            enum { dynamic_bucket_table = false };

            struct ordered_list_traits: public cc::michael_list::traits
            {
                typedef HashMapHdrTest::cmp   compare;
            };
        };

        struct DHP_less_traits: public cc::split_list::traits
        {
            typedef cc::michael_list_tag                ordered_list;
            typedef HashMapHdrTest::hash_int            hash;
            typedef HashMapHdrTest::simple_item_counter item_counter;
            typedef cc::opt::v::sequential_consistent                      memory_model;
            enum { dynamic_bucket_table = true };

            struct ordered_list_traits: public cc::michael_list::traits
            {
                typedef HashMapHdrTest::less   less;
            };
        };

        struct DHP_cmpmix_traits: public cc::split_list::traits
        {
            typedef cc::michael_list_tag                ordered_list;
            typedef HashMapHdrTest::hash_int            hash;
            typedef HashMapHdrTest::simple_item_counter item_counter;

            struct ordered_list_traits: public cc::michael_list::traits
            {
                typedef HashMapHdrTest::cmp   compare;
                typedef std::less<HashMapHdrTest::key_type>     less;
            };
        };

        struct DHP_cmpmix_stat_traits : public DHP_cmpmix_traits
        {
            typedef cc::split_list::stat<> stat;
        };
    }

    void HashMapHdrTest::Split_DHP_cmp()
    {
        // traits-based version
        typedef cc::SplitListMap< cds::gc::DHP, key_type, value_type, DHP_cmp_traits > map_type;
        test_int< map_type >();

        // option-based version
        typedef cc::SplitListMap< cds::gc::DHP,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::memory_model< cc::opt::v::relaxed_ordering >
                ,cc::split_list::dynamic_bucket_table< true >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::compare< cmp >
                    >::type
                >
            >::type
        > opt_map;
        test_int< opt_map >();
    }

    void HashMapHdrTest::Split_DHP_less()
    {
        // traits-based version
        typedef cc::SplitListMap< cds::gc::DHP, key_type, value_type, DHP_less_traits > map_type;
        test_int< map_type >();

        // option-based version
        typedef cc::SplitListMap< cds::gc::DHP,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::memory_model< cc::opt::v::relaxed_ordering >
                ,cc::split_list::dynamic_bucket_table< false >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::less< less >
                    >::type
                >
            >::type
        > opt_map;
        test_int< opt_map >();
    }

    void HashMapHdrTest::Split_DHP_cmpmix()
    {
        // traits-based version
        typedef cc::SplitListMap< cds::gc::DHP, key_type, value_type, DHP_cmpmix_traits > map_type;
        test_int< map_type >();

        // option-based version
        typedef cc::SplitListMap< cds::gc::DHP,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                    cc::opt::less< std::less<key_type> >
                        ,cc::opt::compare< cmp >
                    >::type
                >
            >::type
        > opt_map;
        test_int< opt_map >();
    }

    void HashMapHdrTest::Split_DHP_cmpmix_stat()
    {
        // traits-based version
        typedef cc::SplitListMap< cds::gc::DHP, key_type, value_type, DHP_cmpmix_stat_traits > map_type;
        test_int< map_type >();

        // option-based version
        typedef cc::SplitListMap< cds::gc::DHP,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::stat< cc::split_list::stat<> >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                    cc::opt::less< std::less<key_type> >
                        ,cc::opt::compare< cmp >
                    >::type
                >
            >::type
        > opt_map;
        test_int< opt_map >();
    }

} // namespace map

