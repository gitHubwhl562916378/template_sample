#pragma once
#include <map>
#include <functional>

template <class To, class From>
struct DynamicCaster
{
    static To Cast(From o)
    {
        return dynamic_cast<To>(o);
    }
};

template <class To, class From>
struct StaticCaster
{
    static To Cast(From o)
    {
        return static_cast<To>(o);
    }
};

template <
    class BaseLhs,
    class BaseRhs,
    template <class, class> class CasterPolicy = StaticCaster,
    typename ResultType = void,
    typename Functor = std::function<ResultType(BaseLhs &, BaseRhs &)>>
class DoubleFunctorDispatcher
{
public:
    /**
     * @brief non-type template-parameter(callBack), 无类型模板参数，可以是数字，指针，运行时指定数字。函数指针需要可以外部链接
     * 执行时通过hashcode已经找到类型，不需要dynamic_cast再判断一次安全转换，使用static_cast即可，快得多。对于有菱形继承或虚继承的情况则
     * 必须使用dynamic_cast
     * @param symmetric 是否具有参数对称性，需要ConcreteLhs,ConcreteRhs继承同一基类才能实现，交换时TranslateR(BaseRhs&,BaseLhs&)
     * 与DoubleFunctorDispatcher具体模板参数Functor参数相反无法插入map
     */
    template <
        class ConcreteLhs,
        class ConcreteRhs,
        ResultType (*callBack)(ConcreteLhs &, ConcreteRhs &)>
    void Add(bool symmetric = false) //针对全局可调用的函数，如静态函数，全局函数的添加
    {
        struct Local
        {
            static ResultType Translate(BaseLhs &lhs, BaseRhs &rhs)
            {
                return callBack(CasterPolicy<ConcreteLhs &, BaseLhs &>::Cast(lhs), CasterPolicy<ConcreteRhs &, BaseRhs &>::Cast(rhs));
            }

            static ResultType TranslateR(BaseRhs &rhs, BaseLhs &lhs)
            {
                return Translate(lhs, rhs);
            }
        };

        auto h1 = typeid(ConcreteLhs).hash_code();
        auto h2 = typeid(ConcreteRhs).hash_code();
        m_functors[h1 ^ (h2 << 1)] = &Local::Translate;
        if (symmetric)
        {
            if constexpr (std::is_same_v<BaseLhs, BaseRhs>)
            {
                m_functors[h2 ^ (h1 << 1)] = &Local::TranslateR;
            }
        }
    }

    /**
     * @brief 添加函数对象
     * @param func 参数顺序需要与DoubleFunctorDispatcher具体的Functor一致;如果BaseLhs与BaseRhs是一个类型，且需要BaseLhs与BaseRhs参数对称性，Add另一种参数顺序的函数对象即可
     */
    template <
        class ConcreteLhs,
        class ConcreteRhs,
        typename ImplFunctor = std::function<ResultType(ConcreteLhs &, ConcreteRhs &)>>
    void Add(ImplFunctor func)
    {
        auto h1 = typeid(ConcreteLhs).hash_code();
        auto h2 = typeid(ConcreteRhs).hash_code();
        auto adapterF = [func](BaseLhs &l, BaseRhs &r)
        {
            return func(CasterPolicy<ConcreteLhs &, BaseLhs &>::Cast(l), CasterPolicy<ConcreteRhs &, BaseRhs &>::Cast(r));
        };
        m_functors[h1 ^ (h2 << 1)] = adapterF;
    }

    template <
        class ConcreteLhs,
        class ConcreteRhs>
    ResultType Excute(ConcreteLhs &l, ConcreteRhs &r)
    {
        auto h1 = typeid(ConcreteLhs).hash_code();
        auto h2 = typeid(ConcreteRhs).hash_code();
        auto iter = m_functors.find(h1 ^ (h2 << 1));
        if (iter == m_functors.end())
        {
            throw std::runtime_error("Functor not find");
        }

        return iter->second(l, r);
    }

    std::map<std::size_t, Functor> m_functors;
};