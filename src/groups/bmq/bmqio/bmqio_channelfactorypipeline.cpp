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

namespace BloombergLP {
namespace bmqio {

ChannelFactoryPipeline::Builder::Builder()
: d_pipeline()
{
}

ChannelFactoryPipeline::Builder::Builder(const allocator_type& allocator)
: d_pipeline(allocator)
{
}

ChannelFactoryPipeline::Builder::Builder(bslmf::MovableRef<Builder> other)
    BSLS_KEYWORD_NOEXCEPT
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
             MoveUtil::access(other).d_pipeline.get_allocator())
{
}

ChannelFactoryPipeline::Builder::Builder(bslmf::MovableRef<Builder> other,
                                         const allocator_type&      allocator)
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline), allocator)
{
}

ChannelFactoryPipeline::Builder::allocator_type
ChannelFactoryPipeline::Builder::get_allocator() const
{
    return d_pipeline.get_allocator();
}

void ChannelFactoryPipeline::Builder::reset()
{
    d_pipeline.clear();
}

ChannelFactoryPipeline::Builder&
ChannelFactoryPipeline::Builder::add(const ChannelFactorySP& channelFactory)
{
    BSLS_ASSERT(channelFactory != NULL);

    d_pipeline.push_back(channelFactory);

    return *this;
}

ChannelFactoryPipeline::Builder&
ChannelFactoryPipeline::Builder::addWith(const ChannelFactoryBuilder& builder)
{
    ChannelFactorySP last;
    if (!d_pipeline.empty()) {
        last = d_pipeline.back();
    }
    ChannelFactorySP channelFactory = builder(last);
    add(channelFactory);

    return *this;
}

ChannelFactoryPipeline ChannelFactoryPipeline::Builder::build()
{
    ChannelFactoryPipeline pipeline(MoveUtil::move(*this), get_allocator());
    reset();
    return pipeline;
}

size_t ChannelFactoryPipeline::Builder::size() const
{
    return d_pipeline.size();
}

ChannelFactoryPipeline::ChannelFactoryPipeline(
    bslmf::MovableRef<ChannelFactoryPipeline> other) BSLS_KEYWORD_NOEXCEPT
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline),
             MoveUtil::access(other).get_allocator())
{
}

ChannelFactoryPipeline::ChannelFactoryPipeline(
    bslmf::MovableRef<ChannelFactoryPipeline> other,
    const allocator_type&                     allocator)
: d_pipeline(MoveUtil::move(MoveUtil::access(other).d_pipeline), allocator)
{
}

ChannelFactoryPipeline::ChannelFactoryPipeline(
    bslmf::MovableRef<Builder> builder,
    const allocator_type&      allocator)
: d_pipeline(MoveUtil::move(MoveUtil::access(builder).d_pipeline), allocator)
{
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
