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

#include "async/detail/future_wait_op.hpp"

#include <asio/async_result.hpp>
#include <system_error>

namespace async {

template <class T>
struct promise;

template <class T>
struct future {
    using impl_class = detail::future_state_impl<T>;
    using impl_type = std::shared_ptr<impl_class>;

    using result_type = future_result_type<T>;

    template <class CompletionHandler>
    auto async_wait(CompletionHandler&& token)
        -> ASIO_INITFN_RESULT_TYPE(CompletionHandler, result_type);

  private:
    friend promise<T>;

    future(impl_type impl) : impl_(std::move(impl)) {}

    impl_type impl_;
};

template <class T>
struct promise {
    using impl_class = detail::future_state_impl<T>;
    using impl_type = std::shared_ptr<impl_class>;

    promise();

    promise(promise&& other) noexcept;

    promise& operator=(promise&& other) noexcept;

    ~promise() noexcept;

    void set_value(T val);

    void set_error(std::error_code ec);

    void set_exception(std::exception_ptr ep);

    future<T> get_future();

  private:
    void destroy();

    impl_type impl_;
    impl_type future_impl_;
};

} // namespace async

#include "future.ipp"
#include "promise.ipp"
