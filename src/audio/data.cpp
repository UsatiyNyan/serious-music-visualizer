//
// Created by usatiynyan.
//

#include "audio/data.hpp"

#include <sl/exec/algo/emit/detach.hpp>
#include <sl/exec/algo/make/result.hpp>
#include <sl/exec/algo/sched/continue_on.hpp>
#include <sl/exec/algo/tf/seq/map.hpp>
#include <sl/exec/model/syntax.hpp>

namespace audio {

void DataCallback::operator()(std::span<const float> input) {
    using namespace sl::exec;

    value_as_signal(ChunkT{ input.begin(), input.end() }) //
        | continue_on(sync_executor_) //
        | map([this](ChunkT&& chunk) {
              if (chunk_.empty()) {
                  chunk_ = std::move(chunk);
              } else {
                  chunk_.insert(chunk_.end(), chunk.begin(), chunk.end());
              }
              return sl::meta::unit{};
          })
        | detach();
}

} // namespace audio
