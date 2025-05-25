#pragma once

#include <vector>
#include <string>
#include <vector2.h>
#include <vector4.h>
#include <set>
#include <map>

enum PTNGSocketType
{
	SOCKET_TYPE_BOOL,
	SOCKET_TYPE_INT,
	SOCKET_TYPE_FLOAT,
	SOCKET_TYPE_VECTOR
};

struct PTNGValue
{
	union
	{
		bool b_val;
		int i_val;
		float f_val;
		PTVector4f v_val = PTVector4f{ 0, 0, 0, 0 };
	};

	inline PTNGValue operator=(const PTNGValue& other)
	{
		memcpy(this, &other, sizeof(PTNGValue));
		return *this;
	}
};

class PTNGNode;

struct PTNGInputSocket
{
	std::string name;
	PTNGSocketType type = SOCKET_TYPE_VECTOR;
	PTNGValue value;
	PTNGValue default_value;
	std::pair<PTNGNode*, std::string> connected_socket;
};

struct PTNGOutputSocket
{
	std::string name;
	PTNGSocketType type = SOCKET_TYPE_VECTOR;
	PTNGValue value;
};

class PTNGNode
{
private:
	std::vector<PTNGInputSocket> input_sockets;
	std::set<PTNGNode*> dependencies; // TODO: convert to reference counter
	std::vector<PTNGOutputSocket> output_sockets;
	std::set<PTNGNode*> recipients; // TODO: convert to reference counter
	std::map<std::string, PTNGValue> (*execution_pointer)(std::map<std::string, PTNGValue>) = nullptr;
	PTVector2f position = PTVector2f{ 0, 0 };
	std::string title = "Node";

	// TODO: add handling for duplicately named inputs and outputs
public:
	PTNGNode() = delete;
	PTNGNode(std::vector<PTNGInputSocket> inputs, std::vector<PTNGOutputSocket> outputs, std::map<std::string, PTNGValue>(*evaluation_func)(std::map<std::string, PTNGValue>), std::string node_title = "Node");
	
	void execute();
	void reset();
	void connect(std::string host_socket_name, std::string remote_socket_name, PTNGNode* remote_node);
	void connect(size_t host_socket_index, size_t remote_socket_index, PTNGNode* remote_node);
	void disconnect(std::string host_socket_name);
	void disconnect(size_t host_socket_index);
	void disconnect(PTNGNode* remote_node);

	inline void setPosition(PTVector2f new_position) { position = new_position; }
	inline PTVector2f getPosition() const { return position; }
	inline void setTitle(std::string new_title) { title = new_title; }
	inline std::string getTitle() const { return title; }
	inline std::vector<PTNGInputSocket> getInputs() const { return input_sockets; }
	inline std::vector<PTNGOutputSocket> getOutputs() const { return output_sockets; }
	inline std::set<PTNGNode*> getDependencies() const { return dependencies; }
	inline std::set<PTNGNode*> getRecipients() const { return recipients; }
	std::pair<PTNGNode*, std::string> getConnectionState(std::string input_socket_name) const;
	std::pair<PTNGSocketType, PTNGValue> getOutputValue(std::string output_socket_name) const;

	~PTNGNode();
};

class PTNGGraph
{
private:
	std::set<PTNGNode*> all_nodes;
	PTNGNode* output_node;

public:
	std::vector<PTNGNode*> computeRequiredExecutionOrder();

	std::string serialise() const;
	void deserialise(std::string serialised);
};