#pragma once
#include <coroutine>
#include <concepts>
#include <stdexcept>
#include <type_traits>
#include <functional>
#include <cassert>
#include <memory>
#include <utility>

namespace asyncxx {
    template <typename T> requires (!std::is_void_v<T>)
    struct Generator;
}

/**
 * @brief 生成器. 所有方法都是在实现协程语义, 不应该直接调用.
 *        将该类型作为协程返回值即可使用, 通过 co_yield 产生值;
 *        通过 range-based for-loop 遍历生成器对象即可消费值.
 * @tparam T  生成器产生的值的类型.  T 和 T& 相同, 均表示引用.
 */
template<typename T> requires (!std::is_void_v<T>)
struct asyncxx::Generator {
    using Ref = std::conditional_t<
        std::is_reference_v<T>,
        T&&, T&
    >;
    struct promise_type {
        auto get_return_object() {
            return new auto{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        auto initial_suspend() -> std::suspend_always { return {}; }
        auto yield_value(std::convertible_to<Ref> auto&& value)
        -> std::suspend_always {
            this->yielder = [&] -> Ref {
                return std::forward<decltype(value)>(value);
            };
            return {};
        }
        void return_void() {}
        void unhandled_exception() { throw; }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }

        std::function<Ref()> yielder;
    };

    struct default_delete {
        void operator()(std::coroutine_handle<promise_type> *const handle) const {
            handle->destroy();
            delete handle;
        }
    };

    struct iterator {
        auto&& operator*() const {
            return this->cor->promise().yielder();
        }
        auto& operator++() {
            assert(*this != std::default_sentinel);
            this->cor->resume();
            return *this;
        }
        friend bool operator==(const iterator& iter, std::default_sentinel_t) {
            return iter.cor->done();
        }

        iterator(
            std::unique_ptr<
                std::coroutine_handle<promise_type>, default_delete
            >&& handle
        ): cor{std::move(handle)} {}
        iterator(iterator&& other): cor{std::move(other.cor)} {}
        iterator& operator=(iterator&& other) {
            this->cor = std::move(other.cor);
            return *this;
        }
      private:
        std::unique_ptr<
            std::coroutine_handle<promise_type>, default_delete
        > cor;
    };

    auto begin() -> iterator {
        this->cor->resume();
        return std::move(this->cor);
    }
    auto end() const -> std::default_sentinel_t { return {}; }

    Generator(std::coroutine_handle<promise_type> *const handle)
    : cor{handle} {}
  private:
    std::unique_ptr<
        std::coroutine_handle<promise_type>, default_delete
    > cor;
};
