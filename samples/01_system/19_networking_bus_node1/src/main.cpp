/*
	Project			 : Wolf Engine. Copyright(c) Pooya Eimandar (http://PooyaEimandar.com) . All rights reserved.
	Source			 : Please direct any bug to https://github.com/PooyaEimandar/Wolf.Engine/issues
	Website			 : http://WolfSource.io
	Name			 : main.cpp
	Description		 : This sample shows how to create one two way client in Wolf Engine
					   make sure launch 18_networking_bus_node0 simultaneously
	Comment          : Read more information about this sample on http://wolfsource.io/gpunotes/wolfengine/
*/

#include "pch.h"

//namespaces
using namespace wolf;
using namespace wolf::system;

//Entry point of program 
WOLF_MAIN()
{
    const wchar_t* _name = L"19_networking_bus_node1";
    logger.initialize(_name, wolf::system::io::get_current_directoryW());
    
    w_signal<void(const int&)> on_bind_established;
	on_bind_established += [](const int& pSocketID)
	{
		logger.write("node just attached to bus on socket ID: " + std::to_string(pSocketID));

		while (true)
		{
			//send
			std::string _msg = "Node1 " + w_time_span::now().to_string();
			if (w_network::send(pSocketID, _msg.c_str(), _msg.size()) >= 0)
			{
				logger.write("message \'" + _msg + "\' sent.");
			}
			_msg.clear();

			//receive
			char* _rcv_buffer = nullptr;
			auto _rcv_len = w_network::receive(pSocketID, &_rcv_buffer);
			if (_rcv_len >= 0)
			{
				std::string _rcv_msg;
				_rcv_msg.resize(_rcv_len);
				memcpy(&_rcv_msg[0], _rcv_buffer, _rcv_len);

				logger.write("message \'" + _rcv_msg + "\' received");

				_rcv_msg.clear();
			}
			else
			{
				logger.write("could not receive");
			}
			w_network::free_buffer(_rcv_buffer);

			//wait for 1 second
			w_thread::sleep_current_thread(1000);
		}
	};
    
    w_network _survey_server;
	_survey_server.setup_bus_node(
		"ipc:///tmp/node1.ipc",
		100,
		on_bind_established,
		{ "ipc:///tmp/node0.ipc" });
    
    //sleep 5 sec
    w_thread::sleep_current_thread(5000);
    
	_survey_server.release();
    logger.release();

	return EXIT_SUCCESS;
}
