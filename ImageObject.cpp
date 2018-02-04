#include "ImageObject.h"



ImageObject::ImageObject(int Id, int TeamId, Rect BoundingBox, Point2d ActualPosition, double RealHeight, Mat ObjectColorImage, Mat ObjectMaskImage)
{
	this->Id = Id;
	this->TeamId = TeamId;
	BoundingBoxes.insert(BoundingBoxes.begin(), BoundingBox);
	Positions.insert(Positions.begin(), ActualPosition);
	this->ObjectColorImage = ObjectColorImage;
	this->ObjectMaskImage = ObjectMaskImage;
	this->RealHeights.insert(RealHeights.begin(), RealHeight);
}


ImageObject::~ImageObject()
{
}

vector<Point2d> ImageObject::GetPositions()
{
	return Positions;
}

vector<Rect> ImageObject::GetBoundingBoxes()
{
	return BoundingBoxes;
}

Mat ImageObject::GetObjectColorImage()
{
	return ObjectColorImage;
}

Mat ImageObject::GetObjectMaskImage()
{
	return ObjectMaskImage;
}

int ImageObject::GetTeamId()
{
	return TeamId;
}

int ImageObject::GetId()
{
	return Id;
}

vector<double> ImageObject::GetRealHeights()
{
	return RealHeights;
}
