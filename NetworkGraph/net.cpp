#include "net.h"

NetGraph::Init* NetGraph::Init::instance = nullptr;

void NetGraph::Init::operator()(const std::string name) {
	if (graph) { delete(graph); }
	graph = new Graph(name);
}

NetGraph::Init& NetGraph::Init::operator >>(const std::string filename) {
	NetGraph::Init::createOnce()->graph->writeFile(filename);
	return *this;
}

void NetGraph::Graph::writeFile(const std::string fname) {
	std::ofstream outf(fname);
	outf << "digraph " << name << "{\n    node [ fontname = FangSong ]\n"
		"    edge [ fontname = FangSong ]\n";
	for (const std::pair<std::string, NetGraph::Node*>& it : nodes) {
		it.second->writeStream(outf);
	}
	outf << "}\n";
}

NetGraph::Init* NetGraph::Init::createOnce() {
	if (!Init::instance) {
		instance = new Init();
	}
	return NetGraph::Init::instance;
}

NetGraph::Node::Node() {
	create("nolabel");
}

NetGraph::Node::Node(const std::string& label) {
	create(label);
}

void NetGraph::Node::create(const std::string& label) {
	this->label = label;
	NetGraph::Init::createOnce()->graph->add(this);
	graph = NetGraph::Init::createOnce()->graph;
}

void NetGraph::Graph::add(NetGraph::Node* const node) {
	if (nodes.find(node->getLabel()) != nodes.end()) {
		std::cerr << "Duplicate definition label name." << std::endl;
		std::exit(-1);
	}
	nodes.insert({ node->getLabel(), node });
}

void NetGraph::Node::link(NetGraph::Node* const node) {
	if (graph != node->graph) {
		std::cerr << "Link nodes from different graph." << std::endl;
		std::exit(-1);
	}
	NetGraph::Edge edge = { "", node };
	nodes.push_back(edge);
}

NetGraph::Node& NetGraph::Node::operator >>(NetGraph::Node& node) {
	link(&node);
	return *this;
}

NetGraph::Node& NetGraph::Node::operator <<(NetGraph::Node& node) {
	node.link(this);
	return *this;
}

void NetGraph::Node::writeStream(std::ostream& stream) const {
	for (const NetGraph::Edge& it : nodes) {
		stream << "    \"" << getLabel() << "\" -> \"" << it.task->getLabel() << "\" ";
		stream << "[ " << "label = \"" << it.label << "\" ]\n";
	}
}

NetGraph::Node& NetGraph::Node::operator |(const std::string label) {
	nodes[nodes.size() - 1].label = label;
	return *this;
}

NetGraph::Init::~Init() { 
	if (graph) { delete graph; graph = nullptr; } 
}