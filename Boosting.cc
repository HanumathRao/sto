#include "Boosting.hh"
#include "Boosting_hashtable.hh"

boosting_threadinfo boosting_threads[BOOSTING_MAX_THREADS];
__thread int boosting_threadid;

void boosting_txStartHook() {
  POST_COMMIT(boosting_releaseLocksCallback, NULL, NULL, NULL);
  ON_ABORT(boosting_releaseLocksCallback, NULL, NULL, NULL);
}

void boosting_setThreadID(int threadid) {
  boosting_threadid = threadid;
}

void boosting_releaseLocksCallback(void*, void*, void*) {
  for (auto *lock : _thread().lockset) {
    lock->unlock();
  }
  for (auto *rwlockp : _thread().rwlockset) {
    auto& rwlock = *rwlockp;
    // this certainly doesn't seem very safe but I think it might be
    if (rwlock.isUpgraded()) {
      rwlock.readUnlock();
      rwlock.writeUnlock();
    } else if (rwlock.isWriteLocked()) {
      rwlock.writeUnlock();
    } else {
      rwlock.readUnlock();
    }
  }

  _thread().lockset.unsafe_clear();
  _thread().rwlockset.unsafe_clear();
}
