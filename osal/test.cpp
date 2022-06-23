#include <iostream>

template <typename... T>
auto generate(void (*fn)(T... args), T... args)
{
    return [fn, args...]()
    {
        fn(args...);
    };
}

void test(int a, int b)
{
    std::cout << __func__ << " " << a << " " << b << std::endl;
}

int main()
{

    generate(test, 12, 34)();

    return 0;
}