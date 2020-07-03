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

#include "async/detail/future_invoker.hpp"
#include "async/detail/future_state_impl.hpp"
#include "async/future_types.hpp"

#include <memory>

namespace async {
namespace detail {

template <class T>
struct future_wait_op {
    template <class Self>
    void operator()(Self& self) {
        // initial operation
        shared_state_->set_invoker(std::make_unique<future_invoker<T, Self>>(std::move(self)));
    }

    template <class Self>
    void operator()(Self& self, future_result_type<T>&& value) {
        self.complete(std::move(value));
    }

    std::shared_ptr<future_state_impl<T>> shared_state_;
};

} // namespace detail
} // namespace async
