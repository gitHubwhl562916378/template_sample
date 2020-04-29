
#include <iostream>
#include <typeinfo>
#include "factory.h"

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

int main(int argc, char **argv)
{
    try
    {
        Factory<Shap, std::string> factory;
        std::cout << typeid(Box).name() << std::endl;
        std::cout << factory.Register(typeid(Box).name(), []() -> Shap * { return new Box; }) << std::endl;
        std::cout << factory.Unregister(typeid(Box).name()) << std::endl;
        std::cout << factory.CreateObject(typeid(Box).name()) << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    
    return 0;
}