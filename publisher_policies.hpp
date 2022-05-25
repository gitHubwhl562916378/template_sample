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
struct is_smart_pointer_helper : public std::false_type
{
};

template <typename T>
struct is_smart_pointer_helper<std::shared_ptr<T>> : public std::true_type
{
};

template <typename T>
struct is_smart_pointer_helper<std::unique_ptr<T>> : public std::true_type
{
};

template <typename T>
struct is_smart_pointer_helper<std::weak_ptr<T>> : public std::true_type
{
    /* data */
};

template <typename T>
struct is_smart_pointer : is_smart_pointer_helper<typename std::remove_cv<T>::type>
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
            if constexpr (m_tIsPointer) {
                sub->HandleData(d);
            } else {
                sub.HandleData(d);
            } });
    }

private:
    std::vector<T> m_subs;
    constexpr static bool m_tIsPointer = std::is_pointer<T>::value | is_smart_pointer<T>::value;
};

template <typename D, class T>
class AsyncPublisher : public Publisher<D, T>
{
public:
    AsyncPublisher()
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
    std::condition_variable m_cv;
    volatile bool m_shutDown = false;
    std::thread m_thread;
};