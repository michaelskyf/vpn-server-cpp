#pragma once

#include <memory>
#include <database/db_guard.hpp>
#include <boost/asio.hpp>

auto handle_client(boost::asio::io_context& ctx, std::shared_ptr<AsyncStream> stream, DBGuard db_guard) -> boost::asio::awaitable<void>;