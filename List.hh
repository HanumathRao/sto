#pragma once

#include "TaggedLow.hh"
#include "Interface.hh"

#ifndef STO_NO_STM
#include "Transaction.hh"
#endif

template<typename T>
class DefaultCompare {
public:
  int operator()(const T& t1, const T& t2) {
    if (t1 < t2)
      return -1;
    return t2 < t1 ? 1 : 0;
  }
};

template <typename T, bool Duplicates = false, typename Compare = DefaultCompare<T>, bool Sorted = true, bool Opacity = false> class ListIterator;

template <typename T, bool Duplicates = false, typename Compare = DefaultCompare<T>, bool Sorted = true, bool Opacity = false>
class List 
#ifndef STO_NO_STM
: public Shared 
#endif
{
  friend class ListIterator<T, Duplicates, Compare, Sorted, Opacity>;
  typedef ListIterator<T, Duplicates, Compare, Sorted, Opacity> iterator;
public:
  List(Compare comp = Compare()) : head_(NULL), listsize_(0), listversion_(0), sizeversion_(0), comp_(comp) {
  }

private:
  // XXX: barf. version_type is the only one of these not actually used for
  // versions :)
  typedef TransactionTid::type version_type;
  typedef TVersion node_version_type;
  typedef TVersion size_version_type;

public:
    static constexpr version_type invalid_bit = TransactionTid::user_bit;

    static constexpr TransItem::flags_type insert_bit = TransItem::user0_bit;
    static constexpr TransItem::flags_type delete_bit = TransItem::user0_bit<<1;
    static constexpr TransItem::flags_type doupdate_bit = TransItem::user0_bit<<2;

  struct list_node {
    list_node(const T& val, list_node *next, bool invalid)
      : val(val), next(next), vers(Sto::initialized_tid() | (invalid ? (invalid_bit | TransactionTid::lock_bit | TThread::id()) : 0)) {
    }

    // used for delete commit
    void mark_invalid() {
      assert(vers.is_locked_here());
      auto new_version = vers | invalid_bit;
      fence();
      vers = new_version;
    }

    node_version_type version() {
      return vers;
    }

    void set_version(node_version_type new_v) {
      vers.set_version(new_v);
    }

    void set_version_unlock(version_type new_v) {
      vers.set_version_unlock(new_v);
    }

    bool try_lock() {
      return vers.try_lock();
    }
    void unlock() {
      vers.unlock();
    }

    bool is_valid() {
      return !(vers.value() & invalid_bit);
    }

    T val;
    list_node *next;
    node_version_type vers;
  };

  static constexpr list_node* list_key = nullptr;
  // Can't have non-NULL constexpr pointer
  static inline list_node* size_key() { return (list_node*)1; }

  bool find(const T& elem, T& val) {
    auto *ret = _find(elem);
    if (ret) {
      val = ret->val;
    }
    return !!ret;
  }

  T* find(const T& elem) {
    auto *ret = _find(elem);
    if (ret) {
      return &ret->val;
    }
    return NULL;
  }

  list_node* _find(const T& elem) {
    list_node *cur = head_;
    while (cur != NULL) {
      int c = comp_(cur->val, elem);
      if (c == 0) {
        return cur;
      }
      if (Sorted && c > 0) {
        return NULL;
      }
      cur = cur->next;
    }
    return NULL;
  }

  template <bool Txnal = false>
  list_node* _insert(const T& elem, bool *inserted = NULL) {
    if (inserted)
      *inserted = true;
    lock(listversion_);
    if (!Sorted && !Duplicates) {
      list_node *new_head = new list_node(elem, head_, Txnal);
      head_ = new_head;
      unlock(listversion_);
      return new_head;
    }

    list_node *prev = NULL;
    list_node *cur = head_;
    while (cur != NULL) {
      int c = comp_(cur->val, elem);
      if (!Duplicates && c == 0) {
        unlock(listversion_);
        if (inserted)
          *inserted = false;
        return cur;
      } else if (Sorted && c >= 0) {
        break;
      }
      prev = cur;
      cur = cur->next;
    }
    auto ret = new list_node(elem, cur, Txnal);
    if (prev) {
        prev->next = ret;
    } else {
        head_ = ret;
    }
    if (!Txnal)
      listsize_++;
    unlock(listversion_);
    return ret;
  }

  bool insert(const T& elem) {
    bool inserted;
    _insert<false>(elem, &inserted);
    return inserted;
  }

  template <bool Txnal>
  bool remove(const T& elem, bool locked = false) {
    return _remove<Txnal>([&] (list_node *n2) { return comp_(n2->val, elem) == 0; }, locked);
  }

  template <bool Txnal>
  bool remove(list_node *n, bool locked = false) {
    // TODO: doing this remove means we don't have to value compare, but we also
    // have to go through the whole list (possibly). Unclear which is better.
    return _remove<Txnal>([n] (list_node *n2) { return n == n2; }, locked);
  }

  template <bool Txnal, typename FoundFunc>
  bool _remove(FoundFunc found_f, bool locked = false) {
    if (!locked)
      lock(listversion_);
    list_node *prev = NULL;
    list_node *cur = head_;
    while (cur != NULL) {
      if (found_f(cur)) {
        cur->mark_invalid();
        if (prev) {
            prev->next = cur->next;
        } else {
            head_ = cur->next;
        }
        if (Txnal) {
          Transaction::rcu_delete(cur);
        } else {
          delete cur;
        }
        if (!Txnal)
          listsize_--;
        if (!locked)
          unlock(listversion_);
        return true;
      }
      prev = cur;
      cur = cur->next;
    }
    if (!locked)
      unlock(listversion_);
    return false;
  }

#ifndef STO_NO_STM
  T* transFind(const T& elem) {
    auto listv = sizeversion_;
    fence();
    auto *n = _find(elem);
    if (n) {
      auto version = n->version();
      fence();
      auto item = t_item(n);
      if (!validityCheck(n, item)) {
        Sto::abort();
        return NULL;
      }
      if (has_delete(item)) {
        return NULL;
      }
      item.observe(version);
    } else {
      // log list v#
      verify_list(listv);
      return NULL;
    }
    return &n->val;
  }

  bool transInsert(const T& elem) {
    bool inserted;
    auto *node = _insert<true>(elem, &inserted);
    auto item = t_item(node);
    if (!inserted) {
      auto version = node->version();
      fence();
      if (!validityCheck(node, item)) {
        Sto::abort();
        return false;
      }
      // intertransactional insert-then-insert = failed insert
      if (has_insert(item)) {
        return false;
      }
      // delete-then-insert, then insert -- failed insert
      if (has_doupdate(item)) {
        return false;
      }
      // delete-then-insert... (should really become an update...)
      if (has_delete(item)) {
	// delete already should have observed a version
        item.clear_write().add_write(elem);
        item.assign_flags(doupdate_bit);
        add_trans_size_offs(1);
        return true;
      }
      // "normal" failed insert
      // need to make sure it's still there at commit time
      item.observe(version);
      return false;
    }
    add_trans_size_offs(1);
    // we lock the list for inserts
    add_lock_list_item();
    item.add_write(0);
    item.add_flags(insert_bit);
    return true;
  }

  bool transDelete(const T& elem) {
    auto listv = sizeversion_;
    fence();
    auto *n = _find(elem);
    if (n) {
      auto version = n->version();
      fence();
      auto item = t_item(n);
      if (!validityCheck(n, item)) {
        Sto::abort();
        return false;
      }
      if (has_delete(item)) {
        // we're deleting our delete
        return false;
      }
      // delete-then-insert, then delete
      if (has_doupdate(item)) {
        // back to deleting
        item.assign_flags(delete_bit);
        add_trans_size_offs(-1);
        return true;
      }
      // insert-then-delete becomes absent-get
      if (has_insert(item)) {
        remove<true>(n);
        item.remove_read().remove_write().clear_flags(insert_bit);
        add_trans_size_offs(-1);
        // TODO: should have a count on add_lock_list_item so we can cancel that here
        // still need to make sure no one else inserts something
        verify_list(listv);
        return true;
      }
      item.assign_flags(delete_bit);
      // mark as a write
      item.add_write(0);
      // we also need to check that it's still valid at commit time (not
      // bothering with valid_check_only_bit optimization right now)
      item.observe(version);
      add_lock_list_item();
      add_trans_size_offs(-1);
      return true;
    } else {
      verify_list(listv);
      return false;
    }
  }

  inline void opacity_check() {
    // unused AFAIK
    assert(0);
    // When we check for opacity, we need to compare the latest listversion and not
    // the one at the beginning of the operation.
    assert(Opacity);
    auto listv = sizeversion_;
    fence();
    Sto::check_opacity(listv.value());
  }

  struct ListIter;

  ListIter transIter() {
    verify_list(sizeversion_);//TODO: rename
    return ListIter(this, head_, true);
  }

  size_t size() {
    verify_list(sizeversion_);
    return listsize_ + trans_size_offs();
  }

  size_t nontrans_size() const {
    return listsize_;
  }

#endif /* !STO_NO_STM */
    
  iterator begin() { return iterator(this, head_); }
  iterator end() { return iterator(this, NULL); }

  struct ListIter {
    ListIter() : us(NULL), cur(NULL) {}

    bool valid() const {
      return us != NULL;
    }

    bool hasNext() const {
      return !!cur;
    }

    void reset() {
      cur = us->head_;
    }

    T* next() {
      auto ret = cur ? &cur->val : NULL;
      if (cur)
        cur = cur->next;
      return ret;
    }

    bool transHasNext() const {
      return !!cur;
    }

    void transReset() {
      cur = us->head_;
      ensureValid();
    }

    T* transNext() {
      auto ret = cur ? &cur->val : NULL;
      if (cur)
        cur = cur->next;
      ensureValid();
      return ret;
    }

    T* transNthNext(int n) {
      T* ret = NULL;
      while (n > 0 && transHasNext()) {
        ret = transNext();
        n--;
      }
      if (n == 0)
        return ret;
      return NULL;
    }

private:
    ListIter(List *us, list_node *cur, bool trans) : us(us), cur(cur) {
      if (trans)
        ensureValid();
    }

    void ensureValid() {
#ifndef STO_NO_STM
      while (cur) {
        // need to check if this item already exists
        auto item = Sto::check_item(us, cur);
        if (!cur->is_valid()) {
          if (!item || !us->has_insert(*item)) {
            Sto::abort();
            // TODO: do we continue in this situation or abort?
            cur = cur->next;
            continue;
          }
        }
        if (item && us->has_delete(*item)) {
          cur = cur->next;
          continue;
        }
        break;
      }
#endif
    }

    friend class List;
    List *us;
    list_node *cur;
  };

  ListIter iter() {
    return ListIter(this, head_, false);
  }

  size_t unsafe_size() const {
      return listsize_;
  }

  void clear() {
      while (auto item = head_)
        remove<false>(head_, true);
  }

  void verify_list(size_version_type readv) {
    t_item(list_key).observe(readv);
    acquire_fence();
  }


  void lock(version_type& v) {
    TransactionTid::lock(v);
  }

  void unlock(version_type& v) {
    TransactionTid::unlock(v);
  }

  bool is_locked(version_type& v) {
    return TransactionTid::is_locked(v);
  }

#ifndef STO_NO_STM
    bool lock(TransItem& item, Transaction& txn) override {
      list_node *n = item.key<list_node*>();
      if (n == list_key) {
        return sizeversion_.try_lock();
      } else if (!has_insert(item)) {
        // we only lock non-inserts (removes, updates) so as to make our 
	// life harder (also it's not necessary for inserts).
        return n->try_lock();
      }
      return true;
    }

  bool check(TransItem& item, Transaction&) override {
    if (item.key<list_node*>() == list_key) {
      auto lv = sizeversion_;
      return lv.check_version(item.template read_value<size_version_type>());
    }
    auto n = item.key<list_node*>();
    if (!n->is_valid()) {
      return has_insert(item);
    }
    return n->version().check_version(item.template read_value<node_version_type>());
  }

  void install(TransItem& item, Transaction& t) override {
    // TODO: this item tracks the total size differential so we could just do 
    // a single fetch and add of the size delta here.
    if (item.key<list_node*>() == list_key)
      return;
    list_node *n = item.key<list_node*>();
    if (has_delete(item)) {
      remove<true>(n, true);
      listsize_--;
      // not super ideal that we have to change version
      // but we need to invalidate transSize() calls
      if (Opacity) {
        sizeversion_.set_version(t.commit_tid());
      } else {
        sizeversion_.inc_nonopaque_version();
      }
    } else if (has_doupdate(item)) {
      n->set_version(t.commit_tid());
      n->val = item.template write_value<T>();
    } else {
      // insert
      // clears the invalid bit too
      n->set_version_unlock(t.commit_tid());
      listsize_++;
      assert(Opacity);
      if (Opacity) {
        sizeversion_.set_version(t.commit_tid());
      } else {
        sizeversion_.inc_nonopaque_version();
      }
    }
  }

  void unlock(TransItem& item) override {
    auto n = item.key<list_node*>();
    if (n == list_key) {
      sizeversion_.unlock();
    } else if (!has_insert(item)) {
      n->unlock();
    }
  }

  void cleanup(TransItem& item, bool committed) override {
      if (!committed && (item.flags() & insert_bit)) {
          list_node *n = item.key<list_node*>();
          remove<true>(n);
      }
  }

  TransProxy t_item(list_node* node) {
    // can switch this to fresh_item to not read our writes
    return Sto::item(this, node);
  }

  bool has_insert(const TransItem& item) {
      return item.has_write() && !has_delete(item) && !has_doupdate(item);
  }

  bool has_delete(const TransItem& item) {
      return item.flags() & delete_bit;
  }

  bool has_doupdate(const TransItem& item) {
      return item.flags() & doupdate_bit;
  }

  void add_lock_list_item() {
    t_item(list_key).add_write(0);
  }

  void add_trans_size_offs(int size_offs) {
    // TODO: it'd be simpler and maybe even faster if this was just the 
    // write_value of our list_key item (and we renamed list_key to size_key)
    auto item = t_item(size_key());
    item.template set_stash<int>(item.template stash_value<int>(0) + size_offs);
  }

  int trans_size_offs() {
    return t_item(size_key()).template stash_value<int>(0);
  }
#endif /* !STO_NO_STM */

  bool validityCheck(list_node *n, TransItem& item) {
      return n->is_valid() || (item.flags() & insert_bit);
  }

  list_node *head_;
  long listsize_;
  // XXX: the current state is that listversion_ is just a lock, and
  // sizeversion_ is the list version ... :|
  version_type listversion_;
  size_version_type sizeversion_;
  Compare comp_;
};

    
template <typename T, bool Duplicates, typename Compare, bool Sorted, bool Opacity>
class ListIterator : public std::iterator<std::forward_iterator_tag, T> {
    typedef ListIterator<T, Duplicates, Compare, Sorted, Opacity> iterator;
    typedef List<T, Duplicates, Compare, Sorted, Opacity> list_type;
    typedef typename list_type::list_node list_node;
public:
    ListIterator(list_type * list, list_node* ptr) : myList(list), myPtr(ptr) {
        myList->list_verify(myList->sizeversion_);
    }
    ListIterator(const ListIterator& itr) : myList(itr.myList), myPtr(itr.myPtr) {}
    
    ListIterator& operator= (const ListIterator& v) {
        myList = v.myList;
        myPtr = v.myPtr;
        return *this;
    }
    
    bool operator==(iterator other) const {
        return (myList == other.myList) && (myPtr == other.myPtr);
    }
    
    bool operator!=(iterator other) const {
        return !(operator==(other));
    }
    
    T& operator*() {
        return myPtr->val; // Just returing the pointer to the value because this
                           // list does not transactionally track updates to list values.
    }
    
    void increment_ptr() {
        if (myPtr)
            myPtr = myPtr->next;
        while (myPtr) {
            // need to check if this item already exists
            auto item = Sto::check_item(myList, myPtr);
            if (!myPtr->is_valid()) {
                if (!item || !myList->has_insert(*item)) {
                    Sto::abort();
                    // TODO: do we continue in this situation or abort?
                    myPtr = myPtr->next;
                    continue;
                }
            }
            if (item && myList->has_delete(*item)) {
                myPtr = myPtr->next;
                continue;
            }
            break;
        }
    }
    
    /* This is the prefix case */
    iterator& operator++() {
        increment_ptr();
        return *this;
    }
    
    /* This is the postfix case */
    iterator operator++(int) {
        iterator clone(*this);
        increment_ptr();
        return clone;
    }
    
private:
    list_type * myList;
    list_node * myPtr;
};


