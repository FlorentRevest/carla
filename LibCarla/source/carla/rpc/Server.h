// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "carla/Time.h"
#include "carla/rpc/FunctionTraits.h"
#include "carla/rpc/Metadata.h"
#include "carla/rpc/Response.h"

#include <boost/asio/io_service.hpp>

#include <rpc/server.h>

namespace carla {
namespace rpc {

  // ===========================================================================
  // -- Server -----------------------------------------------------------------
  // ===========================================================================

  /// An RPC server in which functions can be bind to run synchronously or
  /// asynchronously.
  ///
  /// Use `AsyncRun` to start the worker threads, and use `SyncRunFor` to
  /// run a slice of work in the caller's thread.
  ///
  /// Functions that are bind using `BindAsync` will run asynchronously in the
  /// worker threads. Functions that are bind using `BindSync` will run within
  /// `SyncRunFor` function.
  class Server {
  public:

    template <typename... Args>
    explicit Server(Args &&... args);

    template <typename FunctorT>
    void BindSync(const std::string &name, FunctorT &&functor);

    template <typename FunctorT>
    void BindAsync(const std::string &name, FunctorT &&functor);

    void AsyncRun(size_t worker_threads) {
      _server.async_run(worker_threads);
    }

    void SyncRunFor(time_duration duration) {
      _sync_io_service.reset();
      _sync_io_service.run_for(duration.to_chrono());
    }

    /// @warning does not stop the game thread.
    void Stop() {
      _server.stop();
    }

  private:

    boost::asio::io_service _sync_io_service;

    ::rpc::server _server;
  };

  // ===========================================================================
  // -- Server implementation --------------------------------------------------
  // ===========================================================================

namespace detail {

  /// Wraps @a functor into a function type with equivalent signature. The wrap
  /// function returned, when called, posts @a functor into the io_service and
  /// waits for it to finish.
  ///
  /// This way, no matter from which thread the wrap function is called, the
  /// @a functor provided is always called from the context of the io_service.
  ///
  /// @warning The wrap function blocks until @a functor is executed by the
  /// io_service.
  template <typename F>
  inline static auto WrapSyncCall(boost::asio::io_service &io, F &&functor) {
    using func_t = typename FunctionTraits<F>::RPCFunctionType;
    using task_t = typename FunctionTraits<F>::PackagedTaskType;
    using result_t = typename func_t::result_type;

    return func_t([&io, functor=std::forward<F>(functor)](
        Metadata metadata,
        auto &&... args) -> result_t {
      auto task = std::make_shared<task_t>([functor=std::move(functor), args...]() {
        return functor(args...);
      });
      auto result = task->get_future();
      io.post([task=std::move(task)]() mutable { (*task)(); });
      if (metadata.IsResponseIgnored()) {
        // Return default constructed result.
        return result_t{};
      } else {
        // Wait for result.
        return result.get();
      }
    });
  }

  template <typename F>
  inline static auto WrapAsyncCall(F &&functor) {
    return FunctionTraits<F>::WrapForRPC([f=std::forward<F>(functor)](Metadata, auto &&... args) {
      return f(std::forward<decltype(args)>(args)...);
    });
  }

} // namespace detail

  template <typename ... Args>
  inline Server::Server(Args && ... args)
    : _server(std::forward<Args>(args) ...) {
    _server.suppress_exceptions(true);
  }

  template <typename FunctorT>
  inline void Server::BindSync(const std::string &name, FunctorT &&functor) {
    _server.bind(
        name,
        detail::WrapSyncCall(_sync_io_service, std::forward<FunctorT>(functor)));
  }

  template <typename FunctorT>
  inline void Server::BindAsync(const std::string &name, FunctorT &&functor) {
    _server.bind(
        name,
        detail::WrapAsyncCall(std::forward<FunctorT>(functor)));
  }

} // namespace rpc
} // namespace carla
