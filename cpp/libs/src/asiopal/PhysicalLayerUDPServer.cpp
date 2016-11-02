/*
 * Licensed to Green Energy Corp (www.greenenergycorp.com) under one or
 * more contributor license agreements. See the NOTICE file distributed
 * with this work for additional information regarding copyright ownership.
 * Green Energy Corp licenses this file to you under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This project was forked on 01/01/2013 by Automatak, LLC and modifications
 * may have been made to this file. Automatak, LLC licenses these modifications
 * to you under the terms of the License.
 */
#include "asiopal/PhysicalLayerUDPServer.h"

#include <asio.hpp>

#include <functional>
#include <string>

#include <openpal/logging/LogMacros.h>
#include <openpal/channel/IPhysicalLayerCallbacks.h>
#include <openpal/logging/LogLevels.h>

using namespace asio;
using namespace openpal;
using namespace std;

namespace asiopal
{

PhysicalLayerUDPServer::PhysicalLayerUDPServer(
    openpal::Logger logger,
    asio::io_service& service,
    const std::string& endpoint,
    uint16_t port,
    std::function<void (asio::ip::udp::socket&)> configure_) :

	PhysicalLayerBaseUDP(logger, service),
	localEndpointString(endpoint),
	localEndpoint(ip::udp::v4(), port),
	configure(configure_)
{

}

/* Implement the actions */
void PhysicalLayerUDPServer::DoOpen()
{
	if (!socket.is_open())
	{
		std::error_code ec;
		auto address = asio::ip::address::from_string(localEndpointString, ec);
		if (ec)
		{
			auto lambda = [this, ec]()
			{
				this->OnOpenCallback(ec);
			};
			pExecutor->PostLambda(lambda);
		}
		else
		{
			localEndpoint.address(address);
			socket.open(localEndpoint.protocol(), ec);
			if (ec)
			{
				auto lambda = [this, ec]()
				{
					this->OnOpenCallback(ec);
				};
				pExecutor->PostLambda(lambda);
			}
			else
			{
				socket.set_option(ip::udp::socket::reuse_address(true));
				socket.bind(localEndpoint, ec);
				if (ec)
				{
					auto lambda = [this, ec]()
					{
						this->OnOpenCallback(ec);
					};
					pExecutor->PostLambda(lambda);
				}
			}
		}
	}
}

void PhysicalLayerUDPServer::DoOpenCallback()
{
	this->CloseSocket();
}

void PhysicalLayerUDPServer::DoOpeningClose()
{
	this->CloseSocket();
}

void PhysicalLayerUDPServer::DoOpenSuccess()
{
	FORMAT_LOG_BLOCK(logger, logflags::INFO, "Accepted connection from: %s", remoteEndpoint.address().to_string().c_str());
	configure(socket);
}

}


