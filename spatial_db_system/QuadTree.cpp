#include <set>
#include "QuadTree.h"

namespace hw6 {

/*
 * QuadNode
 */
void QuadNode::split(size_t capacity)
{
	for (int i = 0; i < 4; ++i) {
		delete nodes[i];
		nodes[i] = NULL;
	}
	// Task construction
	// Write your code here
	if (features.size() > capacity) {
		double midX = (bbox.getMinX() + bbox.getMaxX()) / 2.0;
		double midY = (bbox.getMinY() + bbox.getMaxY()) / 2.0;
		Envelope e0(bbox.getMinX(), midX, midY, bbox.getMaxY());
		nodes[0] = new QuadNode(e0);
		Envelope e1(midX, bbox.getMaxX(), midY, bbox.getMaxY());
		nodes[1] = new QuadNode(e1);
		Envelope e2(midX, bbox.getMaxX(), bbox.getMinY(), midY);
		nodes[2] = new QuadNode(e2);
		Envelope e3(bbox.getMinX(), midX, bbox.getMinY(), midY);
		nodes[3] = new QuadNode(e3);

		for (int i = 0; i < features.size();++i) {
			for (int j = 0; j < 4; ++j) {
				if (nodes[j]->getEnvelope().intersect(features[i].getEnvelope())) {
					nodes[j]->add(features[i]);
				}
			}
		}

		for (int i = 0; i < 4; ++i) {
			nodes[i]->split(capacity);
		}
		features.clear();
	}
}

void QuadNode::clearNodes()
{
	for (int i = 0; i < 4; ++i) {
		delete nodes[i];
		nodes[i] = NULL;
	}
}

void QuadNode::deleteFeature(Feature& f)
{
	for (vector<Feature>::iterator iter = features.begin(); iter != features.end(); iter++) {
		if (*iter == f) {
			features.erase(iter);
			break;
		}
	}
}

void QuadNode::countNode(int& interiorNum, int& leafNum)
{
	if (isLeafNode()) {
		++leafNum;
	}
	else {
		++interiorNum;
		for (int i = 0; i < 4; ++i)
			nodes[i]->countNode(interiorNum, leafNum);
	}
}

void QuadNode::countFeature(int& featureNum)
{
	if (isLeafNode()) {
		featureNum=featureNum+features.size();
		return;
	}
	else {
		for (int i = 0; i < 4; ++i)
			nodes[i]->countFeature(featureNum);
	}
}

int QuadNode::countHeight(int height)
{
	++height;
	if (!isLeafNode()) {
		int cur = height;
		for (int i = 0; i < 4; ++i) {
			height = max(height, nodes[i]->countHeight(cur));
		}
	}
	return height;
}

void QuadNode::rangeQuery(Envelope& rect, vector<Feature>& features)
{
	if (!bbox.intersect(rect))
		return;

	// Task range query
	// Write your code here
	if (isLeafNode()) {
		for (int i = 0; i < this->features.size(); ++i) {
			if (this->features[i].getEnvelope().intersect(rect)) {
				features.push_back(this->features[i]);
			}
		}
	}
	else {
		for (int i = 0; i < 4; ++i) {
			nodes[i]->rangeQuery(rect, features);
		}
	}
}

QuadNode* QuadNode::pointInLeafNode(double x, double y)
{
	// Task NN query
	// Write your code here
	if (isLeafNode()) {
		return this;
	}
	else {
		for (int i = 0; i < 4; ++i) {
			if (nodes[i]->bbox.contain(x, y)) {
				return nodes[i]->pointInLeafNode(x, y);
			}
		}
	}
	return NULL;
}

QuadNode* QuadNode::parentNode(Feature& feature)
{
	QuadNode* node;
	for (int i = 0; i < 4; ++i) {
		if (nodes[i]->bbox.intersect(feature.getEnvelope())) {
			node = nodes[i];
		}
	}
	if (node->isLeafNode()) {
		return this;
	}
	else {
		return node->parentNode(feature);
	}
}

void QuadNode::deleteAllFeature(Feature& feature, int capacity)
{
	for (int i = 0; i < 4; ++i) {
		if (nodes[i]->bbox.intersect(feature.getEnvelope())) {
			if (nodes[i]->isLeafNode())
			{
				nodes[i]->deleteFeature(feature);
			}
			else nodes[i]->deleteAllFeature(feature, capacity);
		}
	}
	if (nodes[0]->isLeafNode())
	{
		int sumNode = 0;
		countFeature(sumNode);
		if (sumNode <= capacity)
		{
			for (int i = 0; i < 4; i++)
			{
				add(getChildNode(i)->getAllFeature());
			}
			clearNodes();
		}
	}
}

void QuadNode::insertRoad(Feature& feature, int capacity)
{
	for (int i = 0; i < 4; ++i) {
		if (nodes[i]->bbox.intersect(feature.getEnvelope())) {
			if (nodes[i]->isLeafNode())
			{
				nodes[i]->add(feature);
			}
			else nodes[i]->insertRoad(feature, capacity);
		}
	}
	for (int i = 0; i < 4; ++i) {
		if (nodes[i]->isLeafNode())
		{
			if (nodes[i]->getFeatureNum() > capacity)
				nodes[i]->split(capacity);
		}
	}
}

void QuadNode::spatialJoin(Feature& road, vector<Feature>& points)
{
	if (bbox.intersect(road.getEnvelope()))
	{
		if (isLeafNode())
		{
			for (size_t i = 0; i < features.size(); i++)
				points.push_back(features[i]);
		}
		else
		{
			for (int i = 0; i < 4; i++) {
				nodes[i]->spatialJoin(road, points);
			}
		}
	}
	else return;
}

void QuadNode::draw()
{
	if (isLeafNode()) {
		bbox.draw();
	}
	else {
		for (int i = 0; i < 4; ++i)
			nodes[i]->draw();
	}
}

/*
 * QuadTree
 */
bool QuadTree::constructQuadTree(vector<Feature>& features)
{
	if (features.empty())
		return false;

	// Task construction
	// Write your code here

	//bbox = Envelope(-74.1, -73.8, 40.6, 40.8); // 注意此行代码需要更新为features的包围盒，或根节点的包围盒
	
	Envelope e = features.front().getEnvelope();
	for (int i = 1; i < features.size(); i++) {
		e = e.unionEnvelope(features[i].getEnvelope());
	}
	root = new QuadNode(e);
	root->add(features);
	root->split(capacity);

	bbox = e;
	return true;
}

void QuadTree::countQuadNode(int& interiorNum, int& leafNum)
{
	interiorNum = 0;
	leafNum = 0;
	if (root)
		root->countNode(interiorNum, leafNum);
}

void QuadTree::countHeight(int &height)
{
	height = 0;
	if (root)
		height = root->countHeight(0);
}


void QuadTree::rangeQuery(Envelope& rect, vector<Feature>& features) 
{ 
	// Task range query
	// Write your code here

	// filter step (选择查询区域与几何对象包围盒相交的几何对象)
	root->rangeQuery(rect, features);
	// 注意四叉树区域查询仅返回候选集，精炼步在hw6的rangeQuery中完成
}

bool QuadTree::NNQuery(double x, double y, vector<Feature>& features)
{
	if (!root || !(root->getEnvelope().contain(x, y)))
		return false;

	// Task NN query
	// Write your code here

	// filter step (使用maxDistance2Envelope函数，获得查询点到几何对象包围盒的最短的最大距离，然后区域查询获得候选集)

	const Envelope& envelope = root->getEnvelope();
	double minDist = max(envelope.getWidth(), envelope.getHeight());
	
	QuadNode* leaf = root->pointInLeafNode(x, y);
	for (int i = 0; i < leaf->getFeatureNum(); ++i) {
		minDist = min(minDist, leaf->getFeature(i).maxDistance2Envelope(x, y));
	}
	Envelope rect = Envelope(x - minDist, x + minDist, y - minDist, y + minDist);
	root->rangeQuery(rect, features);
	// 注意四叉树邻近查询仅返回候选集，精炼步在hw6的NNQuery中完成

	return true;
}

void QuadTree::deletePoint(Feature& feature)
{
	QuadNode* parent = root->parentNode(feature);
	QuadNode* leaf = root->pointInLeafNode(feature.getEnvelope().getMinX(), feature.getEnvelope().getMinY());
	leaf->deleteFeature(feature);
	int sumNode = 0; 
	parent->countFeature(sumNode);
	
	if (sumNode < capacity)
	{
		for (int i = 0; i < 4; i++)
		{
			parent->add(parent->getChildNode(i)->getAllFeature());
		}
		parent->clearNodes();
	}
}

void QuadTree::deleteRoad(Feature& feature)
{
	root->deleteAllFeature(feature, capacity);
}

void QuadTree::insertRoad(Feature& feature)
{
	root->insertRoad(feature, capacity);
}

void QuadTree::spatialJoin(Feature& road, vector<Feature>& points)
{
	if (!root)
		return;
	root->spatialJoin(road, points);
}

void QuadTree::insertPoint(Feature& feature)
{
	QuadNode* leaf = root->pointInLeafNode(feature.getEnvelope().getMinX(), feature.getEnvelope().getMinY());
	leaf->add(feature);
	if (leaf->getFeatureNum() > capacity) leaf->split(capacity);
}

void QuadTree::draw()
{
	if (root)
		root->draw();
}

}
