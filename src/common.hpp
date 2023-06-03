#pragma once

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/outcome/result.hpp>

// Used commonly throughout the project

namespace asio = boost::asio;

template<typename T>
using Task = boost::asio::awaitable<T>;

template<typename T>
using Option = boost::optional<T>;

template<typename T>
using Result = boost::outcome_v2::result<T>;