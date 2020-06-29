#pragma once

#include "compat/asio.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

/// Adapt the test stream class from beast
/// https://github.com/boostorg/beast/blob/boost-1.73.0/include/boost/beast/_experimental/test/stream.hpp
namespace test {

struct address {
    address(const char* addr) : addr_{addr} {}

    auto to_string() const noexcept -> const std::string& { return addr_; }
    const std::string addr_;
};

struct remote_endpoint {
    remote_endpoint(const char* addr, unsigned short port) : addr_{addr}, port_{port} {}

    auto address() const noexcept -> const ::test::address& { return addr_; }

    auto port() const noexcept -> unsigned short { return port_; }

    const ::test::address addr_;
    unsigned short port_;
};

struct read_op_base {
    virtual ~read_op_base() = default;
    virtual void operator()() = 0;
};

enum class status { ok, eof, reset };

struct state {
    std::mutex m;
    asio::streambuf b;
    std::unique_ptr<read_op_base> op;
    asio::io_context& ioc;
    status code = status::ok;
    std::size_t nread = 0;
    std::size_t read_max = (std::numeric_limits<std::size_t>::max)();

    explicit state(asio::io_context& ioc_) : ioc(ioc_) {}
};

template <class Handler, class Buffers>
class read_op : public read_op_base {

    class lambda {
        state& s_;
        Buffers b_;
        Handler h_;
        asio::executor_work_guard<asio::io_context::executor_type> work_;

      public:
        lambda(lambda&&) = default;
        lambda(lambda const&) = default;

        template <class DeducedHandler>
        lambda(state& s, Buffers const& b, DeducedHandler&& h)
            : s_(s), b_(b), h_(std::forward<DeducedHandler>(h)), work_(s_.ioc.get_executor()) {}

        void post() {
            asio::post(s_.ioc.get_executor(), std::move(*this));
            work_.reset();
        }

        void operator()() {
            using asio::buffer_copy;
            using asio::buffer_size;
            std::unique_lock<std::mutex> lock{s_.m};
            assert(!s_.op);
            if (s_.b.size() > 0) {
                auto const bytes_transferred = buffer_copy(b_, s_.b.data(), s_.read_max);
                s_.b.consume(bytes_transferred);
                auto& s = s_;
                Handler h{std::move(h_)};
                lock.unlock();
                ++s.nread;
                asio::post(
                    s.ioc.get_executor(),
                    asio::detail::bind_handler(std::move(h), std::error_code{}, bytes_transferred));
            } else {
                assert(s_.code != status::ok);
                auto& s = s_;
                Handler h{std::move(h_)};
                lock.unlock();
                ++s.nread;
                std::error_code ec;
                if (s.code == status::eof)
                    ec = asio::error::eof;
                else if (s.code == status::reset)
                    ec = asio::error::connection_reset;
                asio::post(s.ioc.get_executor(), asio::detail::bind_handler(std::move(h), ec, 0));
            }
        }
    };

    lambda fn_;

  public:
    template <class DeducedHandler>
    read_op(state& s, Buffers const& b, DeducedHandler&& h)
        : fn_(s, b, std::forward<DeducedHandler>(h)) {}

    void operator()() override { fn_.post(); }
};

class socket {
  public:
    using executor_type = asio::io_context::executor_type;

    socket(asio::io_context& ioc, remote_endpoint ep)
        : in_{std::make_shared<state>(ioc)}, ep_{ep} {}

    auto get_executor() noexcept -> asio::io_context::executor_type {
        return in_->ioc.get_executor();
    }

    auto buffer() -> asio::streambuf& { return in_->b; }

    auto remote_endpoint() const noexcept -> const ::test::remote_endpoint& { return ep_; }

    template <class MutableBufferSequence, class ReadHandler>
    ASIO_INITFN_RESULT_TYPE(ReadHandler, void(std::error_code, std::size_t))
    async_read_some(MutableBufferSequence const& buffers, ReadHandler&& handler) {
        using asio::buffer_copy;
        using asio::buffer_size;

        auto init =
            asio::async_completion<ReadHandler, void(std::error_code, std::size_t)>{handler};
        {
            std::unique_lock<std::mutex> lock{in_->m};

            assert(!in_->op);

            if (buffer_size(buffers) == 0 || buffer_size(in_->b.data()) > 0) {
                auto const bytes_transferred = buffer_copy(buffers, in_->b.data(), in_->read_max);
                in_->b.consume(bytes_transferred);
                lock.unlock();
                ++in_->nread;
                asio::post(in_->ioc.get_executor(),
                           asio::detail::bind_handler(std::move(init.completion_handler),
                                                      std::error_code{},
                                                      bytes_transferred));
            } else {
                in_->op.reset(
                    new read_op<ASIO_HANDLER_TYPE(ReadHandler, void(std::error_code, std::size_t)),
                                MutableBufferSequence>{
                        *in_, buffers, std::move(init.completion_handler)});
            }
        }
        return init.result.get();
    }

  private:
    std::shared_ptr<state> in_;

    /// A test class that results in reproducible client info
    const ::test::remote_endpoint ep_;
};

} // namespace test
