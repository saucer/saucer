#pragma once
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace saucer
{
    class base_promise
    {
      public:
        virtual ~base_promise() = default;
        virtual void resolve(const nlohmann::json &);
    };

    template <typename type_t> class promise;
    template <typename... args>
    static std::shared_ptr<promise<std::tuple<args...>>> all(std::shared_ptr<promise<args>>... promises); // TODO(curve): void-less

    template <typename O> struct promise_callback
    {
        using type = std::function<void(const O &)>;
    };
    template <> struct promise_callback<void>
    {
        using type = std::function<void()>;
    };

    template <typename type_t> class promise : public base_promise
    {
        template <typename... args> friend std::shared_ptr<promise<std::tuple<args...>>> all(std::shared_ptr<promise<args>>... promises);
        using callback_t = typename promise_callback<type_t>::type;
        friend class json_bridge;

      private:
        callback_t m_callback;
        nlohmann::json m_result;

      protected:
        void resolve(const nlohmann::json &result) override;

      public:
        void then(const callback_t &);
    };

    template <typename... args_t> class promise<std::tuple<args_t...>> : public base_promise
    {
        template <typename... args> friend std::shared_ptr<promise<std::tuple<args...>>> all(std::shared_ptr<promise<args>>... promises);
        using callback_t = std::function<void(const args_t &...)>;
        friend class json_bridge;

      private:
        callback_t m_callback;
        nlohmann::json m_result;

      protected:
        void finish(const args_t &...);
        void resolve(const nlohmann::json &) override;

      public:
        void then(const callback_t &);
    };
} // namespace saucer

#include "promise.inl"