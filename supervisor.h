#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <QThread>
#include <stdatomic.h>
#include <cstring>
#include <vector>
#include <QSemaphore>
#include <QAtomicInt>

#include "thread.h"
#include "spmc_bounded_queue.h"

struct control_info {
	QSemaphore s;
	std::atomic<int> flag;

	inline void thread_step_over() {
		s.release(1);
	}
	inline bool next_step_allowed() {
		return flag.fetch_sub(1, std::memory_order_relaxed) > 0;
	}
};

template <typename Task>
class supervisor: public QThread {
public:
	supervisor() {
		info_.flag.store(0, std::memory_order_relaxed);
	}

	template <template <typename T> Container>
	void set_tasks(const Container<Task*> &c) {
		tasks_.clear();
		tasks_.reserve(c.size());
		Container<Task*>::const_iterator i = c.begin(), end = c.end();
		for (; i != end; ++i) {
			tasks_.push_back(*i);
		}
	}


protected:
	void run();
private:
	void initialize() {
		info_.flag.store(0, std::memory_order_relaxed);

		ideal_thread_count = QThread::idealThreadCount();
		if (ideal_thread_count <= 0)
			ideal_thread_count = 1;
		if (ideal_thread_count > 1)
			--ideal_thread_count;
		threads_.clear();
		threads_.reserve(ideal_thread_count+2);
	}

	void initialize_threads() {
		int count = (tasks_.size()+tasks_.size()%optimal_tasks_per_thread)/optimal_tasks_per_thread;
		if (count < 1) count = 1;
		if (count > ideal_thread_count)
			count = ideal_thread_count;
		int t_count = (tasks_.size()+tasks_.size()%count)/count;
		for (int i = 0; i < count; ++i) {
			thread_info t;
			t.t = new thread<Task>(128);
			t.q = t.t->get_queue();

			for (int j = 0; j < t_count && (i*t_count+j) < tasks_.size(); ++j) {
				t.q->enqueue(tasks_[i*t_count+j]);
			}
			threads_.push_back(t);
		}
	}

private:
	struct thread_info {
		thread<Task> *t;
		spmc_bounded_queue<Task*> *q;
	};

	control_info info_;
	const int optimal_tasks_per_thread = 20;
	int ideal_thread_count;

	std::vector<thread_info> threads_;
	std::vector<Task*> tasks_;
};

template <typename Task>
void supervisor::run() {

}

#endif // SUPERVISOR_H
