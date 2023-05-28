#include "database.hpp"
#include "mq.hpp"
#include <boost/none.hpp>

Database::Database(boost::asio::io_context& ctx, boost::asio::ip::network_v4 net)
    : m_ctx{ctx}, m_address_pool{net}
{

}

auto Database::register_without_ip() -> boost::optional<ip_mqrx_pair>
{
    auto address_option = m_address_pool.remove();

    if(address_option.has_value() == false)
    {
        return boost::none;
    }

    auto address = address_option.value();
    auto mq = std::make_shared<Channel>(m_ctx, 64); // 64 packets can be at the same time in the queue
    auto sender = Sender(mq);
    auto receiver = Receiver(mq);

    m_map.emplace(address, sender);

    return ip_mqrx_pair(address, receiver);
}

auto Database::register_with_ip(boost::asio::ip::address address) -> boost::optional<ip_mqrx_pair>
{
    if(m_address_pool.remove_specific(address).has_value() == false)
    {
        return boost::none;
    }

    auto mq = std::make_shared<Channel>(m_ctx, 64); // 64 packets can be at the same time in the queue
    auto sender = Sender(mq);
    auto receiver = Receiver(mq);

    m_map.emplace(address, sender);

    return ip_mqrx_pair(address, receiver);
}

auto Database::get(boost::asio::ip::address address) -> boost::optional<Sender>
{
    auto result = m_map.find(address);
    if(result == m_map.end())
    {
        return boost::none;
    }
    else
    {
        return result->second;
    }
}

auto Database::unregister(boost::asio::ip::address address) -> void
{
    m_map.erase(address);
    m_address_pool.insert(address);
}