// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "carla/rpc/Metadata.h"

#include <functional>
#include <future>

namespace carla {
namespace rpc {

  /// Function traits based on http://rpclib.net implementation.
  /// MIT Licensed, Copyright (c) 2015-2017, Tamás Szelei
  template <typename T>
  struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

  template <typename C, typename R, typename... Args>
  struct FunctionTraits<R (C::*)(Args...)> : FunctionTraits<R (*)(Args...)> {};

  template <typename C, typename R, typename... Args>
  struct FunctionTraits<R (C::*)(Args...) const> : FunctionTraits<R (*)(Args...)> {};

  template<class T>
  struct FunctionTraits<T &> : public FunctionTraits<T> {};

  template<class T>
  struct FunctionTraits<T &&> : public FunctionTraits<T> {};

  template <typename R, typename... Args>
  struct FunctionTraits<R (*)(Args...)> {
    using ResultType = R;
    using FunctionType = std::function<R(Args...)>;
    using RPCFunctionType = std::function<R(::carla::rpc::Metadata, Args...)>;
    using PackagedTaskType = std::packaged_task<R()>;

    /// Wraps a function object into a lambda suitable for rpclib. Useful to
    /// convert a lambda created with "auto" arguments and return value into a
    /// lambda with defined signature.
    template <typename FuncT>
    static auto WrapForRPC(FuncT &&func) {
      return [f=std::forward<FuncT>(func)](::carla::rpc::Metadata metadata, Args... args) -> R {
        return f(metadata, args...);
      };
    }
  };

} // namespace rpc
} // namespace carla
