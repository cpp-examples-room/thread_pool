#include "thread_pool.hpp"
#include <chrono>
#include <sstream>
#include <iostream>

void background(std::string const& message) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(3s);
    std::stringstream str_s;
    str_s << message << std::endl;
    std::cout << str_s.str();
}

int main() {
    tds::thread_pool thread_pool {3};

    auto task_1 = thread_pool.submit(background, "1");
    auto task_2 = thread_pool.submit(background, "2");
    auto task_3 = thread_pool.submit(background, "3");
    auto task_4 = thread_pool.submit(background, "4");
    auto task_5 = thread_pool.submit(background, "5");

    task_1.wait();
    task_2.wait();
    task_3.wait();
    task_4.wait();
    task_5.wait();
}
