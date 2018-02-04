#pragma once

#include "Globals.h"
#include "ImageObject.h"
#include "Panorama.h"

extern int ShowDebugPictures;

class Player: public ImageObject
{
public:
	Player(int Id, int TeamId, Rect BoundingBox, Point2d ActualPosition, double RealHeight, Mat ObjectColorImage, Mat ObjectMaskImage);
	~Player();

	void SetMinPlayerHeight(double MinPlayerHeight);
	void SetMaxPlayerHeight(double MaxPlayerHeight);
	void SetGmmModel(Ptr<EM> em_model);

	Mat TransformPoint(Point2d Point, const Mat & HomographyMatrix);
	int GetHistogramMaxPosition(Mat & ColorImage, Mat & Mask);
	PlayerTrackProperties Track(Mat &ColorImage, Mat &BgFgImage, vector<ImageObject*> CurrentObjects, Panorama::Offset CenterOffset, Mat HomograpgyMatrixCenter);

private:
	double MinPlayerHeight;
	double MaxPlayerHeight;
	GMM_Model GmmModelForPlayerClassification;
	int iterate;
};

