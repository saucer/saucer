#pragma once
#include "promise.hpp"
#include <atomic>
#include <lock.hpp>
#include <map>
#include <nlohmann/json.hpp>
#include <webview.hpp>

namespace saucer
{
    namespace bridge
    {
        class json : public webview
        {
            using callback_t = std::function<void(int, const nlohmann::json &)>;

          private:
            std::atomic<std::uint64_t> m_idc{};
            lockpp::lock<std::map<std::string, std::pair<callback_t, bool>>> m_callbacks;

          private:
            lockpp::lock<std::map<std::uint64_t, std::shared_ptr<base_promise>>, std::recursive_mutex> m_promises;

          public:
            json();

          protected:
            void reject(int, const nlohmann::json &);
            void resolve(int, const nlohmann::json &);
            void on_message(const std::string &) override;

          public:
            template <typename func_t> void expose_async(const std::string &name, const func_t &function);
            template <typename func_t> void expose(const std::string &name, const func_t &function, bool async = false);
            template <typename rtn_t, typename... args_t> std::shared_ptr<promise<rtn_t>> call(const std::string &function_name, const args_t &...);
        };

    } // namespace bridge
} // namespace saucer

#include "bridge.inl"