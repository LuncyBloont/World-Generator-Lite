#include <iostream>
#include <map>
#include "net.h"

int main() {
	NetGraph::InitHandler init = *(NetGraph::Init::createOnce());
	init("AOE2");

	NetGraph::Node s("项目开始");
	NetGraph::Node p1("完成基础算法库开发");
	NetGraph::Node p2("完成可视化模块");
	NetGraph::Node p3("完成基础模块");
	NetGraph::Node p4("软件主体完成");
	NetGraph::Node p5("项目完工");

	s >> p1 | "基本噪声算法设计";
	s >> p1 | "法线贴图生成算法设计";
	s >> p1 | "OBJ模型库开发";

	(s >> p2 | "UI开发") >> p2 | "3D渲染管线搭建";

	p1 >> p3 | "开发基本生成过程";
	p2 >> p3 | "数据到渲染处理流程开发";

	p3 >> p4 | "模块整合";

	p4 >> p5 | "完整工作流程测试";
	
	s >> p4 | "引擎内插件开发";

	init >> "test.dot";

	system("dot -Tpng ./test.dot -o ./AOE.png");

	return 0;
}
