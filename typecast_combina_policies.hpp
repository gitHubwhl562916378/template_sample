#pragma once
#include <string>
#include <exception>

template <class T>
class NoChecking
{
public:
    static void Check(const T &) {}
};

template <class T>
class EnforceNotNUll
{
public:
    class NotNUllException : public std::exception
    {
    public:
        const char *what() const override { return "nullptr checked"; }
    };

    static void Check(const T &p)
    {
        if (!p)
        {
            throw NotNUllException();
        }
    }
};

//将任意类型SmartPtr<CheckingPolicO>类型为O,拷贝构造Widet<CheckingPolicy>类型为T
//需要 CheckingPolicy<T>有使用SmartPtr<O,CheckingPolicyO>为参数的构造函数
template <class T, template <class> class CheckingPolicy>
class SmartPtr : public CheckingPolicy<T>
{
public:
    template <class O, template <class> class CheckingPolicyO>
    SmartPtr(const SmartPtr<O, CheckingPolicyO> &o) : m_width(o.m_width), m_height(o.m_height), CheckingPolicy<T>(o) {}

    int m_width;
    int m_height;
};

//多种policy组合
template <
    class T,
    template <class> class CheckingPolicy,
    template <class> class ThreadingModel>
class SafePtr : public CheckingPolicy<T>, public ThreadingModel<T>
{
public:
    T *operator->()
    {
        decltype(ThreadingModel<SafePtr>::Lock) guard(*this);
        CheckingPolicy<T>::Check(pointee_);
        return pointee_;
    }

private:
    T *pointee_;
};