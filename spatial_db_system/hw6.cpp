// hw6.cpp : 定义控制台应用程序的入口点。
//

#include "common.h"
#include "Geometry.h"
#include "shapelib/shapefil.h"

#include "gl/freeglut.h"       // Glut库头文件

#include <iostream>
#include <vector>
#include <cstdio> 
#include <ctime>
#include <map>
#include <list>
using namespace std;

extern void test(int t);
extern void QuadTreeAnalysis();

int screenWidth = 640;
int screenHeight = 480;

double pointSize = 2.0;

int mode;
int treeMode = QUADTREE;

vector<hw6::Feature> features;
vector<hw6::Feature> roads;
bool showRoad = true;

hw6::QuadTree pointQTree;
hw6::QuadTree roadQTree;
hw6::RTree pointRTree;
hw6::RTree roadRTree;

bool showTree = false;

hw6::Feature nearestFeature;
hw6::Feature deleteFeature;
hw6::Feature insertFeature;
hw6::Feature drawInsert;
hw6::Feature nearestRoad; 
int newPoint = 0;
int newRoad = 0;

bool firstPoint = true;
bool startFlag = false;
bool endFlag = false;
vector<hw6::Point> roadPoints;
hw6::Point corner[2];
hw6::Envelope selectedRect;
vector<hw6::Feature> selectedFeatures;
vector<hw6::Feature> selectedPoints; 

/*
 * shapefile文件中name和geometry属性读取
 */
vector<string> readName(const char* filename)
{
	DBFHandle file = DBFOpen(filename, "r");

	vector<string> res;
	int cct = DBFGetRecordCount(file);
	res.reserve(cct);
	for (int i = 0; i < cct; ++i) {
		string a = DBFReadStringAttribute(file, i, 0);
		res.push_back(a);
	}

	DBFClose(file);

	return res;
}

vector<hw6::Geometry*> readGeom(const char* filename)
{
	SHPHandle file = SHPOpen(filename, "r");

	int pnEntities, pnShapeType;
	double padfMinBound[4], padfMaxBound[4];
	SHPGetInfo(file, &pnEntities, &pnShapeType, padfMinBound, padfMaxBound);

	vector<hw6::Point> points;
	vector<hw6::Geometry*> geoms;
	geoms.reserve(pnEntities);
	switch (pnShapeType) {
	case SHPT_POINT:
		for (int i = 0; i < pnEntities; ++i) {
			SHPObject* pt = SHPReadObject(file, i);
			geoms.push_back(new hw6::Point(pt->padfY[0], pt->padfX[0]));
			SHPDestroyObject(pt);
		}
		break;

	case SHPT_ARC:
		for (int i = 0; i < pnEntities; ++i) {
			points.clear();
			SHPObject* pt = SHPReadObject(file, i);
			for (int j = 0; j < pt->nVertices; ++j) {
				points.push_back(hw6::Point(pt->padfY[j], pt->padfX[j]));
			}
			SHPDestroyObject(pt);
			geoms.push_back(new hw6::LineString(points));
		}
		break;

	case SHPT_POLYGON:
		for (int i = 0; i < pnEntities; ++i) {
			points.clear();
			SHPObject* pt = SHPReadObject(file, i);
			for (int j = 0; j < pt->nVertices; ++j) {
				points.push_back(hw6::Point(pt->padfY[j], pt->padfX[j]));
			}
			SHPDestroyObject(pt);
			hw6::LineString line(points);
			hw6::Polygon* poly = new hw6::Polygon(line);
			geoms.push_back(new hw6::Polygon(line));
		}
		break;
	}

	SHPClose(file);
	return geoms;
}

/*
 * 输出几何信息
 */
void printGeom(vector<hw6::Geometry*>& geom)
{
	cout << "Geometry:" << endl;
	for (vector<hw6::Geometry*>::iterator it = geom.begin(); it != geom.end(); ++it) {
		(*it)->print();
	}
}

/*
 * 删除几何信息
 */
void deleteGeom(vector<hw6::Geometry*>& geom)
{
	for (vector<hw6::Geometry*>::iterator it = geom.begin(); it != geom.end(); ++it) {
		delete* it;
		*it = NULL;
	}
	geom.clear();
}

/*
 * 读取纽约道路数据
 */
void loadRoadData()
{
	vector<hw6::Geometry*> geom = readGeom(".//data/highway");

	roads.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		roads.push_back(hw6::Feature(to_string(i), geom[i]));

	cout << "road number: " << geom.size() << endl;
	roadQTree.setCapacity(20);
	roadQTree.constructQuadTree(roads);
	roadRTree.constructRTree(roads);
}


/*
 * 读取纽约自行车租赁点数据
 */
void loadStationData()
{
	vector<hw6::Geometry*> geom;

	geom = readGeom(".//data/station");
	vector<string> name = readName(".//data/station");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "station number: " << geom.size() << endl;
	pointQTree.setCapacity(5);
	pointQTree.constructQuadTree(features);
	pointRTree.constructRTree(features);
}

/*
 * 读取纽约出租车打车点数据
 */
void loadTaxiData()
{
	vector<hw6::Geometry*> geom = readGeom(".//data/taxi");
	vector<string> name = readName(".//data/taxi");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "taxi number: " << geom.size() << endl;
	pointQTree.setCapacity(100);
	pointQTree.constructQuadTree(features);
	pointRTree.constructRTree(features);
}

/*
 * 区域查询
 */
void rangeQuery()
{
	vector<hw6::Feature> candidateFeatures;

	// filter step (使用四叉树获得查询区域和几何特征包围盒相交的候选集）
	if (treeMode == QUADTREE)
	{
		if (mode == RANGEPOINT)
			pointQTree.rangeQuery(selectedRect, candidateFeatures);
		else if (mode == RANGELINE)
			roadQTree.rangeQuery(selectedRect, candidateFeatures);
	}
	else if (treeMode == RTREE)
	{
		if (mode == RANGEPOINT)
			pointRTree.rangeQuery(selectedRect, candidateFeatures);
		else if (mode == RANGELINE)
			roadRTree.rangeQuery(selectedRect, candidateFeatures);
	}
	

	// refine step (精确判断时，需要去重，避免查询区域和几何对象的重复计算)
	// write your here to update selectedFeatures
	for (int i = 0; i < candidateFeatures.size(); ++i)
	{
		if (candidateFeatures[i].getGeom()->intersects(selectedRect))
		{
			selectedFeatures.push_back(candidateFeatures[i]);
		}
	}
}

/*
 * 邻近查询
 */
void NNQuery(hw6::Point p)
{
	vector<hw6::Feature> candidateFeatures;

	// filter step (使用四叉树获得距离较近的几何特征候选集)
	if (treeMode == QUADTREE)
	{
		if (mode == NNPOINT || mode == DELETEP)
			pointQTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
		else if (mode == NNLINE || mode == DELETER || mode == SJOIN)
			roadQTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
	}
	else if (treeMode == RTREE)
	{
		if (mode == NNPOINT)
			pointRTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
		else if (mode == NNLINE || mode == SJOIN)
			roadRTree.NNQuery(p.getX(), p.getY(), candidateFeatures);
	}
	

	// refine step (精确计算查询点与几何对象的距离)
	// write your here to update nearestFeature
	if (candidateFeatures.size() > 0)
	{
		double dist;
		double minDist = candidateFeatures.front().getGeom()->distance(&p);
		nearestFeature = candidateFeatures.front();
		for (int i = 1; i < candidateFeatures.size(); ++i)
		{
			dist = candidateFeatures[i].getGeom()->distance(&p);
			if (dist <= minDist) {
				nearestFeature = candidateFeatures[i];
				minDist = dist;
			}
		}
		if (mode == DELETEP || mode == DELETER)
			deleteFeature = nearestFeature;
		if (mode == SJOIN)
			nearestRoad = nearestFeature;
	}
}

/*
 * 空间连接
 */
void spatialJoin()
{
	vector<hw6::Feature> candidateFeatures;
	// filter step 
	if (treeMode == QUADTREE)
	{
		pointQTree.spatialJoin(nearestRoad, candidateFeatures);
	}
	else if (treeMode == RTREE)
	{
		pointRTree.spatialJoin(nearestRoad, candidateFeatures);
	}
	// refine step 
	for (int i = 0; i < candidateFeatures.size(); ++i)
	{
		double distance;
		//计算当前的candidateFeature[i]与nearestRoad的距离
		const hw6:: Geometry* g = candidateFeatures[i].getGeom();
		const hw6::Geometry* g1 = nearestRoad.getGeom();
		distance = candidateFeatures[i].getGeom()->distance(dynamic_cast<hw6::LineString*>(nearestRoad.getGeom()));
		if (distance < 0.00075)
		{
			selectedPoints.push_back(candidateFeatures[i]);
		}
	}
}

void deleteOneFeature()
{
	if (mode == DELETEP)
	{
		for (vector<hw6::Feature>::iterator iter = features.begin(); iter != features.end(); iter++) {
			if (*iter == deleteFeature) {
				features.erase(iter);
				break;
			}
		}
	}
	else if (mode == DELETER)
	{
		for (vector<hw6::Feature>::iterator iter = roads.begin(); iter != roads.end(); iter++) {
			if (*iter == deleteFeature) {
				roads.erase(iter);
				break;
			}
		}
	}
}

void insertOneFeature()
{
	if (mode == INSERTP)
	{
		features.push_back(insertFeature);
	}
	else if (mode == INSERTR)
	{
		roads.push_back(insertFeature);
	}
}

void updateQuadTree()
{
	if (treeMode == QUADTREE)
	{
		if (mode == INSERTP)
			pointQTree.insertPoint(insertFeature);
		else if (mode == INSERTR)
			roadQTree.insertRoad(insertFeature);
		else if (mode == DELETEP)
			pointQTree.deletePoint(deleteFeature);
		else if(mode==DELETER)
			roadQTree.deleteRoad(deleteFeature);
	}
}

/*
 * 从屏幕坐标转换到地理坐标
 */
void transfromPt(hw6::Point& pt)
{
	const hw6::Envelope bbox = hw6::Envelope(-74.125, -73.855, 40.635, 40.815);
	double width = bbox.getMaxX() - bbox.getMinX() + 0.002;
	double height = bbox.getMaxY() - bbox.getMinY() + 0.002;

	double x = pt.getX() * width / screenWidth + bbox.getMinX() - 0.001;
	double y = pt.getY() * height / screenHeight + bbox.getMinY() - 0.001;

	x = max(bbox.getMinX(), x);
	x = min(bbox.getMaxX(), x);
	y = max(bbox.getMinY(), y);
	y = min(bbox.getMaxY(), y);
	pt = hw6::Point(x, y);
}

/*
 * 绘制代码
 */
void display()
{
	//glClearColor(241 / 255.0, 238 / 255.0, 232 / 255.0, 0.0); 
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//const hw6::Envelope bbox = pointQTree.getEnvelope();
	const hw6::Envelope bbox = hw6::Envelope(-74.125, -73.855, 40.635, 40.815);
	gluOrtho2D(bbox.getMinX() - 0.001, bbox.getMaxX() + 0.001, bbox.getMinY() - 0.001, bbox.getMaxY() + 0.001);

	// 道路绘制
	if (showRoad) {
		glColor3d(252 / 255.0, 214 / 255.0, 164 / 255.0);
		for (size_t i = 0; i < roads.size(); ++i)
			roads[i].draw();
	}

	// 点绘制
	if (!(mode == RANGELINE || mode == NNLINE)) {
		glPointSize((float)pointSize);
		glColor3d(0.0, 146 / 255.0, 247 / 255.0);
		for (size_t i = 0; i < features.size(); ++i)
			features[i].draw();

	}

	// 树绘制
	if (showTree) {
		glColor3d(0.0, 146 / 255.0, 247 / 255.0);
		if (treeMode == QUADTREE)
		{
			if (mode == RANGELINE || mode == NNLINE || mode == DELETER || mode == INSERTR)
				roadQTree.draw();
			else
				pointQTree.draw();
		}
		else if (treeMode == RTREE)
		{
			if (mode == RANGELINE || mode == NNLINE)
				roadRTree.draw();
			else 
				pointRTree.draw();
		}
	}

	// 离鼠标最近点绘制
	if (mode == NNPOINT) {
		glPointSize(5.0);
		glColor3d(0.9, 0.0, 0.0);
		nearestFeature.draw();
	}

	// 离鼠标最近道路绘制
	if (mode == NNLINE) {
		glLineWidth(3.0);
		glColor3d(0.9, 0.0, 0.0);
		nearestFeature.draw();
		glLineWidth(1.0);
	}

	// 区域选择绘制
	if (mode == RANGEPOINT || mode == RANGELINE) {
		glColor3d(0.0, 0.0, 0.0);
		selectedRect.draw();
		glColor3d(1.0, 0.0, 0.0);
		for (size_t i = 0; i < selectedFeatures.size(); ++i)
			selectedFeatures[i].draw();
	}

	// 删除要素绘制
	if (mode == DELETEP || mode == DELETER) {
		insertFeature.clear();
		glPointSize(5.0);
		glColor3d(0.9, 0.0, 0.0);
		deleteFeature.draw();
		if (!deleteFeature.isEmpty())
		{
			deleteFeature.clear();
		}
	}

	//插入要素绘制
	if (mode == INSERTP || mode == INSERTR) {
		glPointSize(5.0);
		glColor3d(0.9, 0.0, 0.0);
		if (!drawInsert.isEmpty())
			drawInsert.draw();
		if (!insertFeature.isEmpty())
		{
			insertFeature.draw();
			endFlag = false;
		}
	}

	//空间连接绘制
	if (mode == SJOIN) {
		glLineWidth(2.0);
		glColor3d(119 / 255.0, 136 / 255.0, 153 / 255.0);
		nearestRoad.draw();
		glLineWidth(1.0);
		glPointSize(3.0);
		glColor3d(1.0, 0.0, 0.0);
		for (size_t i = 0; i < selectedPoints.size(); ++i)	
			selectedPoints[i].draw();
	}

	glFlush();
	glutSwapBuffers();
}

/*
 * 鼠标和键盘交互
 */
void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (mode == RANGEPOINT || mode == RANGELINE) {
			if (firstPoint) {
				selectedFeatures.clear();
				corner[0] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[0]);
			}
			else {
				corner[1] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[1]);
				selectedRect = hw6::Envelope(min(corner[0].getX(), corner[1].getX()), max(corner[0].getX(), corner[1].getX()),
					min(corner[0].getY(), corner[1].getY()), max(corner[0].getY(), corner[1].getY()));
				rangeQuery();
			}
			firstPoint = !firstPoint;
			glutPostRedisplay();
		}
		if (mode == INSERTP && treeMode == QUADTREE)
		{
			insertFeature.clear();
			drawInsert.clear();
			hw6::Point p(x, screenHeight - y);
			transfromPt(p);
			insertFeature = hw6::Feature("NewPoint" + to_string(++newPoint), new hw6::Point(p.getX(), p.getY()));
			insertOneFeature();
			updateQuadTree();
			glutPostRedisplay();
		}
		if (mode == INSERTR && treeMode == QUADTREE)
		{
			if (startFlag == false)
			{
				startFlag = true;
				roadPoints.clear();
				insertFeature.clear();
			}
			hw6::Point p(x, screenHeight - y);
			transfromPt(p);
			if (!endFlag)
			{
				roadPoints.push_back(p);
				drawInsert = hw6::Feature("drawInsert", new hw6::LineString(roadPoints));
			}
			glutPostRedisplay();
		}
		if (mode == SJOIN) 
		{
			selectedPoints.clear();
			hw6::Point p(x, screenHeight - y);
			transfromPt(p);
			NNQuery(p);
			spatialJoin();
			glutPostRedisplay();
		}
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		if (mode == DELETEP && treeMode == QUADTREE)
		{
			hw6::Point p(x, screenHeight - y);
			transfromPt(p);
			NNQuery(p);
			deleteOneFeature();
			updateQuadTree();
			glutPostRedisplay();
		}
		else if (mode == DELETER && treeMode == QUADTREE)
		{
			hw6::Point p(x, screenHeight - y);
			transfromPt(p);
			NNQuery(p);
			deleteOneFeature();
			updateQuadTree();
			glutPostRedisplay();
		}
		else if (mode == INSERTR && treeMode == QUADTREE)
		{
			endFlag = true;
			startFlag = false;
			insertFeature = hw6::Feature("NewRoad" + to_string(++newRoad), new hw6::LineString(roadPoints));
			insertOneFeature();
			updateQuadTree();
			glutPostRedisplay();
		}
	}
}

void passiveMotion(int x, int y)
{
	corner[1] = hw6::Point(x, screenHeight - y);

	if ((mode == RANGEPOINT || mode == RANGELINE) && !firstPoint) {
		selectedFeatures.clear();
		corner[1] = hw6::Point(x, screenHeight - y);
		transfromPt(corner[1]);
		selectedRect = hw6::Envelope(min(corner[0].getX(), corner[1].getX()), max(corner[0].getX(), corner[1].getX()),
			min(corner[0].getY(), corner[1].getY()), max(corner[0].getY(), corner[1].getY()));
		rangeQuery();

		glutPostRedisplay();
	}
	else if (mode == NNPOINT || mode == NNLINE) {
		selectedFeatures.clear();
		hw6::Point p(x, screenHeight - y);
		transfromPt(p);
		NNQuery(p);

		glutPostRedisplay();
	}
}

void changeSize(int w, int h)
{
	screenWidth = w;
	screenHeight = h;
	glViewport(0, 0, w, h);
	glutPostRedisplay();
}

void processNormalKeys(unsigned char key, int x, int y)
{
	switch (key) {
	case 27:exit(0); break;
	case 'N':
		mode = NNLINE; break;
	case 'n':
		mode = NNPOINT; break;
	case 'S':
		mode = RANGELINE;
		firstPoint = true;
		break;
	case 's':
		mode = RANGEPOINT;
		firstPoint = true;
		break;
	case 'J':
	case 'j':
		mode = SJOIN;
		break;
	case 'B':
	case 'b':
		loadStationData();
		mode = Default;
		break;
	case 'T':
	case 't':
		loadTaxiData();
		mode = Default;
		break;
	case 'R':
	case 'r':
		showRoad = !showRoad;
		break;
	case 'Q':
	case 'q':
		showTree = !showTree;
		break;
	case 'X':
	case 'x':
		if (treeMode == QUADTREE)
			treeMode = RTREE;
		else if (treeMode == RTREE)
			treeMode = QUADTREE;
		break;
	case 'D':
		mode = DELETER;
		break;
	case 'd':
		mode = DELETEP;
		break;
	case 'I':
		mode = INSERTR;
		break;
	case 'i':
		mode = INSERTP;
		break;
	case '+':
		pointSize *= 1.1;
		break;
	case '-':
		pointSize /= 1.1;
		break;
	case '1':
		test(TEST1); break;
	case '2':
		test(TEST2); break;
	case '3':
		test(TEST3); break;
	case '4':
		test(TEST4); break;
	case '5':
		test(TEST5); break;
	case '6':
		test(TEST6); break;
	case '7':
		test(TEST7); break;
	case '8':
		test(TEST8); break;
	default:
		mode = Default; break;
	}
	glutPostRedisplay();
}

int main(int argc, char* argv[])
{
	cout << "Key Usage:\n"
		<< "  S  : range search for roads\n"
		<< "  s  : range search for stations\n"
		<< "  N  : nearest road search\n"
		<< "  n  : nearest station search\n"
		<< "  J/j: spatial join\n"
		<< "  B/b: Bicycle data\n"
		<< "  T/t: Taxi data\n"
		<< "  R/r: show Road\n"
		<< "  Q/q: show Tree\n"
		<< "  X/x: transfrom Tree\n"
		<< "  D  : delete a road\n"
		<< "  d  : delete a station\n"
		<< "  I  : insert a road\n"
		<< "  i  : insert a station\n"
		<< "  +  : increase point size\n"
		<< "  -  : decrease point size\n"
		<< "  1  : Test Envelope contain, intersect and union\n"
		<< "  2  : Test distance between Point and LineString\n"
		<< "  3  : Test distance between Point and Polygon\n"
		<< "  4  : Test quadtree construction\n"
		<< "  5  : Test distance between LineString and LineString\n"
		<< "  6  : Test distance between LineString and Polygon\n"
		<< "  7  : Test (your option here)\n"
		<< "  8  : Quadtree performance analysis\n"
		<< "  ESC: quit\n"
		<< endl;

	loadRoadData();
	loadStationData();

	glutInit(&argc, argv);
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition(550, 150);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("New York");

	glutMouseFunc(mouse);
	glutDisplayFunc(display);
	glutPassiveMotionFunc(passiveMotion);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(processNormalKeys);

	glutMainLoop();

	return 0;
}

