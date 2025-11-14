// Copyright 2019-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_BMQIO_CHANNELFACTORYPIPELINE
#define INCLUDED_BMQIO_CHANNELFACTORYPIPELINE

#include <bmqio_channelfactory.h>

#include <bsl_memory.h>
#include <bsl_vector.h>
#include <bslmf_movableref.h>
#include <utility>

namespace BloombergLP {
namespace bmqio {

class ChannelFactoryPipeline_Impl {};

class ChannelFactoryPipeline : public bmqio::ChannelFactory {
  private:
    // PRIVATE TYPES
    typedef bslmf::MovableRefUtil MoveUtil;

  public:
    // TYPES
    typedef bsl::shared_ptr<bmqio::ChannelFactory> ChannelFactorySP;

    /// A builder class for constructing `ChannelFactoryPipelines`.
    class Builder {
      private:
        // PRIVATE DATA
        bsl::vector<ChannelFactorySP> d_pipeline;

        // FRIENDS
        friend class ChannelFactoryPipeline;

      public:
        /// The allocator type.
        typedef bsl::allocator<unsigned char> allocator_type;

        /// A builder for channel factories.
        typedef bsl::function<ChannelFactorySP(ChannelFactorySP&)>
            ChannelFactoryBuilder;

        // CONSTRUCTORS

        // Create an empty pipeline builder.
        Builder()
        : d_pipeline()
        {
        }
        explicit Builder(const allocator_type& allocator)
        : d_pipeline(allocator)
        {
        }
        Builder(bslmf::MovableRef<Builder> other) BSLS_KEYWORD_NOEXCEPT
        : d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
                     MoveUtil::access(other).d_pipeline.get_allocator())
        {
        }
        Builder(bslmf::MovableRef<Builder> other,
                const allocator_type&      allocator)
        : d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
                     allocator)
        {
        }

      private:
        // PRIVATE CONSTRUCTORS

        Builder(const Builder& other) BSLS_KEYWORD_DELETED;
        Builder& operator=(const Builder& other) BSLS_KEYWORD_DELETED;

      public:
        // ACESSORS

        allocator_type get_allocator() const
        {
            return d_pipeline.get_allocator();
        }

        // MANIPULATORS

        /// Resets the pipeline builder to the empty state, as on construction
        void reset() { d_pipeline.clear(); }

        /// Add a `ChannelFactory` as the next step in the pipeline.
        ///
        /// @pre `channelFactory` must not be null.
        Builder& add(const ChannelFactorySP& channelFactory)
        {
            BSLS_ASSERT(channelFactory != NULL);

            d_pipeline.push_back(channelFactory);

            return *this;
        }

        /// Add a `ChannelFactory` as the next step in the pipeline,
        /// constructed from the provided factory. The parameter is a reference
        /// to the previous factory in the pipeline. If there are no factories
        /// in the pipeline, the parameter is NULL.
        ///
        /// @pre `builder(previous)` must not return null
        Builder& addWith(const ChannelFactoryBuilder& builder)
        {
            ChannelFactorySP last;
            if (!d_pipeline.empty()) {
                last = d_pipeline.back();
            }
            ChannelFactorySP channelFactory = builder(last);
            add(channelFactory);

            return *this;
        }

        /// Construct a `ChannelFactoryPipeline` using `this` and `get_allocator()`
        ChannelFactoryPipeline build()
        {
            ChannelFactoryPipeline pipeline(MoveUtil::move(*this),
                                            get_allocator());
            reset();
            return pipeline;
        }

        /// Get the current size of the pipeline.
        size_t size() const { return d_pipeline.size(); }
    };

  private:
    // PRIVATE DATA
    bsl::vector<ChannelFactorySP> d_pipeline;

  public:
    // TYPES

    /// The allocator type.
    typedef bsl::allocator<unsigned char> allocator_type;

    ChannelFactoryPipeline(bslmf::MovableRef<ChannelFactoryPipeline> other)
        BSLS_KEYWORD_NOEXCEPT
    : d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
                 MoveUtil::access(other).get_allocator())
    {
    }

    ChannelFactoryPipeline(bslmf::MovableRef<ChannelFactoryPipeline> other,
                           const allocator_type&                     allocator)
    : d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline), allocator)
    {
    }

    ChannelFactoryPipeline(bslmf::MovableRef<Builder> builder,
                           const allocator_type& allocator = allocator_type())
    : d_pipeline(MoveUtil::move(MoveUtil::access(builder).d_pipeline),
                 allocator)
    {
    }

  private:
    // PRIVATE CONSTRUCTORS

    ChannelFactoryPipeline(const ChannelFactoryPipeline& other)
        BSLS_KEYWORD_DELETED;

    // PRIVATE ACCESSORS

    /// Get the top of the stack of channel factories.
    const ChannelFactorySP& top() const { return d_pipeline.back(); }

  public:
    // ACESSORS

    allocator_type get_allocator() const { return d_pipeline.get_allocator(); }

    // MANIPULATORS

    void listen(bmqio::Status*               status,
                bslma::ManagedPtr<OpHandle>* handle,
                const bmqio::ListenOptions&  options,
                const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE
    {
        top()->listen(status, handle, options, cb);
    }

    void connect(bmqio::Status*               status,
                 bslma::ManagedPtr<OpHandle>* handle,
                 const bmqio::ConnectOptions& options,
                 const ResultCallback&        cb) BSLS_KEYWORD_OVERRIDE
    {
        top()->connect(status, handle, options, cb);
    }

    int start() BSLS_KEYWORD_OVERRIDE { return top()->start(); }

    void stop() BSLS_KEYWORD_OVERRIDE { top()->stop(); }
};

}
}
#endif
