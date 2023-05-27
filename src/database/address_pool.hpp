#pragma once

#include "mq.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <map>
#include <tuple>

class AddressPool
{
public:
    AddressPool(boost::asio::ip::network_v4);

    
    auto remove() -> boost::optional<boost::asio::ip::address>;
    auto remove_specific(boost::asio::ip::address) -> boost::optional<boost::asio::ip::address>;
    auto insert(boost::asio::ip::address) -> void;

private:
    auto contains() -> bool;

private:
    std::unordered_map<boost::asio::ip::address, std::tuple<bool>> m_hosts; // TODO replace with another container
    boost::asio::ip::network_v4 net;
};