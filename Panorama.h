#pragma once

#include "Globals.h"

class Panorama
{
public:
	Panorama();
	~Panorama();

	struct TransformedFrameBoarders {
		int minrow;
		int maxrow;
		int mincol;
		int maxcol;
	};

	struct Offset {
		int cols;
		int rows;
	};

	 struct HomographyMatrix {
		int Calibrated;
		Mat H;
	};

	HomographyMatrix HomographyMatrixLeft;
	HomographyMatrix HomographyMatrixCenter;
	HomographyMatrix HomographyMatrixRight;

	Mat PerspectiveTransformMatrixLeft;
	Mat PerspectiveTransformMatrixRight;

	Size PanoramaSize;
	Size TransformedLeftSize;
	Size TransformedRightSize;
	Offset CenterOffset;

	vector<Point2f> ImagePointsLeft;
	vector<Point2f> ImagePointsCenter;
	vector<Point2f> ImagePointsRight;

	vector<Point2f> ObjectPointsLeft;
	vector<Point2f> ObjectPointsCenter;
	vector<Point2f> ObjectPointsRight;

	void Calibrate(vector<CoordWithWindow> ImagePoints, vector<CoordWithWindow> ObjectPoints);

	TransformedFrameBoarders FindBoarders(Mat &src, vector<Point2f> &ImagePoints, Mat &SourceHomographyMatrix, const Mat &DestinationHomographyMatrix);

	Mat GetPerspectiveTransformMatrix(Mat &src, vector<Point2f> &ImagePoints, Mat &SourceHomographyMatrix, const Mat &DestinationHomographyMatrix, Offset &offset);

	bool CreatePanorama(Mat &left, Mat &center, Mat &right);

	bool GetPanorama(Mat &left, Mat &center, Mat &right, Mat &panorama);

	Mat TransformPoint(Point2d Point, const Mat &HomographyMatrix);

	bool ImportFromFile(FileStorage File);
};

