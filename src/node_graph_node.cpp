#include "node_graph.h"

using namespace std;

PTNGNode::PTNGNode(vector<PTNGInputSocket> inputs, vector<PTNGOutputSocket> outputs, map<string, PTNGValue>(*evaluation_func)(map<string, PTNGValue>), string node_title)
{
	input_sockets = inputs;
	output_sockets = outputs;
	execution_pointer = evaluation_func;
	title = node_title;

	// TODO: calculate dependencies, apply connections (recipients on the other node!
}

void PTNGNode::execute()
{
	if (execution_pointer == nullptr)
		return;

	// gather input info from input sockets and connected nodes
	map<string, PTNGValue> inputs;
	for (const PTNGInputSocket& socket : input_sockets)
	{
		PTNGValue value;
		if (socket.connected_socket.first == nullptr)
			value = socket.value;
		else
		{
			auto remote = socket.connected_socket.first->getOutputValue(socket.connected_socket.second);
			if (remote.first == socket.type)
				value = remote.second;
			else
			{
				switch (remote.first)
				{
				case PTNGSocketType::SOCKET_TYPE_BOOL:
					switch (socket.type)
					{
					case PTNGSocketType::SOCKET_TYPE_FLOAT: value.f_val = (remote.second.b_val ? 1.0f : 0.0f); break;
					case PTNGSocketType::SOCKET_TYPE_INT: value.i_val = (remote.second.b_val ? 1 : 0); break;
					case PTNGSocketType::SOCKET_TYPE_VECTOR: value.v_val = (remote.second.b_val ? PTVector4f{ 1.0f, 1.0f, 1.0f, 1.0f } : PTVector4f{ 0.0f, 0.0f, 0.0f, 0.0f }); break;
					}
					break;
				case PTNGSocketType::SOCKET_TYPE_FLOAT:
					switch (socket.type)
					{
					case PTNGSocketType::SOCKET_TYPE_BOOL: value.b_val = remote.second.f_val > 0; break;
					case PTNGSocketType::SOCKET_TYPE_INT: value.i_val = (int)remote.second.f_val; break;
					case PTNGSocketType::SOCKET_TYPE_VECTOR: value.v_val = PTVector4f{ remote.second.f_val, remote.second.f_val, remote.second.f_val, remote.second.f_val }; break;
					}
					break;
				case PTNGSocketType::SOCKET_TYPE_INT:
					switch (socket.type)
					{
					case PTNGSocketType::SOCKET_TYPE_BOOL: value.b_val = remote.second.i_val > 0; break;
					case PTNGSocketType::SOCKET_TYPE_FLOAT: value.f_val = (float)remote.second.i_val; break;
					case PTNGSocketType::SOCKET_TYPE_VECTOR: value.v_val = PTVector4f{ (float)remote.second.i_val, (float)remote.second.i_val, (float)remote.second.i_val, (float)remote.second.i_val }; break;
					}
					break;
				case PTNGSocketType::SOCKET_TYPE_VECTOR:
					switch (socket.type)
					{
					case PTNGSocketType::SOCKET_TYPE_BOOL: value.b_val = mag(remote.second.v_val) > 0; break;
					case PTNGSocketType::SOCKET_TYPE_FLOAT: value.f_val = mag(remote.second.v_val); break;
					case PTNGSocketType::SOCKET_TYPE_INT: value.i_val = (int)mag(remote.second.v_val); break;
					}
					break;
				}
			}
		}
		inputs[socket.name] = value;
	}

	// execute the evaluation function
	map<string, PTNGValue> outputs = execution_pointer(inputs);

	// transfer the values to the output sockets
	for (PTNGOutputSocket& socket : output_sockets)
	{
		auto f = outputs.find(socket.name);
		if (f != outputs.end())
			socket.value = f->second;
	}
}

void PTNGNode::reset()
{
	// clear input socket connections and reset values
	for (PTNGInputSocket& socket : input_sockets)
	{
		disconnect(socket.name);
		socket.value = socket.default_value;
	}

	dependencies.clear();
}

void PTNGNode::connect(std::string host_socket_name, std::string remote_socket_name, PTNGNode* remote_node)
{
	if (remote_node == nullptr)
		return;

	int host_socket_index = 0;
	for (const PTNGInputSocket& socket : input_sockets)
	{
		if (socket.name == host_socket_name)
			break;
		host_socket_index++;
	}
	if (host_socket_index >= input_sockets.size())
		return;
	
	int remote_socket_index = 0;
	for (const PTNGOutputSocket& socket : remote_node->output_sockets)
	{
		if (socket.name == remote_socket_name)
			break;
		remote_socket_index++;
	}
	if (remote_socket_index >= remote_node->output_sockets.size())
		return;

	connect(host_socket_index, remote_socket_index, remote_node);
}

void PTNGNode::connect(size_t host_socket_index, size_t remote_socket_index, PTNGNode* remote_node)
{
	if (remote_node == nullptr)
		return;

	disconnect(host_socket_index);

	dependencies.insert(remote_node);
	input_sockets[host_socket_index].connected_socket = pair<PTNGNode*, string>(remote_node, remote_node->output_sockets[remote_socket_index].name);

	remote_node->recipients.insert(this);
}

// TODO: rest of the functions
// TODO: convert dependencies to a reference-counter type thing
