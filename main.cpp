#include <QtCore/QCoreApplication>
#include <QDebug>

#include "mpmc_bounded_queue.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

	mpmc_bounded_queue<char> q(128);

	q.enqueue('a');
	q.enqueue('b');
	q.enqueue('c');

	char c;

	q.dequeue(c);
	qDebug() << c;
	q.dequeue(c);
	qDebug() << c;
	q.dequeue(c);
	qDebug() << c;

	return 0;
}
