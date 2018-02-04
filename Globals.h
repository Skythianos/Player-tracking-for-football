#pragma once

#include <opencv2/opencv.hpp>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/ml.hpp"
#include <thread>
#include <math.h>
#include <stack>

#define PI 3.14159265

using namespace cv;
using namespace cv::ml;
using namespace std;

const Scalar colors[] =
{
	Scalar(255, 0, 0), Scalar(0, 255, 0),
	Scalar(0, 0, 255), Scalar(0, 255, 255),
	Scalar(255, 255, 0), Scalar(255, 0, 255),
	Scalar(255, 255, 255), Scalar(128, 128, 128),
	Scalar(0, 128, 255), Scalar(0, 0, 0)
};

struct GMM_Model
{
	bool trained = false;
	Ptr<EM> em_model;
};

typedef struct MaskLineType{
	int vertical;
	int x;
	double m;
	double b;
	int direction;
}MaskLine;

struct CoordWithWindow {
	int Window;
	Point2f Clickcoord;
};

struct ObjectProperties
{
	int Id;
	Rect ObjectBoundingBox;
	Mat ObjectImage;
	Mat ObjectMask;
	Point2d ObjectRealPosition;
	int ObjectMaxHistogramPosition;
	int TeamId;
	double RealHeight;
};

enum PlayerTrackState
{
	NoObjectInSearchRadius,
	DifferentIdsForColorAndPosition,
	TrackDone
};

struct PlayerTrackProperties
{
	int Id;
	PlayerTrackState State;
	int NearestObjectToEstimatedPositionId;
	int NearestSameTeamObjectToEstimatedPositionId;
};

struct MenuState
{
	int Menu;
	int SubMenu;
};

struct ProgramState
{
	int CutROIMode;
	int StartSetPanorama;
	int PlayersInit;
};