#include "w_system_pch.h"
#include "w_network.h"

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pair.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/survey.h>
#include <nanomsg/bus.h>

namespace wolf
{
    namespace system
    {
        class w_network_pimp
        {
        public:

			enum w_socket_connection_type : uint8_t
			{
				CONNECT = 1,
				BIND,
			};

			struct w_socket_options
			{
				int socket_level = 0;
				int option = 0;
				void* option_value = nullptr;
				int option_value_length = 0;
			};

            w_network_pimp() :
                _name("w_network")
            {

            }

			W_RESULT initialize(
				_In_z_ const char* pURL,
                _In_ const int& pDomain,
                _In_ const int& pProtocol,
                _In_ uint8_t pConnectionType,
				_In_ w_signal<void(const int& pSocketID)> pOnConnectOrBindEstablished,
				_In_ w_socket_options* pSocketOption,
				_In_ std::initializer_list<const char*> pConnectURLs = {})
            {
				const std::string _trace_info = this->_name + "::setup_one_way_puller";

				this->_socket = nn_socket(pDomain, pProtocol);
				if (this->_socket < 0)
				{
					V(W_FAILED, "creating socket", _trace_info, 3);
					return W_FAILED;
				}
                
				//set socket option
				if (pSocketOption)
				{
					set_socket_option(pSocketOption);
				}

				switch (pConnectionType)
				{
				case w_socket_connection_type::BIND:
					//on bind, which used for server
					if (nn_bind(this->_socket, pURL) < 0)
					{
						V(W_FAILED, "binding to " + std::string(pURL), _trace_info, 3);
						return W_FAILED;
					}
					break;
				case w_socket_connection_type::CONNECT:
					//on conneect, which used for client
					if (nn_connect(this->_socket, pURL) < 0)
					{
						V(W_FAILED, "connecting to " + std::string(pURL), _trace_info, 3);
						return W_FAILED;
					}
					break;

				}

				//this will be used just for BUS
				for (auto& con : pConnectURLs)
				{
					if (nn_connect(this->_socket, con) < 0)
					{
						V(W_FAILED, "connecting to " + std::string(con), _trace_info, 3);
						return W_FAILED;
					}
				}

				//rise on connect or bind
				pOnConnectOrBindEstablished(this->_socket);
                return W_PASSED;
            }
            
            //http://nanomsg.org/v0.1/nn_setsockopt.3.html
			W_RESULT set_socket_option(_In_ w_socket_options* pSocketOption)
            {
				if (!pSocketOption) return W_FAILED;
				
                const std::string _trace_info = this->_name + "::set_socket_option";
                if(nn_setsockopt(
					this->_socket, 
					pSocketOption->socket_level, 
					pSocketOption->option,
					pSocketOption->option_value,
					pSocketOption->option_value_length) < 0)
                {
                    V(W_FAILED, "setting socket option. Level: " + std::to_string(pSocketOption->socket_level) +
                      " Option:" + std::to_string(pSocketOption->option), _trace_info, 3);
                    return W_FAILED;
                }
                return W_PASSED;
            }
            
            int release()
            {
				return nn_shutdown(this->_socket, 0);
            }

        private:
            std::string     _name;
            int             _socket;
        };
    }
}

using namespace wolf::system;

w_network::w_network() : _pimp(new w_network_pimp())
{
}

w_network::~w_network()
{
    release();
}

W_RESULT w_network::setup_one_way_pusher(
    _In_z_ const char* pURL,
	_In_ w_signal<void(const int& pSocketID)> pOnConnectionEstablishedCallback)
{
    if (!this->_pimp) return W_FAILED;
    return this->_pimp->initialize(
		pURL, 
		AF_SP, NN_PUSH, 
		w_network_pimp::w_socket_connection_type::CONNECT, 
		pOnConnectionEstablishedCallback, 
		nullptr);
}

W_RESULT w_network::setup_one_way_puller(
	_In_z_ const char* pURL,
	_In_ w_signal<void(const int& pSocketID)> pOnBindEstablishedCallback)
{
    if (!this->_pimp) return W_FAILED;
    return this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_PULL, 
		w_network_pimp::w_socket_connection_type::BIND, 
		pOnBindEstablishedCallback, 
		nullptr);
}

W_RESULT w_network::setup_two_way_server(
    _In_z_ const char* pURL,
    _In_ int pReceiveTime,
    _In_ w_signal<void(const int& pSocketID)> pOnBindEstablishedCallback)
{
    if (!this->_pimp) return W_FAILED;
	
	const std::string _trace_info = "w_network::setup_two_way_server";

	auto _socket_options = (w_network_pimp::w_socket_options*)malloc(sizeof(w_network_pimp::w_socket_options));
	if (_socket_options)
	{
		_socket_options->socket_level = NN_SOL_SOCKET;
		_socket_options->option = NN_RCVTIMEO;
		_socket_options->option_value = &pReceiveTime;
		_socket_options->option_value_length = sizeof(pReceiveTime);
	}
	else
	{
		V(W_FAILED, "allocating memory for socket option", _trace_info, 3);
	}
	auto _hr = this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_PAIR, 
		w_network_pimp::w_socket_connection_type::BIND, 
		pOnBindEstablishedCallback, 
		_socket_options);
	if (_socket_options)
	{
		free(_socket_options);
	}
    return _hr;
}

W_RESULT w_network::setup_two_way_client(
    _In_z_ const char* pURL,
    _In_ int pReceiveTime,
    _In_ w_signal<void(const int& pSocketID)> pOnConnectionEstablishedCallback)
{
    if (!this->_pimp) return W_FAILED;

	const std::string _trace_info = "w_network::setup_two_way_client";
    
	auto _socket_options = (w_network_pimp::w_socket_options*)malloc(sizeof(w_network_pimp::w_socket_options));
	if (_socket_options)
	{
		_socket_options->socket_level = NN_SOL_SOCKET;
		_socket_options->option = NN_RCVTIMEO;
		_socket_options->option_value = &pReceiveTime;
		_socket_options->option_value_length = sizeof(pReceiveTime);
	}
	else
	{
		V(W_FAILED, "allocating memory for socket option", _trace_info, 3);
	}

	auto _hr = this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_PAIR, 
		w_network_pimp::w_socket_connection_type::CONNECT, 
		pOnConnectionEstablishedCallback, 
		_socket_options);
	if (_socket_options)
	{
		free(_socket_options);
	}
    return _hr;
}

W_RESULT w_network::setup_broadcast_publisher(
    _In_z_ const char* pURL,
    _In_ w_signal<void(const int& pSocketID)> pOnBindEstablishedCallback)
{
    if (!this->_pimp) return W_FAILED;
    return this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_PUB, 
		w_network_pimp::w_socket_connection_type::BIND, 
		pOnBindEstablishedCallback, 
		nullptr);
}

W_RESULT w_network::setup_broadcast_subscriptore(
    _In_z_ const char* pURL,
    _In_ w_signal<void(const int& pSocketID)> pOnConnectionEstablishedCallback)
{
    if (!this->_pimp) return W_FAILED;
	
	const std::string _trace_info = "w_network::setup_broadcast_subscriptore";

	auto _socket_options = (w_network_pimp::w_socket_options*)malloc(sizeof(w_network_pimp::w_socket_options));
	if (_socket_options)
	{
		_socket_options->socket_level = NN_SUB;
		_socket_options->option = NN_SUB_SUBSCRIBE;
		_socket_options->option_value = (void*)"";
		_socket_options->option_value_length = 0;
	}
	else
	{
		V(W_FAILED, "allocating memory for socket option", _trace_info, 3);
	}

	auto _hr = this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_SUB, 
		w_network_pimp::w_socket_connection_type::CONNECT, 
		pOnConnectionEstablishedCallback, 
		_socket_options);
	if (_socket_options)
	{
		free(_socket_options);
	}
    return _hr;
}

W_RESULT w_network::setup_survey_server(
	_In_z_ const char* pURL,
	_In_ w_signal<void(const int& pSocketID)> pOnBindEstablishedCallback)
{
	if (!this->_pimp) return W_FAILED;
	return this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_SURVEYOR, 
		w_network_pimp::w_socket_connection_type::BIND, 
		pOnBindEstablishedCallback, 
		nullptr);
}

W_RESULT w_network::setup_survey_client(
	_In_z_ const char* pURL,
	_In_ w_signal<void(const int& pSocketID)> pOnConnectionEstablishedCallback)
{
	if (!this->_pimp) return W_FAILED;
	return this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_RESPONDENT, 
		w_network_pimp::w_socket_connection_type::CONNECT, 
		pOnConnectionEstablishedCallback, 
		nullptr);
}

W_RESULT w_network::setup_bus_node(
	_In_z_ const char* pURL,
	_In_ int pReceiveTime,
	_In_ w_signal<void(const int& pSocketID)> pOnBindEstablishedCallback,
	_In_ std::initializer_list<const char*> pConnectURLs)
{
	if (!this->_pimp) return W_FAILED;

	const std::string _trace_info = "w_network::setup_bus_node";

	auto _socket_options = (w_network_pimp::w_socket_options*)malloc(sizeof(w_network_pimp::w_socket_options));
	if (_socket_options)
	{
		_socket_options->socket_level = NN_SOL_SOCKET;
		_socket_options->option = NN_RCVTIMEO;
		_socket_options->option_value = &pReceiveTime;
		_socket_options->option_value_length = sizeof(pReceiveTime);
	}
	else
	{
		V(W_FAILED, "allocating memory for socket option", _trace_info, 3);
	}

	auto _hr = this->_pimp->initialize(
		pURL, 
		AF_SP, 
		NN_BUS, 
		w_network_pimp::w_socket_connection_type::BIND,
		pOnBindEstablishedCallback, 
		_socket_options,
		pConnectURLs);
	if (_socket_options)
	{
		free(_socket_options);
	}
	return _hr;
}

ULONG w_network::free_buffer(_In_z_ char* pBuffer)
{
	if (!pBuffer) return W_PASSED;
	const std::string _trace_info = "w_network::free_buffer";
	if (nn_freemsg(pBuffer) < 0)
	{
		V(W_FAILED, "free buffer", _trace_info, 3);
		return W_FAILED;
	}
	return W_PASSED;
}

w_network_error w_network::get_last_error()
{
	return (w_network_error)nn_errno();
}

ULONG w_network::release()
{
    if (!this->_pimp || _super::get_is_released()) return 1;
    
	if (this->_pimp->release() < 0) return 1;

    return _super::release();
}

int w_network::send(_In_ const int& pSocketID, _In_z_ const char* pMessage, _In_ const size_t& pMessageSize)
{
    return nn_send(pSocketID, pMessage, pMessageSize, 0);
}

int w_network::receive(_In_ const int& pSocketID, _In_opt_z_ char** pBuffer)
{
    return nn_recv(pSocketID, pBuffer, NN_MSG, 0);
}
