//
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/webclient
//
// This project was made possible with the generous support of:
// The C++ Alliance (https://cppalliance.org/)
// Jetbrains (https://www.jetbrains.com/)
//
// Talk to us on Slack (https://cppalliance.org/slack/)
//
// Many thanks to Vinnie Falco for continuous mentoring and support
//

#pragma once

#include "async/detail/future_invoker_base.hpp"

#include <asio/detail/bind_handler.hpp>
#include <asio/post.hpp>
#include <cassert>
#include <optional>
#include <utility>

namespace async {
namespace detail {

template <class T, class Handler>
struct future_invoker : detail::future_invoker_base<T> {
    explicit future_invoker(Handler&& handler) : handler_(std::move(handler)) {}

    virtual void notify_value(future_result_type<T>&& value) override {
        // FIXME
        // net::post(boost::beast::bind_front_handler(std::move(*handler_), std::move(value)));
        asio::post(asio::detail::bind_handler(std::move(*handler_), std::move(value)));
        handler_.reset();
    }

    std::optional<Handler> handler_;
};

} // namespace detail
} // namespace async
