#pragma once
/*
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/optional/optional.hpp>
#include <memory>

#include <packet/packet.hpp>

using SharedPacket = std::shared_ptr<Packet const>;
using Channel = boost::asio::use_awaitable_t<boost::asio::any_io_executor>::as_default_on_t<boost::asio::experimental::concurrent_channel<void(
        boost::system::error_code, SharedPacket)>>;

class Sender
{
public:
    Sender(std::shared_ptr<Channel>&);

    auto async_send(SharedPacket packet) -> boost::asio::awaitable<void>;

private:
    std::shared_ptr<Channel> m_channel;
};

class Receiver
{
public:
    Receiver(std::shared_ptr<Channel>&);

    auto async_receive() -> boost::asio::awaitable<SharedPacket>;

private:
    std::shared_ptr<Channel> m_channel;
};*/

#include <packet/packet.hpp>

#include <boost/asio.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <iomanip>
#include <iostream>
#include <list>
namespace asio = boost::asio;
using boost::system::error_code;
using namespace std::literals;

static std::mutex mx;

static inline void trace(auto const&... args) {
    static std::atomic_uint tid_gen{};
    thread_local unsigned const tid = tid_gen++;

    std::lock_guard lk(mx); // avoid interleaved output
    std::cout << std::setw(10) << "ms tid=" << tid << " ";
}

using SharedPacket = std::shared_ptr<Packet const>;

using Task    = asio::awaitable<void>;
using Channel = asio::use_awaitable_t<asio::any_io_executor>::as_default_on_t<
    asio::experimental::concurrent_channel<void(error_code, SharedPacket)> >;

struct Subject {
    using Subscription = std::shared_ptr<Channel>;
    using Handle       = std::weak_ptr<Channel>;

    void garbage_collect() {
        subs_.remove_if(std::mem_fn(&Handle::expired));
    }

    static void for_each(std::list<Handle> subs, auto const& action) {
        for (auto& handle : subs)
            if (auto sub = handle.lock())
                action(sub);
    }

    static Task async_for_each(std::list<Handle> subs, auto action) {
#if defined(PARALLEL_GROUPS)
        using Op = decltype(action(std::declval<Subscription>()));
        std::vector<Op> ops;

        for (auto& handle : subs)
            if (auto sub = handle.lock())
                if (sub->is_open())
                    ops.push_back(action(sub));

        namespace X = asio::experimental;
        co_await X::make_parallel_group(ops) //
            .async_wait(X::wait_for_all(), asio::use_awaitable);
#else
        for (auto handle : subs)
            if (auto sub = handle.lock())
                if (sub->is_open())
                    co_await action(sub)(asio::use_awaitable);
#endif
    }

    static Task async_notify_all(std::list<Handle> subs, SharedPacket msg) {
        co_await async_for_each(std::move(subs), [=](Subscription const& channel) {
            return channel->async_send(error_code{}, msg, asio::deferred);
        });
    }

    Task async_for_each(auto action) {
        co_await async_for_each(subs_, action);
    }

    Task async_notify_all(SharedPacket msg) {
        co_await async_notify_all(subs_, msg);
    }

    void close() {
        std::list<Handle> tmp;
        tmp.swap(subs_);
        for_each(tmp, [](Subscription const& sub) { sub->cancel(); });
    }

    void add_observer(Handle hsub) {
        subs_.push_back(std::move(hsub));
    }

  private:
    std::list<Handle> subs_;
};

#include <packet/packet.hpp>

using SharedPacket = std::shared_ptr<Packet const>;

class Sender
{
public:
    Sender(std::shared_ptr<Subject>&);

    auto async_send(SharedPacket packet) -> boost::asio::awaitable<void>;

private:
    std::shared_ptr<Subject> m_channel;
};

class Receiver
{
public:
    Receiver(std::shared_ptr<Subject>&);

    auto async_receive() -> boost::asio::awaitable<SharedPacket>;

private:
    std::shared_ptr<Subject> m_channel;
};