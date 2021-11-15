#pragma once
#include "promise.hpp"
#include <atomic>
#include <map>
#include <nlohmann/json.hpp>
#include <webview.hpp>

namespace saucer
{
    class json_bridge : public webview
    {
      private:
        std::atomic<std::uint64_t> m_idc{};
        std::map<std::string, std::function<void(int, const nlohmann::json &)>> m_callbacks;

      private:
        std::map<std::uint64_t, std::shared_ptr<base_promise>> m_promises;

      public:
        json_bridge();

      protected:
        void reject(int, const nlohmann::json &);
        void resolve(int, const nlohmann::json &);
        void on_message(const std::string &) override;

      public:
        template <typename func_t> void expose(const std::string &name, const func_t &function);
        template <typename rtn_t, typename... args_t> std::shared_ptr<promise<rtn_t>> call(const std::string &function_name, const args_t &...);
    };
} // namespace saucer

#include "bridge.inl"