#pragma once

#include "Globals.h"

class Camera
{
public:
	Camera();
	Camera(Mat Frame);
	Camera(Mat Frame, Mat Mask);
	~Camera();

	void SetFrame(Mat Frame);
	void SetMask(Mat Mask);
	Mat GetConfigFrame();
	Mat GetMask();
	int GetCurrentClusterNumber();
	vector<int> GetBackGroundIds();
	
	bool ClassifyImage(Mat &FrameClassified, Mat &FrameGMM, const int ClusterNumber, const int ThreadNumber);
	void CreateMask(vector<Point2f> ROIPoints, Point2f InitPoint);
	void CreateBgFgImage(Mat &FrameClassified, Mat &BgFgImage);

	bool AddClusterToBackGround();
	bool RemoveClusterFromBackGround();

private:
	Mat Frame;
	Mat Mask;
	vector<Mat> channelvector;
	vector<int> BackGroundIds;

	GMM_Model GMM_Models[10];

	int CannyTH1;
	int CannyTH2;

	int CurrentClusterNumber;
};

