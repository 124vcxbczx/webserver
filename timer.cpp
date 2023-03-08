//
// Created by liqm on 23-3-6.
//
#include "timer.h"

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < m_heap.size());
    assert(j >= 0 && j < m_heap.size());
    std::swap(m_heap[i], m_heap[j]);
    m_ref[m_heap[i].id] = i;
    m_ref[m_heap[j].id] = j;
}

bool HeapTimer::siftdown_(size_t index, size_t n){
    assert(index >= 0 && index < m_heap.size());
    assert(n >= 0 && n <= m_heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        //找出左右中较小的一个
        if(j + 1 < n && m_heap[j + 1] < m_heap[j]) j++;
        //如果父比左右子都小 那就不用再变了
        if(m_heap[i] < m_heap[j]) break;
        //和左右子节点中较小的那个交换
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    //比较是否向下调整了
    return i > index;
}

void HeapTimer::siftup_(size_t i){
    assert(i >= 0 && i < m_heap.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(m_heap[j] < m_heap[i]) { break; }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}
void HeapTimer::adjust(int id, int newExpires){
    assert(!m_heap.empty() && m_ref.count(id) > 0);
    m_heap[m_ref[id]].expires = Clock::now() + MS(newExpires);;
    //向下调整
    siftdown_(m_ref[id], m_heap.size());
}
void HeapTimer::add(int id, int timeout, void (*cb) (http_conn*), http_conn* user_Data){
    assert(id >= 0);
    size_t i;
    //这个id的节点不存在->新插入一个
    if(m_ref.count(id) == 0) {
        i = m_heap.size();
        //m_ref存储节点id是m_heap中的第几个的关系
        m_ref[id] = i;
        m_heap.push_back({id, Clock::now()+MS(timeout), cb, user_Data} );
        siftup_(i);
    }
    else{
        i = m_ref[id];
        m_heap[i].expires = Clock::now() + MS(timeout);
        m_heap[i].cb = cb;
        if(!siftdown_(i, m_heap.size()))
            siftup_(i);
    }
}

void HeapTimer::del_(size_t index){
    assert(!m_heap.empty() && index >= 0 && index < m_heap.size());
    size_t i = index;
    size_t n = m_heap.size() - 1;
    assert(i <= n);
    //判断它是不是最后一个
    if(i < n){
        SwapNode_(i, n);
        if(!siftdown_(i,n)){
            siftup_(i);
        }
    }
    m_ref.erase(m_heap.back().id);
    m_heap.pop_back();
}

void HeapTimer::doWork(int id){
    //删除指定节点 并触发回调函数
    if(m_heap.empty() || m_ref.count(id) == 0)
        return;
    //i是id对应的node在heap中的位置
    size_t i = m_ref[id];
    TimerNode node = m_heap[i];
    //调用这个id的回调函数 删除这个节点
    node.cb(node.user_data);
    del_(i);
}
void HeapTimer::clear(){
    m_ref.clear();
    m_heap.clear();
}

void HeapTimer::pop(){
    assert(!m_heap.empty());
    del_(0);
}
void HeapTimer::tick(){
    if(m_heap.empty()){
        std::cout<<"定时器到达：无连接关闭"<<std::endl;
        return;
    }

    while(!m_heap.empty()){
        TimerNode node = m_heap[0];
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0){
            break;
        }
        std::cout<<"定时器到达，关闭："<< node.user_data->get_msocket()<<std::endl;
        node.cb(node.user_data);
        pop();
    }
}
int HeapTimer::GetNextTick(){
    tick();
    size_t res = -1;
    if(!m_heap.empty()){
        res = std::chrono::duration_cast<MS>(m_heap.front().expires - Clock::now()).count();
        if(res < 0){
            res = 0;
        }
        return res;
    }
}


