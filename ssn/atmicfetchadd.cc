#include <iostream>
#include <atomic>

int main()
{
    std::atomic<int> x(3);

    int value = std::atomic_fetch_add(&x, 2);
    int value2 = std::atomic_fetch_add(&x, 2);

    std::cout << value << std::endl;
    std::cout << x.load() << std::endl;
    std::cout << value2 << std::endl;
}