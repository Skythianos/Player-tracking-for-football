#pragma once

#include "Globals.h"
#include "Panorama.h"

class ImageObject
{
public:
	ImageObject(int Id, int TeamId, Rect BoundingBox, Point2d ActualPosition, double RealHeight, Mat ObjectColorImage, Mat ObjectMaskImage);
	~ImageObject();
	vector<Point2d> GetPositions();
	vector<Rect> GetBoundingBoxes();
	vector<double> GetRealHeights();
	Mat GetObjectColorImage();
	Mat GetObjectMaskImage();
	int GetTeamId();
	int GetId();

	PlayerTrackProperties TrackStatus;

	virtual PlayerTrackProperties Track(Mat &ColorImage, Mat &BgFgImage, vector<ImageObject*> CurrentObjects, Panorama::Offset CenterOffset, Mat HomograpgyMatrixCenter) = 0;

protected:
	int Id;
	int TeamId;
	Mat ObjectColorImage;
	Mat ObjectMaskImage;
	double AverageRealHeight;
	vector<double> RealHeights;
	vector<Rect> BoundingBoxes;
	vector<Point2d> Positions;
	vector<double> Velocities;
	vector<double> Accelerations;
};

