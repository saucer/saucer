#include "smartview.hpp"

#include "webview.impl.hpp"

#include "lease.hpp"
#include "scripts.hpp"

#include <atomic>
#include <functional>

#include <lockpp/lock.hpp>

namespace saucer
{
    using lockpp::lock;

    using resolver = serializer_core::resolver;
    using function = serializer_core::function;

    struct evaluation
    {
        resolver resolve;
        std::optional<std::size_t> pending;
    };

    struct smartview_base::impl
    {
        using exposed = std::shared_ptr<function>;

      public:
        std::atomic_size_t id_counter{0};
        std::unique_ptr<serializer_core> serializer;

      public:
        lock<std::unordered_map<std::string, exposed>> functions;
        lock<std::unordered_map<std::size_t, evaluation>> evaluations;

      public:
        utils::lease<webview::impl *> lease;

      public:
        void on_dom_ready();
        status on_message(std::string_view);

      public:
        void call(std::unique_ptr<function_data>);
        void resolve(std::unique_ptr<result_data>);
    };

    smartview_base::smartview_base(webview &&base, std::unique_ptr<serializer_core> serializer)
        : webview(std::move(base)), m_impl(std::make_unique<impl>())
    {
        using namespace scripts;

        m_impl->lease      = utils::lease{webview::m_impl.get()};
        m_impl->serializer = std::move(serializer);

        inject({
            .code      = m_impl->serializer->script(),
            .run_at    = script::time::creation,
            .clearable = false,
        });

        inject({
            .code      = std::format(bridge_script, m_impl->serializer->js_serializer()),
            .run_at    = script::time::creation,
            .clearable = false,
        });

        on<event::message>({{.func = std::bind_front(&impl::on_message, m_impl.get()), .clearable = false}});
        on<event::dom_ready>({{.func = std::bind_front(&impl::on_dom_ready, m_impl.get()), .clearable = false}});
    }

    smartview_base::smartview_base(smartview_base &&) noexcept = default;

    smartview_base::~smartview_base() = default;

    status smartview_base::impl::on_message(std::string_view message)
    {
        auto parsed = serializer->parse(message);

        overload visitor = {
            [](std::monostate &)
            {
                //
                return status::unhandled;
            },
            [this](std::unique_ptr<function_data> &parsed)
            {
                call(std::move(parsed));
                return status::handled;
            },
            [this](std::unique_ptr<result_data> &parsed)
            {
                resolve(std::move(parsed));
                return status::handled;
            },
        };

        return std::visit(visitor, parsed);
    }

    void smartview_base::impl::on_dom_ready()
    {
        auto locked      = evaluations.write();
        auto *const impl = lease.value();

        for (auto it = locked->begin(); it != locked->end();)
        {
            auto &[resolve, pending] = it->second;

            if (pending && pending >= impl->last_pending)
            {
                ++it;
                continue;
            }

            resolve(err(contract_error::broken_promise));
            it = locked->erase(it);
        }
    }

    void smartview_base::impl::call(std::unique_ptr<function_data> message)
    {
        exposed function;

        if (auto locked = functions.write(); locked->contains(message->name))
        {
            function = locked->at(message->name);
        }
        else
        {
            return lease.value()->reject(message->id, std::format("\"No exposed function '{}'\"", message->name));
        }

        auto executor = serializer_core::executor{
            utils::defer(lease, [id = message->id](auto *self, auto result) { return self->resolve(id, result); }),
            utils::defer(lease, [id = message->id](auto *self, auto error) { return self->reject(id, error); }),
        };

        return (*function)(std::move(message), std::move(executor));
    }

    void smartview_base::impl::resolve(std::unique_ptr<result_data> message)
    {
        auto resolve = resolver{};

        if (auto locked = evaluations.write(); auto node = locked->extract(message->id))
        {
            resolve = std::move(node.mapped().resolve);
        }
        else
        {
            return;
        }

        resolve(std::move(message));
    }

    void smartview_base::add_function(std::string name, function &&resolve)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), std::make_shared<function>(std::move(resolve)));
    }

    void smartview_base::add_evaluation(resolver &&resolve, std::string_view code)
    {
        const auto id      = ++m_impl->id_counter;
        const auto pending = webview::execute(std::format("window.saucer.internal.resolve({}, async () => {})", id, code));

        auto locked = m_impl->evaluations.write();
        locked->emplace(id, evaluation{.resolve = std::move(resolve), .pending = pending});
    }

    void smartview_base::unexpose()
    {
        auto locked = m_impl->functions.write();
        locked->clear();
    }

    void smartview_base::unexpose(const std::string &name)
    {
        auto locked = m_impl->functions.write();
        locked->erase(name);
    }
} // namespace saucer
