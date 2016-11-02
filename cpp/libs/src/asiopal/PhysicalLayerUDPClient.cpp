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
#include "asiopal/PhysicalLayerUDPClient.h"

#include "asiopal/SocketHelpers.h"

#include <asio.hpp>

#include <functional>
#include <string>

#include <openpal/channel/IPhysicalLayerCallbacks.h>
#include <openpal/logging/LogLevels.h>
#include <iostream>

using namespace asio;
using namespace std;
using namespace openpal;

namespace asiopal
{

PhysicalLayerUDPClient::PhysicalLayerUDPClient(
    openpal::Logger logger,
    asio::io_service& service,
    const std::string& host_,
    const std::string& localAddress_,
    uint16_t port,
    std::function<void (asio::ip::udp::socket&)> aConfigure) :

	PhysicalLayerBaseUDP(logger, service),
	condition(logger),
	host(host_),
	localAddress(localAddress_),
	remoteEndpoint(ip::udp::v4(), port),
	localEndpoint(),
	resolver(service),
	configure(aConfigure)
{
    std::cout <<"Opening UDP Client to " << host_ << ":" << port << std::endl;
}

/* Implement the actions */
void PhysicalLayerUDPClient::DoOpen()
{
	std::error_code ec;
	SocketHelpers::BindToLocalAddress(localAddress, localEndpoint, socket, ec);

	if (ec)
	{
		auto callback = [this, ec]()
		{
			this->OnOpenCallback(ec);
		};
		executor.strand.post(callback);
	}
	else
	{
		auto address = asio::ip::address::from_string(host, ec);
		if (ec)
		{
			auto callback = [this](const std::error_code & code, ip::udp::resolver::iterator endpoints)
			{
				this->HandleResolve(code, endpoints);
			};
			stringstream portstr;
			portstr << remoteEndpoint.port();
			ip::udp::resolver::query query(host, portstr.str());
			resolver.async_resolve(query, executor.strand.wrap(callback));
		}
		else
		{
			remoteEndpoint.address(address);
			auto callback = [this](const std::error_code & code)
			{
				this->OnOpenCallback(code);
			};
			socket.async_connect(remoteEndpoint, executor.strand.wrap(callback));
		}
	}
}

void PhysicalLayerUDPClient::HandleResolve(const std::error_code& code, asio::ip::udp::resolver::iterator endpoints)
{
	if (code)
	{
		this->OnOpenCallback(code);
	}
	else
	{
		// attempt a connection to each endpoint in the iterator until we connect
		auto callback = [this](const std::error_code & code, ip::udp::resolver::iterator endpoints)
		{
			this->OnOpenCallback(code);
		};

		asio::async_connect(socket, endpoints, condition, executor.strand.wrap(callback));
	}
}

void PhysicalLayerUDPClient::DoOpeningClose()
{
	this->CloseSocket();
}

void PhysicalLayerUDPClient::DoOpenSuccess()
{
	SIMPLE_LOG_BLOCK(logger, logflags::INFO, "Connected to host");
	configure(socket);
}

}


