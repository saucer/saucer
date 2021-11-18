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
    template <typename... args_t> [[nodiscard]] static auto all(std::shared_ptr<promise<args_t>>...);

    namespace internal
    {
        template <typename type_t> struct promise_callback
        {
            using type = std::function<void(const type_t &)>;
        };
        template <> struct promise_callback<void>
        {
            using type = std::function<void()>;
        };

        template <typename... args_t> struct grouped_promise
        {
            template <typename... _args_t> friend auto saucer::all(std::shared_ptr<promise<_args_t>>...);
            using callback_t = std::function<void(const args_t &...)>;

          private:
            callback_t m_callback;

          protected:
            void resolve(args_t...);

          public:
            void then(const callback_t &);
        };

        template <typename... args_t> struct grouped_promise<std::tuple<args_t...>> : public grouped_promise<args_t...>
        {
        };
    } // namespace internal

    template <typename type_t> class promise : public base_promise
    {
        template <typename... args_t> friend auto all(std::shared_ptr<promise<args_t>>...);
        using callback_t = typename internal::promise_callback<type_t>::type;
        friend class json_bridge;
        using type = type_t;

      private:
        callback_t m_callback;
        nlohmann::json m_result;

      protected:
        void resolve(const nlohmann::json &result) override;

      public:
        void then(const callback_t &);
    };
} // namespace saucer

#include "promise.inl"