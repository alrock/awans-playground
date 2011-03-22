#ifndef SPMC_BOUNDED_QUEUE_H
#define SPMC_BOUNDED_QUEUE_H

#include <stdatomic.h>
#include <inttypes.h>
#include <cstring>
#include <assert.h>

template<typename T>
class spmc_bounded_queue {
public:
	spmc_bounded_queue(size_t buffer_size)
		: buffer_(new cell_t [buffer_size])
		, buffer_mask_(buffer_size - 1)
	{
		typedef char assert_nothrow [__has_nothrow_assign(T)
			|| __has_trivial_assign(T) || !__is_class(T) ? 1 : -1];
		assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));
		for (size_t i = 0; i != buffer_size; i += 1)
			buffer_[i].sequence_.store(i, std::memory_order_relaxed);
		enqueue_pos_.store(0, std::memory_order_relaxed);
		read_pos_.store(0, std::memory_order_relaxed);
		dequeue_pos_.store(0, std::memory_order_relaxed);
	}

	~spmc_bounded_queue() {
		delete [] buffer_;
	}

	bool enqueue(T const& data) {
		cell_t* cell;
		// загружаем текущую позицию для добавления в очередь
		size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
		for (;;) {
			// находим текущий элемент
			cell = &buffer_[pos & buffer_mask_];
			// загружаем статус (sequence) текущего элемента
			size_t seq = cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)pos;
			// элемент готов для записи
			if (dif == 0) {
				// пытаемся сдвинуть позицию для добавления
				/*if (enqueue_pos_.compare_exchange_weak
					(pos, pos + 1, std::memory_order_relaxed))
					break;*/
				// так как я расчитываю на одного производителя проще и быстрее просто записывать
				// в переменную enqueue_pos_, чем что-то ещё проверять. Но для случая с множеством
				// производителей, предыдущий код должен быть восстановлен.
				enqueue_pos_.store(pos+1, std::memory_order_relaxed);
				// если не получилось, то начинаем сначала
			}
			// элемент ещё не готов для записи (очередь полна или типа того)
			else if (dif < 0)
				return false;
			// нас кто-то опередил
			// перезагружаем текущий элемент и начинаем сначала
			else /* if (dif > 0) */
				pos = enqueue_pos_.load(std::memory_order_relaxed);
		}
		// в данной точке мы зарезервировали элемент для записи
		// пишем данные
		cell->data_ = data;
		// помечаем элемент как готовый для потребления
		cell->sequence_.store(pos + 1, std::memory_order_release);
		return true;
	}

	/**
	  * Метод простого чтения мне нужен для циклической обработки имеющихся заданий. Использовать
	  * его вместе с методом извлечения из очереди как минимум не безопасно!
	  */
	bool read(T &data) {
		cell_t *cell;
		// загружаем текущую позицию для извлечения из очереди
		size_t pos = read_pos_.load(std::memory_order_relaxed);
		for (;;) {
			// находим текущий элемент
			cell = &buffer_[pos & buffer_mask_];
			// загружаем статус (sequence) текущего элемента
			size_t seq = cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
			// элемент готов для извлечения
			if (dif == 0) {
				// пытаемся сдвинуть позицию для извлечения
				if (read_pos_.compare_exchange_weak
					(pos, pos + 1, std::memory_order_relaxed))
					break;
				// если не получилось, то начинаем сначала
			}
			// элемент ещё не готов для потребления (очередь пуста или типа того)
			else if (dif < 0)
				return false;
			// нас кто-то опередил
			// перезагружаем текущий элемент и начинаем сначала
			else /* if (dif > 0) */
				pos = read_pos_.load(std::memory_order_relaxed);
		}
		// в данной точке мы зарезервировали элемент для чтения
		// читаем данные
		data = cell->data_;
		// помечаем элемент как готовый для следующей записи
		// cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);
		return true;
	}

	bool dequeue(T& data)
	{
		cell_t* cell;
		// загружаем текущую позицию для извлечения из очереди
		size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
			// находим текущий элемент
			cell = &buffer_[pos & buffer_mask_];
			// загружаем статус (sequence) текущего элемента
			size_t seq = cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
			// элемент готов для извлечения
			if (dif == 0)
			{
				// пытаемся сдвинуть позицию для извлечения
				if (dequeue_pos_.compare_exchange_weak
					(pos, pos + 1, std::memory_order_relaxed))
					break;
				// если не получилось, то начинаем сначала
			}
			// элемент ещё не готов для потребления (очередь пуста или типа того)
			else if (dif < 0)
				return false;
			// нас кто-то опередил
			// перезагружаем текущий элемент и начинаем сначала
			else /* if (dif > 0) */
				pos = dequeue_pos_.load(std::memory_order_relaxed);
		}
		// в данной точке мы зарезервировали элемент для чтения
		// читаем данные
		data = cell->data_;
		// помечаем элемент как готовый для следующей записи
		cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);
		return true;
	}

	void next_read_cycle() {
		read_pos_.store(0, std::memory_order_relaxed);
	}

private:
	struct cell_t
	{
		std::atomic<size_t>     sequence_;
		T                       data_;
	};
	static size_t const         cacheline_size = 64;
	typedef char                cacheline_pad_t [cacheline_size];
	cacheline_pad_t             pad0_;
	cell_t* const               buffer_;
	size_t const                buffer_mask_;
	cacheline_pad_t             pad1_;
	std::atomic<size_t>         enqueue_pos_;
	cacheline_pad_t             pad2_;
	std::atomic<size_t>         dequeue_pos_;
	cacheline_pad_t             pad3_;
	std::atomic<size_t>         read_pos_;
	cacheline_pad_t             pad4_;
	spmc_bounded_queue(spmc_bounded_queue const&);
	void operator = (spmc_bounded_queue const&);
};

#endif // SPMC_BOUNDED_QUEUE_H
