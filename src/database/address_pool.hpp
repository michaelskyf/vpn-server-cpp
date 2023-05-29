#pragma once

#include <mq/mq.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/none.hpp>
#include <map>
#include <tuple>
#include <iostream>

class AddressPool
{
public:
    AddressPool(boost::asio::ip::network_v4 net)
        : m_hosts{}, m_net{net}
    {
        for(auto h : net.hosts())
        {
            m_hosts.emplace(h, false);
        }
    }
    
    auto remove() -> boost::optional<boost::asio::ip::address>
    {
        for(auto it = m_hosts.begin(); it != m_hosts.end(); it++)
        {
            auto ip = it->first;
            m_hosts.erase(it);

            return ip;
        }

        return boost::none;
    }

    auto remove_specific(boost::asio::ip::address address) -> boost::optional<boost::asio::ip::address>
    {
        auto it = m_hosts.find(address);
        if(it == m_hosts.end())
        {
            return boost::none;
        }

        auto ip = it->first;
        m_hosts.erase(it);

        return ip;
    }

    auto insert(boost::asio::ip::address address) -> void
    {
        if(m_net.hosts().find(address.to_v4()) == m_net.hosts().end())
        {
            return;
        }

        m_hosts.emplace(address, false);
    }

private:

private:
    std::unordered_map<boost::asio::ip::address, bool> m_hosts; // TODO replace with another container (bool is never used)
    boost::asio::ip::network_v4 m_net;
};