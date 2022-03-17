#ifndef NET_H
#define NET_H

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

namespace NetGraph {
	class Graph;
	class Node;
	class Init {
	public:
		Graph* graph = nullptr;
		void operator() (const std::string name);
		static Init* createOnce();
		Init& operator >>(const std::string filename);
		~Init();
	private:
		static Init* instance;
		Init() {}
		Init(const Init& i) {}
		Init& operator= (const Init& i);
	};

	typedef Init& InitHandler;

	class Graph {
	public:
		Graph(const std::string name): name(name) {};
		void add(NetGraph::Node* const node);
		std::string getName() const { return name; }
		void writeFile(const std::string name);
	private:
		std::string name;
		std::map<std::string, Node*> nodes;
	};

	struct Edge;

	class Node {
	public:
		Node();
		Node(const std::string& label);
		Node& operator >>(Node& node);
		Node& operator <<(Node& node);
		Node& operator |(const std::string label);
		std::string getLabel() const { return label; }
		void writeStream(std::ostream& stream) const;

	private:
		std::string label;
		Graph* graph = nullptr;
		std::vector<Edge> nodes;

		void create(const std::string& label);
		void link(NetGraph::Node* const node);
	};

	struct Edge
	{
		std::string label;
		Node* task;
	};
}

#endif // NET_H