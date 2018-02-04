#pragma once

#include "Globals.h"
#include "ImageObject.h"

class ObjectTracker
{
public:
	ObjectTracker();
	~ObjectTracker();

	void AddObjects(ImageObject* object);
	bool RemoveObject(int Id);
	vector<ImageObject*> GetObjects();

	void SetGmmModel(Ptr<EM> em_model);

	Mat TransformPoint(Point2d Point, const Mat & HomographyMatrix);
	int GetHistogramMaxPosition(Mat & ColorImage, Mat & Mask);
	void TrackAll(Mat &ColorImage, Mat &BgFgImage, vector<ImageObject*> CurrentObjects, Panorama::Offset CenterOffset, Mat HomograpgyMatrixCenter);

private:
	vector<ImageObject*> Objects;
	GMM_Model GmmModelForPlayerClassification;
};

