//
// Created by liqm on 23-3-6.
//

#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "http_conn.h"

using Clock = std::chrono::high_resolution_clock;
using TimeStamp = Clock::time_point;
using MS = std::chrono::milliseconds;

struct TimerNode{
    int id;
    TimeStamp expires;
    void (*cb) (http_conn*);
    http_conn* user_data;
    bool operator<(const TimerNode& t){
        return expires < t.expires;
    }
};

class HeapTimer{

public:
    HeapTimer() { m_heap.reserve(64); }

    ~HeapTimer() { clear(); }
    void adjust(int id, int newExpires);
    void add(int id, int timeOut, void (*cb) (http_conn*), http_conn* user_Data);
    void doWork(int id);
    void clear();
    void tick();
    void pop();
    int GetNextTick();
private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t index, size_t n);
    void SwapNode_(size_t i, size_t j);
private:
    std::vector<TimerNode> m_heap;
    std::unordered_map<int, size_t> m_ref;
};
#endif //WEBSERVER_TIMER_H
