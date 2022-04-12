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


	init("功能对象图");
	NetGraph::Node f0("地形生成");

	NetGraph::Node f1("生成地形数据");
	NetGraph::Node f2("生成植被分布");
	NetGraph::Node f3("生成河流数据");

	NetGraph::Node f4("用户提供输入");

	NetGraph::Node f5("生成网格数据");
	NetGraph::Node f6("生成法线细节");

	NetGraph::Node f7("选择植被类型");
	NetGraph::Node f8("执行位置撒点");

	NetGraph::Node f9("计算海拔分布");

	NetGraph::Node f10("PCG生成算法");

	f10 >> f5 >> f6; 
	f10 >> f7 >> f8; 
	f10 >> f9;

	f4 >> f5 >> f6; 
	f4 >> f7 >> f8; 
	f4 >> f9;

	f5 >> f1;
	f6 >> f1;

	f7 >> f2;
	f8 >> f2;

	f9 >> f3;

	f1 >> f0;
	f2 >> f0;
	f3 >> f0;

	init >> "objects.dot";

	system("dot -Tpng ./test.dot -o ./AOE.png");
	system("dot -Tpng ./objects.dot -o ./objects.png");

	return 0;
}
