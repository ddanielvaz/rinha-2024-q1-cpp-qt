#pragma once

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

#include <deque>
#include <memory>

using namespace std;

template <typename T>
class MutexedPool
{
    std::deque<shared_ptr<T>> store;
    QMutex mutex;
    QWaitCondition condWait;

public:
    void push(shared_ptr<T> &&el)
    {
        QMutexLocker l(&mutex);
        store.push_back(std::move(el));
        condWait.notify_one();
    }

    shared_ptr<T> pop()
    {
        QMutexLocker l(&mutex);
        while (store.empty())
        {
            condWait.wait(l.mutex());
        }
        shared_ptr<T> el = store.front();
        store.pop_front();
        return el;
    }
};