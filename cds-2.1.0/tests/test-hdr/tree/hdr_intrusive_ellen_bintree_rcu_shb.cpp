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

#include "tree/hdr_intrusive_bintree.h"
#include <cds/urcu/signal_buffered.h>
#include <cds/intrusive/ellen_bintree_rcu.h>

#include "tree/hdr_intrusive_ellen_bintree_pool_rcu.h"
#include "unit/print_ellenbintree_stat.h"

namespace tree {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
    namespace ci = cds::intrusive;
    namespace co = cds::opt;
    namespace {
        typedef cds::urcu::gc< cds::urcu::signal_buffered<> > rcu_type;

        typedef ci::ellen_bintree::node<rcu_type>                           leaf_node;
        typedef IntrusiveBinTreeHdrTest::base_hook_value< leaf_node >       base_value;

        struct print_stat {
            template <typename Tree>
            void operator()( Tree const& t)
            {
                std::cout << t.statistics();
            }
        };

        typedef ci::ellen_bintree::internal_node< IntrusiveBinTreeHdrTest::key_type, leaf_node > internal_node;
        typedef ci::ellen_bintree::update_desc< leaf_node, internal_node >   update_desc;
    }
#endif

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_less()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::less< less< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_less_stat()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::less< less< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::stat< ci::ellen_bintree::stat<> >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_cmp()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::compare< compare< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_cmpless()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::compare< compare< base_value > >
                ,co::less< less< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_less_ic()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::less< less< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::item_counter< cds::atomicity::item_counter >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_cmp_ic()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::compare< compare< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::item_counter< cds::atomicity::item_counter >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_cmp_ic_stat()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::compare< compare< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::item_counter< cds::atomicity::item_counter >
                ,co::stat< ci::ellen_bintree::stat<> >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_cmp_ic_stat_yield()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::compare< compare< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::item_counter< cds::atomicity::item_counter >
                ,co::stat< ci::ellen_bintree::stat<> >
                ,co::back_off< cds::backoff::yield >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_less_pool()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::less< less< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::node_allocator< cds::memory::pool_allocator< internal_node, ellen_bintree_rcu::internal_node_pool_accessor > >
                ,ci::ellen_bintree::update_desc_allocator< cds::memory::pool_allocator< update_desc, ellen_bintree_rcu::update_desc_pool_accessor > >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }

    void IntrusiveBinTreeHdrTest::EllenBinTree_rcu_shb_base_less_pool_ic_stat()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef ci::EllenBinTree< rcu_type, key_type, base_value,
            ci::ellen_bintree::make_traits<
                ci::opt::hook< ci::ellen_bintree::base_hook< co::gc< rcu_type > > >
                ,ci::ellen_bintree::key_extractor< key_extractor< base_value > >
                ,co::less< less< base_value > >
                ,ci::opt::disposer< disposer< base_value > >
                ,co::node_allocator< cds::memory::pool_allocator< internal_node, ellen_bintree_rcu::internal_node_pool_accessor > >
                ,ci::ellen_bintree::update_desc_allocator< cds::memory::pool_allocator< update_desc, ellen_bintree_rcu::update_desc_pool_accessor > >
                ,co::item_counter< cds::atomicity::item_counter >
                ,co::stat< ci::ellen_bintree::stat<> >
            >::type
        > tree_type;

        test_rcu<tree_type, print_stat>();
#endif
    }


} //namespace tree
