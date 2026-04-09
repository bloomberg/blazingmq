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

#include <bmqio_channelfactorypipeline.h>
#include <bslma_allocator.h>
#include <bslma_managedptr.h>

namespace BloombergLP {
namespace bmqio {

ChannelFactoryPipeline::Config::Config(const ChannelFactorySP& baseFactory,
                                       const allocator_type&   allocator)
: d_pipeline(1, baseFactory, allocator)
{
    BSLS_ASSERT(baseFactory != NULL);
}

ChannelFactoryPipeline::Config::Config(bslmf::MovableRef<Config> other)
    BSLS_KEYWORD_NOEXCEPT
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
             MoveUtil::access(other).d_pipeline.get_allocator())
{
}

ChannelFactoryPipeline::Config::Config(bslmf::MovableRef<Config> other,
                                       const allocator_type&     allocator)
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline), allocator)
{
}

ChannelFactoryPipeline::Config::allocator_type
ChannelFactoryPipeline::Config::get_allocator() const
{
    return d_pipeline.get_allocator();
}

ChannelFactoryPipeline::Config&
ChannelFactoryPipeline::Config::add(const ChannelFactorySP& channelFactory)
{
    BSLS_ASSERT(channelFactory != NULL);

    d_pipeline.push_back(channelFactory);

    return *this;
}

ChannelFactoryPipeline::Config&
ChannelFactoryPipeline::Config::addWith(const ChannelFactoryBuilder& builder)
{
    ChannelFactorySP last;
    if (!d_pipeline.empty()) {
        last = d_pipeline.back();
    }
    ChannelFactorySP channelFactory = builder(last);
    add(channelFactory);

    return *this;
}

size_t ChannelFactoryPipeline::Config::size() const
{
    return d_pipeline.size();
}

ChannelFactoryPipeline::ChannelFactoryPipeline(
    bslmf::MovableRef<ChannelFactoryPipeline> other) BSLS_KEYWORD_NOEXCEPT
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
             MoveUtil::access(other).get_allocator())
{
    BSLS_ASSERT(!d_pipeline.empty());
}

ChannelFactoryPipeline::ChannelFactoryPipeline(
    bslmf::MovableRef<ChannelFactoryPipeline> other,
    const allocator_type&                     allocator)
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline), allocator)
{
    BSLS_ASSERT(!d_pipeline.empty());
}

ChannelFactoryPipeline::ChannelFactoryPipeline(
    bslmf::MovableRef<Config> builder,
    const allocator_type&     allocator)
: d_pipeline(MoveUtil::move(MoveUtil::access(builder).d_pipeline), allocator)
{
    BSLS_ASSERT(!d_pipeline.empty());
}

ChannelFactoryPipeline::allocator_type
ChannelFactoryPipeline::get_allocator() const
{
    return d_pipeline.get_allocator();
}

const ChannelFactoryPipeline::ChannelFactorySP&
ChannelFactoryPipeline::top() const
{
    return d_pipeline.back();
}

void ChannelFactoryPipeline::listen(bmqio::Status*               status,
                                    bslma::ManagedPtr<OpHandle>* handle,
                                    const bmqio::ListenOptions&  options,
                                    const ResultCallback&        cb)
{
    top()->listen(status, handle, options, cb);
}

void ChannelFactoryPipeline::connect(bmqio::Status*               status,
                                     bslma::ManagedPtr<OpHandle>* handle,
                                     const bmqio::ConnectOptions& options,
                                     const ResultCallback&        cb)
{
    top()->connect(status, handle, options, cb);
}

int ChannelFactoryPipeline::start()
{
    return top()->start();
}

void ChannelFactoryPipeline::stop()
{
    top()->stop();
}

}
}
