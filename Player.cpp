#include "Player.h"

Player::Player(int Id, int TeamId, Rect BoundingBox, Point2d ActualPosition, double RealHeight, Mat ObjectColorImage, Mat ObjectMaskImage) : ImageObject(Id, TeamId, BoundingBox, ActualPosition, RealHeight, ObjectColorImage, ObjectMaskImage)
{
	iterate = 0;
}


Player::~Player()
{
}

void Player::SetMinPlayerHeight(double MinPlayerHeight)
{
	this->MinPlayerHeight = MinPlayerHeight;
}

void Player::SetMaxPlayerHeight(double MaxPlayerHeight)
{
	this->MaxPlayerHeight = MaxPlayerHeight;
}

void Player::SetGmmModel(Ptr<EM> em_model)
{
	GmmModelForPlayerClassification.em_model = em_model;
	GmmModelForPlayerClassification.trained = true;
}

int Player::GetHistogramMaxPosition(Mat & ColorImage, Mat & Mask)
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

Mat Player::TransformPoint(Point2d Point, const Mat & HomographyMatrix)
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

PlayerTrackProperties Player::Track(Mat &ColorImage, Mat &BgFgImage, vector<ImageObject*> CurrentObjects, Panorama::Offset CenterOffset, Mat HomograpgyMatrixCenter)
{
	cout << "Team Id: " << TeamId << endl;
	cout << AverageRealHeight << endl;

	PlayerTrackProperties TrackProperties;

	TrackProperties.Id = Id;

	if (ShowDebugPictures)
	{
		namedWindow("Prev player image", WINDOW_KEEPRATIO);

		imshow("Prev player image", ObjectColorImage);

		waitKey(100);
	}

	double MaxSearchRadius = 2;
	vector<int> IdsInSearchRadius;

	Mat ColorImageCopy;

	if (ShowDebugPictures)
	{
		namedWindow("Panorama for track", WINDOW_KEEPRATIO);
		namedWindow("Object in search radius", WINDOW_KEEPRATIO);
		namedWindow("Object in search radius mask", WINDOW_KEEPRATIO);

		ColorImageCopy = Mat();

		ColorImageCopy = Mat(ColorImage.rows, ColorImage.cols, ColorImage.type());

		ColorImage.copyTo(ColorImageCopy);

		rectangle(ColorImageCopy, BoundingBoxes.at(0), Scalar(0, 0, 255), 5);

		imshow("Panorama for track", ColorImageCopy);
	}

	if (iterate == 1 && TrackStatus.State == NoObjectInSearchRadius)
	{
		MaxSearchRadius *= 2;
	}

	double GlobalMinDistanceFromPlayer = -1;

	for (int i = 0; i < CurrentObjects.size(); i++)
	{
		if (iterate == 0)
		{
			if (CurrentObjects.at(i)->GetRealHeights().at(0) > MinPlayerHeight && CurrentObjects.at(i)->GetRealHeights().at(0) < MaxPlayerHeight)
			{
				double DistanceFromPlayer = sqrt(pow(Positions.at(0).x - CurrentObjects.at(i)->GetPositions().at(0).x, 2) + pow(Positions.at(0).y - CurrentObjects.at(i)->GetPositions().at(0).y, 2));

				if (DistanceFromPlayer <= MaxSearchRadius)
				{
					if (ShowDebugPictures)
					{
						ColorImageCopy = Mat();

						ColorImageCopy = Mat(ColorImage.rows, ColorImage.cols, ColorImage.type());

						ColorImage.copyTo(ColorImageCopy);

						rectangle(ColorImageCopy, BoundingBoxes.at(0), Scalar(0, 0, 255), 5);
						rectangle(ColorImageCopy, CurrentObjects.at(i)->GetBoundingBoxes().at(0), Scalar(255, 0, 0), 5);

						imshow("Panorama for track", ColorImageCopy);
						imshow("Object in search radius", CurrentObjects.at(i)->GetObjectColorImage());
						imshow("Object in search radius mask", CurrentObjects.at(i)->GetObjectMaskImage());

						waitKey(0);
					}

					IdsInSearchRadius.push_back(CurrentObjects.at(i)->GetId());
				}
			}
		}
		else if (iterate == 1)
		{
			if (CurrentObjects.at(i)->GetRealHeights().at(0) > AverageRealHeight*0.5 && CurrentObjects.at(i)->GetRealHeights().at(0) < AverageRealHeight*1.5)
			{
				double DistanceFromPlayer = sqrt(pow(Positions.at(0).x - CurrentObjects.at(i)->GetPositions().at(0).x, 2) + pow(Positions.at(0).y - CurrentObjects.at(i)->GetPositions().at(0).y, 2));

				if (DistanceFromPlayer < GlobalMinDistanceFromPlayer || GlobalMinDistanceFromPlayer == -1)
				{
					GlobalMinDistanceFromPlayer = DistanceFromPlayer;
				}

				if (DistanceFromPlayer <= MaxSearchRadius)
				{
					if (ShowDebugPictures)
					{
						ColorImageCopy = Mat();

						ColorImageCopy = Mat(ColorImage.rows, ColorImage.cols, ColorImage.type());

						ColorImage.copyTo(ColorImageCopy);

						rectangle(ColorImageCopy, BoundingBoxes.at(0), Scalar(0, 0, 255), 5);
						rectangle(ColorImageCopy, CurrentObjects.at(i)->GetBoundingBoxes().at(0), Scalar(255, 0, 0), 5);

						imshow("Panorama for track", ColorImageCopy);
						imshow("Object in search radius", CurrentObjects.at(i)->GetObjectColorImage());
						imshow("Object in search radius mask", CurrentObjects.at(i)->GetObjectMaskImage());

						waitKey(0);
					}

					IdsInSearchRadius.push_back(CurrentObjects.at(i)->GetId());
				}
			}
		}
	}

	Point2d NextPlayerPositionEstimation;

	if (Positions.size() >= 2)
	{
		double dx = Positions.at(0).x - Positions.at(1).x;
		double dy = Positions.at(0).y - Positions.at(1).y;

		NextPlayerPositionEstimation = Point2d(Positions.at(0).x + dx, Positions.at(0).y + dy);
	}
	else
	{
		NextPlayerPositionEstimation = Positions.at(0);
	}

	if (IdsInSearchRadius.size() == 0)
	{
		cout << "No object in search radius! iterate: " << iterate << endl;

		if (ShowDebugPictures)
		{
			waitKey(0);

			destroyWindow("Object in search radius");
			destroyWindow("Object in search radius mask");
		}

		if (iterate == 1 && TrackStatus.State == NoObjectInSearchRadius)
		{
			cout << "Nearest object to not found player: " << GlobalMinDistanceFromPlayer << endl;

			iterate = 0;
		}
		else
		{
			iterate = 1;
		}

		TrackProperties.State = NoObjectInSearchRadius;

		TrackStatus = TrackProperties;

		return TrackProperties;
	}

	if (ShowDebugPictures)
	{
		destroyWindow("Object in search radius");
		destroyWindow("Object in search radius mask");
	}

	double MinDistanceFromPlayer = -1;
	int NearestObjectToEstimatedPositionId = -1;

	for (int i = 0; i < IdsInSearchRadius.size(); i++)
	{
		ImageObject* ObjectSearchedById = NULL;

		for (int j = 0; j < CurrentObjects.size(); j++)
		{
			if (CurrentObjects.at(j)->GetId() == IdsInSearchRadius.at(i))
			{
				ObjectSearchedById = CurrentObjects.at(j);
			}
		}

		double DistanceFromPlayer = sqrt(pow(Positions.at(0).x - ObjectSearchedById->GetPositions().at(0).x, 2) + pow(Positions.at(0).y - ObjectSearchedById->GetPositions().at(0).y, 2));

		if (DistanceFromPlayer < MinDistanceFromPlayer || MinDistanceFromPlayer == -1)
		{
			NearestObjectToEstimatedPositionId = ObjectSearchedById->GetId();
			MinDistanceFromPlayer = DistanceFromPlayer;
		}
	}

	ImageObject* NearestObject = NULL;

	for (int j = 0; j < CurrentObjects.size(); j++)
	{
		if (CurrentObjects.at(j)->GetId() == NearestObjectToEstimatedPositionId)
		{
			NearestObject = CurrentObjects.at(j);
		}
	}

	TrackProperties.NearestObjectToEstimatedPositionId = NearestObjectToEstimatedPositionId;

	if (ShowDebugPictures)
	{
		namedWindow("Nearest object to the estimated position", WINDOW_KEEPRATIO);

		ImageObject* ObjectSearchedById = NULL;

		for (int j = 0; j < CurrentObjects.size(); j++)
		{
			if (CurrentObjects.at(j)->GetId() == NearestObjectToEstimatedPositionId)
			{
				ObjectSearchedById = CurrentObjects.at(j);
			}
		}

		imshow("Nearest object to the estimated position", ObjectSearchedById->GetObjectColorImage());

		rectangle(ColorImageCopy, ObjectSearchedById->GetBoundingBoxes().at(0), Scalar(255, 0, 0), 5);

		imshow("Panorama for track", ColorImageCopy);

		waitKey(0);

		destroyWindow("Nearest object to the estimated position");
	}

	MinDistanceFromPlayer = -1;
	int NearestSameTeamObjectToEstimatedPositionId = -1;

	for (int i = 0; i < IdsInSearchRadius.size(); i++)
	{
		ImageObject* ObjectSearchedById = NULL;

		for (int j = 0; j < CurrentObjects.size(); j++)
		{
			if (CurrentObjects.at(j)->GetId() == IdsInSearchRadius.at(i))
			{
				ObjectSearchedById = CurrentObjects.at(j);
			}
		}

		double DistanceFromPlayer = sqrt(pow(Positions.at(0).x - ObjectSearchedById->GetPositions().at(0).x, 2) + pow(Positions.at(0).y - ObjectSearchedById->GetPositions().at(0).y, 2));

		if ((DistanceFromPlayer < MinDistanceFromPlayer || MinDistanceFromPlayer == -1) && TeamId == ObjectSearchedById->GetTeamId())
		{
			NearestSameTeamObjectToEstimatedPositionId = ObjectSearchedById->GetId();
			MinDistanceFromPlayer = DistanceFromPlayer;
		}
	}

	ImageObject* NearestSameTeamObject = NULL;

	for (int j = 0; j < CurrentObjects.size(); j++)
	{
		if (CurrentObjects.at(j)->GetId() == NearestSameTeamObjectToEstimatedPositionId)
		{
			NearestSameTeamObject = CurrentObjects.at(j);
		}
	}

	if (ShowDebugPictures)
	{
		if (NearestSameTeamObjectToEstimatedPositionId != -1)
		{
			namedWindow("Nearest object to the estimated position with same team", WINDOW_KEEPRATIO);

			ImageObject* ObjectSearchedById = NULL;

			for (int j = 0; j < CurrentObjects.size(); j++)
			{
				if (CurrentObjects.at(j)->GetId() == NearestSameTeamObjectToEstimatedPositionId)
				{
					ObjectSearchedById = CurrentObjects.at(j);
				}
			}

			imshow("Nearest object to the estimated position with same team", ObjectSearchedById->GetObjectColorImage());

			ColorImageCopy = Mat();

			ColorImageCopy = Mat(ColorImage.rows, ColorImage.cols, ColorImage.type());

			ColorImage.copyTo(ColorImageCopy);

			rectangle(ColorImageCopy, ObjectSearchedById->GetBoundingBoxes().at(0), Scalar(255, 0, 0), 5);

			imshow("Panorama for track", ColorImageCopy);

			waitKey(0);

			destroyWindow("Nearest object to the estimated position with same team");
		}
		else
		{
			cout << "There is no object in search radius with same team id!" << endl;
		}
	}

	TrackProperties.NearestSameTeamObjectToEstimatedPositionId = NearestSameTeamObjectToEstimatedPositionId;

	if (iterate == 0)
	{
		if (NearestSameTeamObjectToEstimatedPositionId == -1)
		{
			cout << "There is no object in search radius with same team id!" << endl;

			iterate = 1;

			TrackProperties.State = DifferentIdsForColorAndPosition;

			TrackStatus = TrackProperties;

			return TrackProperties;
		}
		else
		{
			if (NearestObjectToEstimatedPositionId == NearestSameTeamObjectToEstimatedPositionId)
			{
				TrackProperties.State = TrackDone;
			}
			else
			{
				cout << "Different Ids for color and position!" << endl;

				TrackProperties.State = TrackDone;
				TrackProperties.NearestObjectToEstimatedPositionId = NearestSameTeamObjectToEstimatedPositionId;
			}
		}
	}
	else if (iterate == 1)
	{
		if (TrackStatus.State == DifferentIdsForColorAndPosition)
		{
			TrackProperties.State = TrackDone;

			TrackProperties.NearestObjectToEstimatedPositionId = NearestObjectToEstimatedPositionId;
			TrackProperties.NearestSameTeamObjectToEstimatedPositionId = NearestObjectToEstimatedPositionId;

			iterate = 0;
		}
		if (TrackStatus.State == NoObjectInSearchRadius)
		{
			TrackProperties.State = TrackDone;

			if (NearestSameTeamObjectToEstimatedPositionId != -1)
			{
				TrackProperties.NearestObjectToEstimatedPositionId = NearestSameTeamObjectToEstimatedPositionId;
				TrackProperties.NearestSameTeamObjectToEstimatedPositionId = NearestSameTeamObjectToEstimatedPositionId;
			}
			else
			{
				TrackProperties.NearestObjectToEstimatedPositionId = NearestObjectToEstimatedPositionId;
				TrackProperties.NearestSameTeamObjectToEstimatedPositionId = NearestObjectToEstimatedPositionId;
			}

			iterate = 0;
		}
	}

	ImageObject* ObjectSearchedById = NULL;

	for (int j = 0; j < CurrentObjects.size(); j++)
	{
		if (CurrentObjects.at(j)->GetId() == TrackProperties.NearestSameTeamObjectToEstimatedPositionId)
		{
			ObjectSearchedById = CurrentObjects.at(j);
		}
	}

	Positions.insert(Positions.begin(), ObjectSearchedById->GetPositions().at(0));
	BoundingBoxes.insert(BoundingBoxes.begin(), ObjectSearchedById->GetBoundingBoxes().at(0));

	if (RealHeights.size() == 10)
	{
		RealHeights.erase(RealHeights.begin() + 9);
	}

	RealHeights.insert(RealHeights.begin(), ObjectSearchedById->GetRealHeights().at(0));

	AverageRealHeight = 0;

	for (int i = 0; i < RealHeights.size(); i++)
	{
		AverageRealHeight += RealHeights.at(i);
	}

	if (RealHeights.size() != 0)
	{
		AverageRealHeight /= RealHeights.size();
	}

	ObjectColorImage = Mat();
	ObjectSearchedById->GetObjectColorImage().copyTo(ObjectColorImage);
	ObjectMaskImage = Mat();
	ObjectSearchedById->GetObjectMaskImage().copyTo(ObjectMaskImage);

	if (Positions.size() >= 2)
	{
		Velocities.insert(Velocities.begin(), MinDistanceFromPlayer);
	}

	if (Velocities.size() >= 2)
	{
		Accelerations.insert(Accelerations.begin(), abs(Velocities.at(0) - Velocities.at(1)));
	}

	if (ShowDebugPictures)
	{
		destroyWindow("Prev player image");
	}

	TrackStatus = TrackProperties;

	return TrackProperties;
}
