// https://cpprefjp.github.io/reference/coroutine/noop_coroutine.html

#include <iostream>
#include <utility>

#define __cpp_impl_coroutine 1
# include <coroutine>
#undef  __cpp_impl_coroutine
namespace std::inline experimental
{
    using namespace std;
}

inline namespace aux
{
    struct task {
        struct promise_type {
            std::coroutine_handle<> next;
            auto get_return_object() { return task{*this}; }
            auto initial_suspend() { return std::suspend_always{}; }
            auto final_suspend() noexcept { return std::suspend_always{}; }
            auto yield_value(bool cont) {
                struct awaiter {
                    std::coroutine_handle<> next;
                    bool await_ready() { return false; }
                    auto await_suspend(std::coroutine_handle<>) { return this->next; }
                    void await_resume() {}
                };
                return awaiter{ cont ? this->next : std::noop_coroutine() };
            }
            void return_void() { }
            void unhandled_exception() { std::terminate(); }
        };

        ~task() {
            if (this->continuation) this->continuation.destroy();
        }

        task(task const&) = delete;
        task(task&& rhs)
            : continuation(std::exchange(rhs.continuation, nullptr)) { }
        void set_next(task& t) {
            this->continuation.promise().next = t.continuation;
        }
        void start() {
            if (!this->continuation.done()) this->continuation.resume();
        }

    private:
        explicit task(promise_type& p)
            : continuation(std::coroutine_handle<promise_type>::from_promise(p))
        {
        }
        std::coroutine_handle<promise_type> continuation;
    };
}

task coro(int id) {
    int n = id * 16;
    for (;;) {
        std::cout << "coro#" << id << ' ' << n << std::endl;
        co_yield (0 < n);
        n /= 2;
    }
}

int main() {
    auto c1 = coro(1);
    auto c2 = coro(2);
    auto c3 = coro(3);
    c1.set_next(c2);
    c2.set_next(c3);
    c3.set_next(c1);

    c1.start();
    return 0;
}
