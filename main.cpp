
#include <iostream>
#include <memory>
#include <typeinfo>
#include "factory.h"
#include "publisher_policies.hpp"
#include "double_functor_dispatcher.hpp"

class Shap
{
public:
    virtual ~Shap() {}
    virtual void Rote(const float) = 0;
};

class Box : public Shap
{
public:
    void Rote(const float angle)
    {
        std::cout << angle << std::endl;
    }
};

class Rectangle : public Shap
{
public:
    void Rote(const float angle)
    {
        std::cout << angle << std::endl;
    }
};

void DrawUnion(Box &b, Rectangle &r)
{
    std::cout << "DrawUnion" << std::endl;
}

class Tool
{
public:
    virtual ~Tool() {}
};

class Compass : public Tool
{
};

void DrawCompass(Box &b, Compass &t)
{
    std::cout << "DrawCompass" << std::endl;
}

class Command
{
public:
    void HandleData(const int d)
    {
        std::cout << "Command HandleData " << d << std::endl;
    }
};
template <template <typename, class> class DispatcherPolicy = Publisher>
class TaskPublisher final : public DispatcherPolicy<int, std::shared_ptr<Command>>
{
};

//只有当T不是一个智能指针类型的时候，类型D继承于T;不是的话类型不存在，编译报错
// std::enable_if<bool con, T = void>::type 当con为true的时候，表达式是一个类型，不为true的时候类型T未定义
template <class T>
class D : public std::enable_if<!is_shared_pointer<T>::value, T>::type
{
};

int main(int argc, char **argv)
{
    try
    {
        Factory<std::shared_ptr<Shap>, std::string> factory;
        std::cout << typeid(Box).name() << std::endl;
        std::cout << factory.Register(typeid(Box).name(), []() -> std::shared_ptr<Shap>
                                      { return std::make_shared<Box>(); })
                  << std::endl;
        std::cout << factory.Unregister(typeid(Box).name()) << std::endl;
        std::cout << factory.CreateObject(typeid(Box).name()) << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }

    //直接使用policy,通过类型创建
    std::shared_ptr<Publisher<int, std::shared_ptr<Command>>> dis = std::make_shared<AsyncPublisher<int, std::shared_ptr<Command>>>();
    dis->AddSub(std::make_shared<Command>());
    dis->Dispatch(3);

    dis = std::make_shared<Publisher<int, std::shared_ptr<Command>>>();
    dis->AddSub(std::make_shared<Command>());
    dis->Dispatch(3);

    //通过再次定义具体类型，传入需要的policy名称来创建，类似于策略模式
    auto asyncTaskPub = std::make_shared<TaskPublisher<AsyncPublisher>>();
    asyncTaskPub->AddSub(std::make_shared<Command>());
    asyncTaskPub->Dispatch(3);

#if 0 // BaseLhs, 与BaseRhs不相同时
    DoubleFunctorDispatcher<Shap, Tool, StaticCaster> doubleFnDisp;
    doubleFnDisp.Add<Box, Compass, DrawCompass>(true);
    Box bb;
    Compass cmss;
    doubleFnDisp.Excute(bb, cmss);
    //二次添加覆盖
    doubleFnDisp.Add<Box, Compass>([](Box &b, Compass &c)
                                   { std::cout << "Compass in lamuda" << std::endl; });
    doubleFnDisp.Excute(bb, cmss);

#else // BaseLhs, 与BaseRhs不相同时
    DoubleFunctorDispatcher<Shap, Shap, StaticCaster> doubleFnDisp;
    doubleFnDisp.Add<Box, Rectangle, DrawUnion>(true);
    Box bb;
    Rectangle r;
    doubleFnDisp.Excute(r, bb);
    doubleFnDisp.Excute(bb, r); // BaseLhs与BaseRhs相同，可以反向参数调用，Add(true)已将全局函数中返回调用添加

    doubleFnDisp.Add<Box, Rectangle>([](Box &b, Rectangle &r)
                                     { std::cout << "Rectangle in lamuda" << std::endl; });
    doubleFnDisp.Add<Rectangle, Box>([](Rectangle &r, Box &b)
                                     { std::cout << "Rectangle in lamuda" << std::endl; });
    doubleFnDisp.Excute(r, bb);
    doubleFnDisp.Excute(bb, r); // BaseLhs与BaseRhs相同，可以反向参数调用，通过添加对称的函数对象
#endif

    //等待异步操作完成，如AsyncDispatcher
    ::getchar();
    return 0;
}