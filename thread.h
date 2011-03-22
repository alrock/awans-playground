#ifndef THREAD_H
#define THREAD_H

#include <QThread>
#include <cstring>
#include <vector>

#include "spmc_bounded_queue.h"

template <typename Task>
class thread: public QThread {
	typedef spmc_bounded_queue<Task*> task_queue;
	typedef std::vector<task_queue*>  additional_queues;
public:
	thread(size_t buffer_size): q(buffer_size) {}

	void set_additional_queues(const additional_queues &v) {
		aq = v;
	}

	task_queue* get_queue() { return &q; }
protected:
	void run();
private:
	task_queue q;
	additional_queues aq;
};

template <typename Task>
void thread::run() {

}

#endif // THREAD_H
