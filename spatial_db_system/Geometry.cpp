#include "Geometry.h"
#include <cmath>
#include <climits>
#include "gl/freeglut.h"

#define NOT_IMPLEMENT -1.0

namespace hw6 {

/*
 * Envelope functions
 */
bool Envelope::contain(double x, double y) const
{
	return x >= minX && x <= maxX && y >= minY && y <= maxY;
}

bool Envelope::contain(const Envelope& envelope) const
{
	// Task 测试Envelope是否包含关系
	// Write your code here
	bool containFlag = contain(envelope.getMinX(), envelope.getMinY()) && contain(envelope.getMinX(), envelope.getMaxY())
		&& contain(envelope.getMaxX(), envelope.getMinY()) && contain(envelope.getMaxX(), envelope.getMaxY());
	return containFlag;
}

bool Envelope::intersect(const Envelope& envelope) const
{
	// Task 测试Envelope是否相交
	// Write your code here
	bool disjointFlag = (envelope.getMaxX() < minX) || (envelope.getMinX() > maxX)
		|| (envelope.getMaxY() < minY) || (envelope.getMinY() > maxY);
	return !disjointFlag;
}

Envelope Envelope::unionEnvelope(const Envelope& envelope) const
{
	// Task 合并两个Envelope生成一个新的Envelope
	// Write your code here
	Envelope e(min(minX, envelope.getMinX()), max(maxX, envelope.getMaxX())
		, min(minY, envelope.getMinY()), max(maxY, envelope.getMaxY()));
	return e;
}

void Envelope::draw() const
{
	glBegin(GL_LINE_STRIP);

	glVertex2d(minX, minY);
	glVertex2d(minX, maxY);
	glVertex2d(maxX, maxY);
	glVertex2d(maxX, minY);
	glVertex2d(minX, minY);

	glEnd();
}


/*
 * Points functions
 */
double Point::distance(const Point* point) const
{
	return sqrt((x - point->x) * (x - point->x) + (y - point->y) * (y - point->y));
}

double Point::distance(const LineString* line) const
{
	double mindist = line->getPointN(0).distance(this);
	for (size_t i = 0; i < line->numPoints() - 1; ++i) {
		double dist = 0;
		double x1 = line->getPointN(i).getX();
		double y1 = line->getPointN(i).getY();
		double x2 = line->getPointN(i + 1).getX();
		double y2 = line->getPointN(i + 1).getY();
		// Task calculate the distance between Point P(x, y) and Line [P1(x1, y1), P2(x2, y2)] (less than 10 lines)
		// Write your code here
		double dot1 = (x - x1) * (x1 - x2) + (y - y1) * (y1 - y2);
		double dot2 = (x - x2) * (x1 - x2) + (y - y2) * (y1 - y2);
		if (dot1 * dot2 >= 0)
		{
			dist = min(sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1)), sqrt((x - x2) * (x - x2) + (y - y2) * (y - y2)));
		}
		else
		{
			double projection = dot1 / sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			dist = sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1) - pow(projection, 2));
		}
		if (dist < mindist)
			mindist = dist;
	}
	return mindist;
}

bool IsInPolygon(const LineString *line, const Point *point)
{
	size_t n = line->numPoints();
	double x = point->getX();
	double y = point->getY();
	bool inPolygon = false;
	for (size_t i = 0; i < n - 1; i++)
	{
		double x1 = line->getPointN(i).getX();
		double y1 = line->getPointN(i).getY();
		double x2 = line->getPointN(i + 1).getX();
		double y2 = line->getPointN(i + 1).getY();
		if (y1 > y2)
		{
			swap(x1, x2);
			swap(y1, y2);
		}
		if (y <= y1 && y<y2 || y >= y1 && y>y2) continue;
		double slope = (x2 - x1) / (y2 - y1);
		double intersectX = (y - y1) * slope + x1;
		if (intersectX > x) inPolygon = !inPolygon;
	}
	return inPolygon;
}

double Point::distance(const Polygon* polygon) const
{
	LineString line = polygon->getExteriorRing();
	double mindist = 0;
	
	//在外环外，计算与外环最近距离
	if (!IsInPolygon(&line, this))
		return this->distance(&line);	
	//在外环内
	//没有内环，即在多边形内部
	vector<LineString> interior = polygon->getInteriorRing();
	if (interior.empty())		
		return 0;

	//有内环
	for (int i = 0; i < interior.size(); i++)
	{
		line = interior[i];
		//在内环内部，计算与该内环最近距离
		if (IsInPolygon(&line, this))
			return this->distance(&line);
	}

	//在外环内，内环外，即在多边形内部
	return 0;
}

bool Point::intersects(const Envelope& rect)  const
{
	return (x >= rect.getMinX()) && (x <= rect.getMaxX()) && (y >= rect.getMinY()) && (y <= rect.getMaxY());
}

void Point::draw()  const
{
	glBegin(GL_POINTS);
	glVertex2d(x, y);
	glEnd();
}


/*
 * LineString functions
 */
void LineString::constructEnvelope()
{
	double minX, minY, maxX, maxY;
	maxX = minX = points[0].getX();
	maxY = minY = points[0].getY();
	for (size_t i = 1; i < points.size(); ++i) {
		maxX = max(maxX, points[i].getX());
		maxY = max(maxY, points[i].getY());
		minX = min(minX, points[i].getX());
		minY = min(minY, points[i].getY());
	}
	envelope = Envelope(minX, maxX, minY, maxY);
}

//叉乘，大于0：顺时针，小于零：逆时针，等于0：共线
double Cross(const Point *p, const Point* a, const Point* b)
{
	double abX = b->getX() - a->getX();
	double abY = b->getY() - a->getY();
	double apX = p->getX() - a->getX();
	double apY = p->getY() - a->getY();
	return abX * apY - abY * apX;
}

bool SegmentHaveIntersection(const Point* x, const Point* y, const Point* a, const Point* b)
{
	//叉乘异号，点在异侧，有交点
	if (Cross(x, a, b) * Cross(y, a, b) < 0 && Cross(a, x, y) * Cross(b, x, y) < 0)
		return true;
	//叉乘同号，点在同侧，没有交点
	if (Cross(x, a, b) * Cross(y, a, b) > 0 || Cross(a, x, y) * Cross(b, x, y) > 0)
		return false;
	//叉乘为0，共线
	if (Cross(x, a, b) == 0 && Cross(y, a, b) == 0)
	{
		if (x->getX() > max(a->getX(), b->getX()) && y->getX() > max(a->getX(), b->getX()))
			return false;
		if (x->getX() < min(a->getX(), b->getX()) && y->getX() < min(a->getX(), b->getX()))
			return false;
		return true;
	}
	return true;
}

bool LineHaveIntersection(const LineString *line1, const LineString *line2)
{
	size_t n1 = line1->numPoints();
	size_t n2 = line2->numPoints();
	for (size_t i = 0; i < n1 - 1; i++)
	{
		Point x = line1->getPointN(i);
		Point y = line1->getPointN(i + 1);
		for (size_t j = 0; j < n2 - 1; j++)
		{
			Point a = line2->getPointN(j);
			Point b = line2->getPointN(j + 1);
			//存在交点
			if (SegmentHaveIntersection(&x, &y, &a, &b))
				return true;
		}
	}
	return false;
}

double LineString::distance(const LineString* line) const
{
	size_t n1 = line->numPoints();
	size_t n2 = this->numPoints();
	double minDist = DBL_MAX;

	//相交
	if (LineHaveIntersection(line, this))
		return 0;

	//不相交
	for (size_t i = 0; i < n2; i++)
	{
		Point x = this->getPointN(i);
		double dist = x.distance(line);
		if (dist < minDist)
			minDist = dist;
	}
	for (size_t i = 0; i < n1; i++)
	{
		Point x = line->getPointN(i);
		double dist = x.distance(this);
		if (dist < minDist)
			minDist = dist;
	}
	return minDist;
}

double LineString::distance(const Polygon* polygon) const
{
	//判断line和多边形是否相交，相交dist为0
	//与外环相交
	LineString exterior = polygon->getExteriorRing();
	if (LineHaveIntersection(this, &exterior))
		return 0;
	//与内环相交
	if (!polygon->getInteriorRing().empty())
	{
		vector<LineString> interior = polygon->getInteriorRing();
		for (int i = 0; i < interior.size(); i++)
		{
			if (LineHaveIntersection(this, &interior[i]))
				return 0;
		}
	}

	//不相交
	Point point = this->getStartPoint();
	//在外环外,最近距离为line和外环的最近距离
	if (!IsInPolygon(&exterior, &point))
		return this->distance(&exterior);
	//在外环内
	//无内环，距离为0
	if (polygon->getInteriorRing().empty())
		return 0;
	//有内环
	//在内环内，最近距离为line和内环的最近距离
	vector<LineString> interior = polygon->getInteriorRing();
	for (int i = 0; i < interior.size(); i++)
	{
		if (IsInPolygon(&interior[i], &point))
			return this->distance(&interior[i]);
	}
	//在内环外
	return 0;
}

typedef int OutCode;

const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)
// ASSUME THAT xmax, xmin, ymax and ymin are global constants.
OutCode ComputeOutCode(double x, double y, double xmin, double xmax, double ymin, double ymax)
{
	OutCode code;

	code = INSIDE;          // initialised as being inside of [[clip window]]

	if (x < xmin)           // to the left of clip window
		code |= LEFT;
	else if (x > xmax)      // to the right of clip window
		code |= RIGHT;
	if (y < ymin)           // below the clip window
		code |= BOTTOM;
	else if (y > ymax)      // above the clip window
		code |= TOP;
	
	return code;
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
// diagonal from (xmin, ymin) to (xmax, ymax).
bool intersectTest(double x0, double y0, double x1, double y1, double xmin, double xmax, double ymin, double ymax)
{
	// compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
	OutCode outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
	OutCode outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
	bool accept = false;

	while (true) {
		if (!(outcode0 | outcode1)) {
			// bitwise OR is 0: both points inside window; trivially accept and exit loop
			accept = true;
			break;
		}
		else if (outcode0 & outcode1) {
			// bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
			// or BOTTOM), so both must be outside window; exit loop (accept is false)
			break;
		}
		else {
			// failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge
			double x, y;

			// At least one endpoint is outside the clip rectangle; pick it.
			OutCode outcodeOut = outcode0 ? outcode0 : outcode1;

			// Now find the intersection point;
			// use formulas:
			//   slope = (y1 - y0) / (x1 - x0)
			//   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
			//   y = y0 + slope * (xm - x0), where xm is xmin or xmax
			// No need to worry about divide-by-zero because, in each case, the
			// outcode bit being tested guarantees the denominator is non-zero
			if (outcodeOut & TOP) {           // point is above the clip window
				x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
				y = ymax;
			}
			else if (outcodeOut & BOTTOM) { // point is below the clip window
				x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
				y = ymin;
			}
			else if (outcodeOut & RIGHT) {  // point is to the right of clip window
				y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
				x = xmax;
			}
			else if (outcodeOut & LEFT) {   // point is to the left of clip window
				y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
				x = xmin;
			}
			
			// Now we move outside point to intersection point to clip
			// and get ready for next pass.
			if (outcodeOut == outcode0) {
				x0 = x;
				y0 = y;
				outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
			}
			else {
				x1 = x;
				y1 = y;
				outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
			}
		}
	}
	return accept;
}

bool LineString::intersects(const Envelope& rect)  const
{
	double xmin = rect.getMinX();
	double xmax = rect.getMaxX();
	double ymin = rect.getMinY();
	double ymax = rect.getMaxY();

	for (size_t i = 1; i < points.size(); ++i)
		if (intersectTest(points[i - 1].getX(), points[i - 1].getY(), points[i].getX(), points[i].getY(), xmin, xmax, ymin, ymax))
			return true;
	return false;
}

void LineString::draw()  const
{
	if (points.size() > 1)
	{
		glBegin(GL_LINE_STRIP);
		for (size_t i = 0; i < points.size(); ++i)
			glVertex2d(points[i].getX(), points[i].getY());
		glEnd();
	}
	else
	{
		glPointSize(2.0);
		points[0].draw();
	}
}

void LineString::print() const
{
	cout << "LineString(";
	for (size_t i = 0; i < points.size(); ++i) {
		if (i != 0)
			cout << ", ";
		cout << points[i].getX() << " " << points[i].getY();
	}
	cout << ")";
}

/*
 * Polygon
 */
double Polygon::distance(const Polygon* polygon) const
{
	return min(exteriorRing.distance(polygon), polygon->getExteriorRing().distance(this));
}

void Polygon::draw() const
{
	exteriorRing.draw();
}

void Polygon::print() const
{
	cout << "Polygon(";
	for (size_t i = 0; i < exteriorRing.numPoints(); ++i) {
		if (i != 0)
			cout << ", ";
		Point p = exteriorRing.getPointN(i);
		cout << p.getX() << " " << p.getY();
	}
	cout << ")";
}
	
bool Polygon::intersects(const Envelope& rect)  const
{
	LineString exterior = this->getExteriorRing();
	//和外环相交
	if (exterior.intersects(rect))
		return true;

	//和内环相交
	if (!this->getInteriorRing().empty())
	{
		vector<LineString> interior = this->getInteriorRing();
		for (int i = 0; i < interior.size(); i++)
		{
			if (interior[i].intersects(rect))
				return true;
		}
	}

	//envelop在外环外
	Point point(rect.getMaxX(), rect.getMaxY());
	if (!IsInPolygon(&exterior, &point))
		return false;

	//envelop在内环内
	if (!this->getInteriorRing().empty())
	{
		vector<LineString> interior = this->getInteriorRing();
		for (int i = 0; i < interior.size(); i++)
		{
			if (!IsInPolygon(&interior[i], &point))
				return false;
		}
	}

	//envelop在外环内环之间
	return true;
}

}
