#pragma once
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <condition_variable>
#include <algorithm>

template <typename T>
struct is_shared_pointer_helper : public std::false_type
{
};

template <typename T>
struct is_shared_pointer_helper<std::shared_ptr<T>> : public std::true_type
{
};

template <typename T>
struct is_weak_pointer_helper : public std::false_type
{
};

template <typename T>
struct is_weak_pointer_helper<std::weak_ptr<T>> : public std::true_type
{
    /* data */
};

template <typename T>
struct is_shared_pointer : is_shared_pointer_helper<typename std::remove_cv<T>::type>
{
};

template <typename T>
struct is_weak_pointer : is_weak_pointer_helper<typename std::remove_cv<T>::type>
{
};

template <class T>
struct TFT : public std::false_type
{
};

template <typename D, class T>
class Publisher
{
public:
    virtual ~Publisher() {}

    void AddSub(const T &t)
    {
        m_subs.push_back(t);
    }

    void Dispatch(const D &d)
    {
        std::for_each(m_subs.begin(), m_subs.end(), [&d](const T &sub)
                      {
            if constexpr (std::is_pointer<T>::value | is_shared_pointer<T>::value) {
                sub->HandleData(d);
            } else if constexpr (is_weak_pointer<T>::value) {
                auto p = sub.lock();
                if (nullptr != p) {
                    p->HandleData(d);
                }
            } else {
                static_assert(TFT<T>::value, "subdescriber should be pointer type");
            } });
    }

private:
    std::vector<T> m_subs;
};

template <typename D, class T>
class AsyncPublisher : public Publisher<D, T>
{
public:
    AsyncPublisher(const uint64_t maxQueueSize = 50)
        : m_maxQueueSize(maxQueueSize)
    {
        std::promise<void> start_ok;
        auto fut = start_ok.get_future();
        m_thread = std::thread(&AsyncPublisher::Process, this, std::ref(start_ok));
        fut.get();
    }

    ~AsyncPublisher()
    {
        m_shutDown = true;
        m_cv.notify_one();
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void Dispatch(const D &d)
    {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (m_maxQueueSize <= m_queue.size())
            {
                m_queue.pop();
            }
            m_queue.push(d);
        }
        m_cv.notify_one();
    }

private:
    void Process(std::promise<void> &pro)
    {
        pro.set_value();
        while (!m_shutDown)
        {
            D temp;
            {
                std::unique_lock<std::mutex> lock(m_mtx);
                if (m_queue.empty())
                {
                    m_cv.wait(lock, [&]
                              { return !m_queue.empty() || m_shutDown; });
                    if (m_shutDown)
                    {
                        break;
                    }
                }

                temp = m_queue.front();
                m_queue.pop();
            }

            Publisher<D, T>::Dispatch(temp);
        }
    }

private:
    std::queue<D> m_queue;
    std::mutex m_mtx;
    const uint64_t m_maxQueueSize;
    std::condition_variable m_cv;
    volatile bool m_shutDown = false;
    std::thread m_thread;
};