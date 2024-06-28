
#ifndef _CC_LIB_WORK_QUEUE_H
#define _CC_LIB_WORK_QUEUE_H

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

#include "threadutil.h"
#include "base/logging.h"

// Thread-safe work queues. Accumulate work items and fetch them in
// fixed-size batches.

template<class Item>
struct BatchedWorkQueue {
  const int batch_size = 0;
  explicit BatchedWorkQueue(int batch_size) : batch_size(batch_size) {
    CHECK(batch_size > 0);
    // Set up invariant.
    queue.push_back(std::vector<Item>{});
    queue.back().reserve(batch_size);
  }

  // Consumers of the work queue call this in a loop. If the result
  // is nullopt, then the queue is done. The final work item can be
  // smaller than the batch size, but not empty.
  std::optional<std::vector<Item>> WaitGet() {
    std::vector<Item> batch;
    {
      std::unique_lock ml(mutex);
      cond.wait(ml, [this] {
          // Either the queue is empty (and we're totally done)
          // or there's something in the queue.
          if (done) return true;
          CHECK(!queue.empty());
          return queue.front().size() == batch_size;
        });
      if (done && queue.empty()) return std::nullopt;
      // Now holding lock with a full batch (or the last batch).
      // Take ownership.
      batch = std::move(queue.front());
      size -= batch.size();
      // It's the responsibility of the functions that insert into the
      // queue to maintain the presence of an incomplete vector. So we
      // can just remove the full one.
      queue.pop_front();
    }
    cond.notify_all();

    return {batch};
  }

  bool IsDone() {
    std::unique_lock ml(mutex);
    return done;
  }


  int64_t Size() {
    std::unique_lock ml(mutex);
    return size;
  }

  // PERF: Versions that take rvalue reference.

  void WaitAdd(const Item &item) {
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      CHECK(!queue.empty() && queue.back().size() < batch_size);
      queue.back().push_back(item);
      size++;
      MaybeFinishBatch();
    }
    cond.notify_all();
  }

  void WaitAdd(Item &&item) {
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      CHECK(!queue.empty() && queue.back().size() < batch_size);
      queue.back().emplace_back(item);
      size++;
      MaybeFinishBatch();
    }
    cond.notify_all();
  }

  void WaitAddVec(const std::vector<Item> &items) {
    if (items.empty()) return;
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      // PERF: This can be tighter, but the main thing is to avoid
      // repeatedly taking the lock.
      size += items.size();
      for (const Item &item : items) {
        CHECK(!queue.empty() && queue.back().size() < batch_size);
        queue.back().push_back(item);
        MaybeFinishBatch();
      }
    }
    cond.notify_all();
  }

  void MarkDone() {
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      done = true;
    }
    cond.notify_all();
  }

  // Wait until the number of pending batches is fewer than the
  // argument. An incomplete batch counts, including an empty one, so
  // there is always at least one pending batch. This can be used to
  // efficiently throttle threads that add to the queue. Queue may
  // not be done.
  void WaitUntilFewer(int num_batches) {
    CHECK(num_batches > 0) << "This would never return.";
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      cond.wait(ml, [this, num_batches] {
          return queue.size() < num_batches;
        });
    }
    // State hasn't changed, so no need to notify others.
  }

 private:
  // Must hold lock.
  inline void MaybeFinishBatch() {
    if (queue.back().size() == batch_size) {
      // Finished batch, so add new empty batch.
      queue.push_back(std::vector<Item>());
      queue.back().reserve(batch_size);
    }
  }

  std::mutex mutex;
  std::condition_variable cond;
  // Add at the end. This always consists of a series (maybe zero)
  // of full vectors and an incomplete vector (maybe empty) at the
  // end. (Unless "done", in which case it can be empty.)
  std::deque<std::vector<Item>> queue;
  // Size is the number of items, not batches.
  int64_t size = 0;
  bool done = false;
};

// TODO: To threadutil?
// Here, a serial queue. This is intended for larger items (perhaps
// pre-batched work).
template<class Item>
struct WorkQueue {
  // TODO: Could support max queue size pretty easily.
  WorkQueue() {
  }

  // Consumers of the work queue call this in a loop. If nullopt,
  // then the queue is done.
  std::optional<Item> WaitGet() {
    Item item;
    {
      std::unique_lock ml(mutex);
      cond.wait(ml, [this] {
          if (done) return true;
          return !queue.empty();
        });

      if (done && queue.empty()) return std::nullopt;

      item = std::move(queue.front());
      queue.pop_front();
    }
    cond.notify_all();

    return {item};
  }

  int64_t Size() {
    std::unique_lock ml(mutex);
    return queue.size();
  }

  void WaitAdd(const Item &item) {
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      queue.push_back(item);
    }
    cond.notify_all();
  }

  void WaitAdd(Item &&item) {
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      queue.emplace_back(item);
    }
    cond.notify_all();
  }

  void MarkDone() {
    {
      std::unique_lock ml(mutex);
      CHECK(!done);
      done = true;
    }
    cond.notify_all();
  }

private:
  std::mutex mutex;
  std::condition_variable cond;
  // The items. Can be empty.
  std::deque<Item> queue;
  bool done = false;
};

#endif
