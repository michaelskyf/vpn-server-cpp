#pragma once

#include "mq/mq.hpp"
#include <common.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <memory>
#include <database/db_guard.hpp>
#include <boost/asio.hpp>
#include <stream/stream.hpp>

class ConnectionHandler
{
public:
    static auto incoming(std::shared_ptr<AsyncStream> stream, DBGuard db_guard) -> Task<void>;
    static auto outgoing(std::shared_ptr<AsyncStream> stream, Receiver mq_rx) -> Task<void>;
};