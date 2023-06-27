/*
 * Copyright 2023, chloro-pn;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TUNNEL_EMPTY_SOURCE_H
#define TUNNEL_EMPTY_SOURCE_H

#include "tunnel/source.h"

namespace tunnel {

template <typename T>
class EmptySource : public Source<T> {
 public:
  virtual async_simple::coro::Lazy<std::optional<T>> generate() override { co_return std::optional<T>{}; }
};

}  // namespace tunnel

#endif