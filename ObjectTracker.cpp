#include "ObjectTracker.h"



ObjectTracker::ObjectTracker()
{
}


ObjectTracker::~ObjectTracker()
{
}

void ObjectTracker::AddObjects(ImageObject* object)
{
	Objects.push_back(object);
}

bool ObjectTracker::RemoveObject(int Id)
{
	for (int i = 0; i < Objects.size(); i++)
	{
		if (Objects.at(i)->GetId() == Id)
		{
			Objects.erase(Objects.begin() + i);
			return true;
		}
	}

	return false;
}

vector<ImageObject*> ObjectTracker::GetObjects()
{
	return Objects;
}

void ObjectTracker::SetGmmModel(Ptr<EM> em_model)
{
	GmmModelForPlayerClassification.em_model = em_model;
	GmmModelForPlayerClassification.trained = true;
}

int ObjectTracker::GetHistogramMaxPosition(Mat & ColorImage, Mat & Mask)
{
	int hbins = 180;
	int histSize[] = { hbins };
	float hranges[] = { 0, 180 };
	const float* ranges[] = { hranges };
	int channels[] = { 0 };

	Mat hsv, PlayerHistogram;

	cvtColor(ColorImage, hsv, COLOR_BGR2HSV);

	calcHist(&hsv, 1, channels, Mask, PlayerHistogram, 1, histSize, ranges, true, false);

	float HistMaxValue = 0;
	int HistMaxPosition = 0;

	for (int j = 0; j < hbins; j++)
	{
		if (PlayerHistogram.at<float>(j) > HistMaxValue)
		{
			HistMaxValue = PlayerHistogram.at<float>(j);
			HistMaxPosition = j;
		}
	}

	return HistMaxPosition;
}

Mat ObjectTracker::TransformPoint(Point2d Point, const Mat & HomographyMatrix)
{
	Mat OneMat = Mat::eye(1, 1, HomographyMatrix.type());

	Mat ExtendedPoint; // (Point.x, Point.y, 1)
	Mat PointForTypeConvert;
	vector<Mat> ConcatenateVector;

	Mat(Point).convertTo(PointForTypeConvert, HomographyMatrix.type());

	ConcatenateVector.push_back(PointForTypeConvert);
	ConcatenateVector.push_back(OneMat);

	vconcat(ConcatenateVector, ExtendedPoint);

	Mat TransformedPoint = HomographyMatrix * ExtendedPoint;

	double ratio = 1 / TransformedPoint.at<double>(2);
	TransformedPoint.at<double>(0) = TransformedPoint.at<double>(0) * ratio;
	TransformedPoint.at<double>(1) = TransformedPoint.at<double>(1) * ratio;
	TransformedPoint.at<double>(2) = TransformedPoint.at<double>(2) * ratio;

	return TransformedPoint;
}

void ObjectTracker::TrackAll(Mat &ColorImage, Mat &BgFgImage, vector<ImageObject*> CurrentObjects, Panorama::Offset CenterOffset, Mat HomograpgyMatrixCenter)
{
	vector<PlayerTrackProperties> PlayersTrackProperties;

	for (int i = 0; i < Objects.size(); i++)
	{
		PlayerTrackProperties ActualPlayerTrackProperties;

		int TeamIdCluster = Objects.at(i)->GetTeamId();

		ActualPlayerTrackProperties = Objects.at(i)->Track(ColorImage, BgFgImage, CurrentObjects, CenterOffset, HomograpgyMatrixCenter);

		PlayersTrackProperties.push_back(ActualPlayerTrackProperties);
	}

	vector<ImageObject*> UnselectedCurrentObjects;
	vector<int> SelectedCurrentObjectIds;

	for (int i = 0; i < PlayersTrackProperties.size(); i++)
	{
		if (PlayersTrackProperties.at(i).State == TrackDone)
		{
			SelectedCurrentObjectIds.push_back(PlayersTrackProperties.at(i).NearestSameTeamObjectToEstimatedPositionId);
		}
	}

	for (int i = 0; i < CurrentObjects.size(); i++)
	{
		bool SelectedCurrentObjectId = false;

		for (int j = 0; j < SelectedCurrentObjectIds.size(); j++)
		{
			if (SelectedCurrentObjectIds.at(j) == CurrentObjects.at(i)->GetId())
			{
				SelectedCurrentObjectId = true;
			}
		}

		if (!SelectedCurrentObjectId)
		{
			UnselectedCurrentObjects.push_back(CurrentObjects.at(i));
		}
	}

	for (int i = 0; i < Objects.size(); i++)
	{
		if (Objects.at(i)->TrackStatus.State == DifferentIdsForColorAndPosition)
		{
			Objects.at(i)->Track(ColorImage, BgFgImage, UnselectedCurrentObjects, CenterOffset, HomograpgyMatrixCenter);
		}
	}

	UnselectedCurrentObjects.clear();
	SelectedCurrentObjectIds.clear();

	for (int i = 0; i < Objects.size(); i++)
	{
		if (Objects.at(i)->TrackStatus.State == TrackDone)
		{
			SelectedCurrentObjectIds.push_back(Objects.at(i)->TrackStatus.NearestObjectToEstimatedPositionId);
		}
	}

	for (int i = 0; i < CurrentObjects.size(); i++)
	{
		bool SelectedCurrentObjectId = false;

		for (int j = 0; j < SelectedCurrentObjectIds.size(); j++)
		{
			if (SelectedCurrentObjectIds.at(j) == CurrentObjects.at(i)->GetId())
			{
				SelectedCurrentObjectId = true;
			}
		}

		if (!SelectedCurrentObjectId)
		{
			UnselectedCurrentObjects.push_back(CurrentObjects.at(i));
		}
	}

	for (int i = 0; i < Objects.size(); i++)
	{
		if (Objects.at(i)->TrackStatus.State == NoObjectInSearchRadius)
		{
			Objects.at(i)->Track(ColorImage, BgFgImage, UnselectedCurrentObjects, CenterOffset, HomograpgyMatrixCenter);
		}
	}
}
