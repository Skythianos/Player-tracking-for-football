#pragma once

#include "Globals.h"

class EMHandler
{
public:
	EMHandler(Mat Frame, Mat Mask);
	~EMHandler();

	Mat Frame;
	Mat ConvertedFrame;
	Mat Mask;
	Mat hsv[3];
	vector<Mat> ChannelVector;

	bool TrainEMModel(const int ClusterNumber, Ptr<EM> &em_model);
	void ClassifyImage(Mat &FrameClassified, Mat &FrameGMM, const Ptr<EM> &em_model, const int ClusterNumber, const Rect roi);

private:
	Mat ConvertHueChannelAngles();
	Mat ConvertMatrixToVector(Mat &matrix);
};

