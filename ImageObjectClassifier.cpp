#include "ImageObjectClassifier.h"


ImageObjectClassifier::ImageObjectClassifier()
{
	MinPlayerHeight = 3;
	MaxPlayerHeight = 15;
	CurrentClusterNumber = -1;

	for (int i = 0; i < 10; i++)
	{
		GMM_ModelsForTeam[i].trained = false;
	}
}

ImageObjectClassifier::ImageObjectClassifier(Mat BgFgImage, Mat ColorImage)
{
	MinPlayerHeight = 3;
	MaxPlayerHeight = 15;

	CurrentClusterNumber = -1;

	for (int i = 0; i < 10; i++)
	{
		GMM_ModelsForTeam[i].trained = false;
	}

	this->BgFgImage = BgFgImage;
	this->ColorImage = ColorImage;
}

ImageObjectClassifier::~ImageObjectClassifier()
{
}

void ImageObjectClassifier::SetBgFgImage(Mat BgFgImage)
{
	this->BgFgImage = BgFgImage;
}

Mat ImageObjectClassifier::GetBgFgImage(void)
{
	return BgFgImage;
}

void ImageObjectClassifier::SetColorImage(Mat ColorImage)
{
	this->ColorImage = ColorImage;
}

Mat ImageObjectClassifier::GetColorImage(void)
{
	return ColorImage;
}

vector<Rect> ImageObjectClassifier::GetPlayerBoundingBoxes(void)
{
	return PlayerBoundingBoxes;
}

vector<Point2d> ImageObjectClassifier::GetPlayerPositions(void)
{
	return PlayerPositions;
}

vector<Point2d> ImageObjectClassifier::GetRealPlayerPositions()
{
	return RealPlayerPositions;
}

vector<int> ImageObjectClassifier::GetRealPlayersTeam(void)
{
	return RealPlayersTeam;
}

vector<int> ImageObjectClassifier::GetPLayersTeam(void)
{
	return PlayersTeam;
}

vector<int> ImageObjectClassifier::GetRemovedObjectIds(void)
{
	return RemovedObjectIds;
}

void ImageObjectClassifier::SetMinPlayerHeight(int MinPlayerHeight)
{
	this->MinPlayerHeight = MinPlayerHeight;
}

void ImageObjectClassifier::SetMaxPlayerHeight(int MaxPlayerHeight)
{
	this->MaxPlayerHeight = MaxPlayerHeight;
}

int ImageObjectClassifier::GetCurrentClusterNumber(void)
{
	return CurrentClusterNumber;
}

Ptr<EM> ImageObjectClassifier::GetCurrentEMModel(void)
{
	return GMM_ModelsForTeam[CurrentClusterNumber].em_model;
}

vector<Mat> ImageObjectClassifier::GetPlayerMasks(void)
{
	return PlayerMasks;
}

vector<Mat> ImageObjectClassifier::GetPlayerImages(void)
{
	return PlayerImages;
}

double ImageObjectClassifier::GetMinPlayerHeight(void)
{
	return MinPlayerHeight;
}

double ImageObjectClassifier::GetMaxPlayerHeight(void)
{
	return MaxPlayerHeight;
}

vector<double> ImageObjectClassifier::GetPlayerRealHeights(void)
{
	return PlayerRealHeights;
}

Mat ImageObjectClassifier::TransformPoint(Point2d Point, const Mat & HomographyMatrix)
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

int ImageObjectClassifier::GetHistogramMaxPosition(Mat & ColorImage, Mat & Mask)
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

void ImageObjectClassifier::FindProbablePlayers(const Mat & HomographyMatrix, Panorama::Offset CenterOffset)
{
	Mat labels, stats, centroids;

	connectedComponentsWithStats(BgFgImage, labels, stats, centroids);

	for (int i = 0; i < centroids.rows; i++)
	{
		Mat BoundingBoxTop = TransformPoint(Point2d((double)stats.at<int>(i, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) - CenterOffset.rows), HomographyMatrix);
		Point2d BoundingBoxTopPoint = Point2d(BoundingBoxTop.at<double>(0), BoundingBoxTop.at<double>(1));

		Mat BoundingBoxBottom = TransformPoint(Point2d((double)stats.at<int>(i, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)stats.at<int>(i, 3) - CenterOffset.rows), HomographyMatrix);
		Point2d BoundingBoxBottomPoint = Point2d(BoundingBoxBottom.at<double>(0), BoundingBoxBottom.at<double>(1));

		double RealHeight = sqrt(pow(BoundingBoxBottomPoint.x - BoundingBoxTopPoint.x, 2) + pow(BoundingBoxBottomPoint.y - BoundingBoxTopPoint.y, 2));

		if (RealHeight > MinPlayerHeight && RealHeight < MaxPlayerHeight)
		{
			Rect PlayerBoundingBox = Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3));
			Mat PlayerLabelRegion = labels(Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3)));

			Mat PlayerMask = Mat(PlayerLabelRegion.rows, PlayerLabelRegion.cols, CV_8UC1);

			for (int y = 0; y < PlayerLabelRegion.rows; y++)
			{
				for (int x = 0; x < PlayerLabelRegion.cols; x++)
				{
					if (PlayerLabelRegion.at<int>(y, x) == i)
					{
						PlayerMask.at<char>(y, x) = 255;
					}
					else
					{
						PlayerMask.at<char>(y, x) = 0;
					}
				}
			}

			Point2d PlayerImagePosition = Point2d((double)stats.at<int>(i, 0) + (double)stats.at<int>(i, 2) / 2 - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)stats.at<int>(i, 3) - CenterOffset.rows);

			Mat PlayerObjectPositionMat = TransformPoint(PlayerImagePosition, HomographyMatrix);

			Point2d PlayerObjectPosition = Point2d(PlayerObjectPositionMat.at<double>(0), PlayerObjectPositionMat.at<double>(1));

			int HistMaxValuePosition = GetHistogramMaxPosition(ColorImage(PlayerBoundingBox), PlayerMask);

			PlayerBoundingBoxes.push_back(PlayerBoundingBox);
			PlayerMasks.push_back(PlayerMask);
			PlayerImages.push_back(ColorImage(PlayerBoundingBox));
			PlayerPositions.push_back(PlayerObjectPosition);
			PlayerMaxHistogramPositions.push_back(HistMaxValuePosition);
		}
	}
}

void ImageObjectClassifier::SetProbablePlayersMaxHistogramPositions()
{
	for (int i = 0; i < PlayerImages.size(); i++)
	{
		int HistMaxValue = GetHistogramMaxPosition(PlayerImages.at(i), PlayerMasks.at(i));

		PlayerMaxHistogramPositions.push_back(HistMaxValue);
	}
}

bool ImageObjectClassifier::ClassifyPlayers(int ClusterNumber)
{
	bool TrainResult = true;

	PlayersTeam.clear();

	Mat samples = Mat(PlayerMaxHistogramPositions.size(), 1, CV_32FC1);

	for (int i = 0; i < PlayerMaxHistogramPositions.size(); i++)
	{
		samples.at<float>(i, 0) = (float)PlayerMaxHistogramPositions.at(i);
	}

	Ptr<EM> em_model;

	CurrentClusterNumber = ClusterNumber;

	if (GMM_ModelsForTeam[ClusterNumber].trained == false)
	{

		em_model = EM::create();
		em_model->setClustersNumber(ClusterNumber);
		em_model->setCovarianceMatrixType(EM::COV_MAT_SPHERICAL);
		em_model->setTermCriteria(TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 300, 0.1));

		TrainResult = em_model->trainEM(samples);

		if (TrainResult == true)
		{
			GMM_ModelsForTeam[ClusterNumber].em_model = em_model;

			GMM_ModelsForTeam[ClusterNumber].trained = true;
		}
	}
	else
	{
		em_model = GMM_ModelsForTeam[ClusterNumber].em_model;
	}

	cout << em_model->getMeans() << endl;

	for (int i = 0; i < samples.rows; i++)
	{
		double MinDistanceFromClustersMean = -1.0;
		int NearestCluster = -1;

		for (int j = 0; j < CurrentClusterNumber; j++)
		{
			double DistanceFromClusterMean = abs(GMM_ModelsForTeam[CurrentClusterNumber].em_model->getMeans().at<double>(j, 0) - (double)samples.at<float>(i));

			if (DistanceFromClusterMean < MinDistanceFromClustersMean || MinDistanceFromClustersMean == -1.0)
			{
				MinDistanceFromClustersMean = DistanceFromClusterMean;
				NearestCluster = j;
			}
		}

		PlayersTeam.push_back(NearestCluster);
	}

	return TrainResult;
}

void ImageObjectClassifier::ClearVectors()
{
	PlayerBoundingBoxes.clear();
	PlayerMasks.clear();
	PlayerImages.clear();
	PlayerPositions.clear();
	PlayerMaxHistogramPositions.clear();
	PlayersTeam.clear();
	PlayerRealHeights.clear();
	RealPlayerPositions.clear();
	RealPlayersTeam.clear();
}

bool ImageObjectClassifier::RemoveObjectFromProbablePlayers(Point2d ObjectPosition)
{
	if (PlayerPositions.size() != 0)
	{
		int IdForObjectToRemove = 0;
		double MinDistanceFromObjectToRemove = sqrt(pow(PlayerPositions.at(0).x - ObjectPosition.x, 2) + pow(PlayerPositions.at(0).y - ObjectPosition.y, 2));

		for (int i = 0; i < PlayerPositions.size(); i++)
		{
			double DistanceFromObjectToRemove = sqrt(pow(PlayerPositions.at(i).x - ObjectPosition.x, 2) + pow(PlayerPositions.at(i).y - ObjectPosition.y, 2));

			if (DistanceFromObjectToRemove < MinDistanceFromObjectToRemove)
			{
				MinDistanceFromObjectToRemove = DistanceFromObjectToRemove;
				IdForObjectToRemove = i;
			}
		}

		RemovedObjectIds.push_back(IdForObjectToRemove);

		return true;
	}
	else
	{
		cout << "Player (position) number is zero!" << endl;
		
		return false;
	}
}

bool ImageObjectClassifier::AddObjectToProbablePlayers(Point2d ObjectPosition)
{
	if (RemovedObjectIds.size() != 0)
	{
		double MinDistanceFromObjectToAdd = sqrt(pow(PlayerPositions.at(RemovedObjectIds.at(0)).x - ObjectPosition.x, 2) + pow(PlayerPositions.at(RemovedObjectIds.at(0)).y - ObjectPosition.y, 2));
		int IdForObjectToAdd = 0;

		for (int i = 0; i < RemovedObjectIds.size(); i++)
		{
			double DistanceFromObjectToAdd = sqrt(pow(PlayerPositions.at(RemovedObjectIds.at(i)).x - ObjectPosition.x, 2) + pow(PlayerPositions.at(RemovedObjectIds.at(i)).y - ObjectPosition.y, 2));

			if (DistanceFromObjectToAdd < MinDistanceFromObjectToAdd)
			{
				MinDistanceFromObjectToAdd = DistanceFromObjectToAdd;
				IdForObjectToAdd = i;
			}
		}

		RemovedObjectIds.erase(RemovedObjectIds.begin() + IdForObjectToAdd);

		return true;
	}
	else
	{
		cout << "There is no removed object!" << endl;

		return false;
	}
}

void ImageObjectClassifier::UpdateProbablePlayersWithTeamInformation(const Mat & HomographyMatrix, Panorama::Offset CenterOffset)
{
	Mat labels, stats, centroids;

	vector<Rect> ObjectBoundingBoxes;
	vector<Mat> ObjectMasks;
	vector<Mat> ObjectImages;
	vector<Point2d> ObjectPositions;
	vector<double> ObjectRealHeights;

	Mat* BgFgImagesWithTeam = new Mat[CurrentClusterNumber];

	for (int i = 0; i < CurrentClusterNumber; i++)
	{
		BgFgImagesWithTeam[i] = Mat::zeros(BgFgImage.rows, BgFgImage.cols, CV_8U);
	}

	Mat BgFgImageWithJustPlayers = Mat::zeros(BgFgImage.rows, BgFgImage.cols, CV_8U);

	vector<int> PlayersClusterIds;

	for (int i = 0; i < PlayersTeam.size(); i++)
	{
		bool RemovedId = false;
		int ActualPlayerTeam = PlayersTeam.at(i);

		for (int j = 0; j < RemovedObjectIds.size(); j++)
		{
			if (i == RemovedObjectIds.at(j))
			{
				RemovedId = true;
			}
		}

		if (!RemovedId)
		{
			bool AlreadyFoundTeamId = false;

			for (int j = 0; j < PlayersClusterIds.size(); j++)
			{
				if (ActualPlayerTeam == PlayersClusterIds.at(j))
				{
					AlreadyFoundTeamId = true;
				}
			}

			if (!AlreadyFoundTeamId)
			{
				PlayersClusterIds.push_back(ActualPlayerTeam);

				cout << ActualPlayerTeam << endl;
			}
		}
	}

	connectedComponentsWithStats(BgFgImage, labels, stats, centroids);

	for (int i = 1; i < centroids.rows; i++)
	{
		Rect ObjectBoundingBox = Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3));
		Mat ObjectLabelRegion = labels(Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3)));

		Mat ObjectMask = Mat(ObjectLabelRegion.rows, ObjectLabelRegion.cols, CV_8UC1);

		for (int y = 0; y < ObjectLabelRegion.rows; y++)
		{
			for (int x = 0; x < ObjectLabelRegion.cols; x++)
			{
				if (ObjectLabelRegion.at<int>(y, x) == i)
				{
					ObjectMask.at<char>(y, x) = 255;
				}
				else
				{
					ObjectMask.at<char>(y, x) = 0;
				}
			}
		}

		Mat PlayerImage;

		ColorImage(ObjectBoundingBox).copyTo(PlayerImage);

		Mat ObjectImagehsv, ObjectImagehsvFloat;
		Mat ObjectImageChannels[3];

		cvtColor(PlayerImage, ObjectImagehsv, COLOR_BGR2HSV);

		ObjectImagehsv.convertTo(ObjectImagehsvFloat, CV_64F);

		split(ObjectImagehsvFloat, ObjectImageChannels);

		for (int y = 0; y < PlayerImage.rows; y++)
		{
			for (int x = 0; x < PlayerImage.cols; x++)
			{
				if (ObjectMask.at<char>(y, x) != 0)
				{
					float MinDistanceFromClustersMean = -1.0;
					int NearestCluster = -1;

					for (int j = 0; j < CurrentClusterNumber; j++)
					{
						float DistanceFromClusterMean = abs(GMM_ModelsForTeam[CurrentClusterNumber].em_model->getMeans().at<double>(j, 0) - ObjectImageChannels[0].at<double>(y, x));

						if (DistanceFromClusterMean < MinDistanceFromClustersMean || MinDistanceFromClustersMean == -1.0)
						{
							MinDistanceFromClustersMean = DistanceFromClusterMean;
							NearestCluster = j;
						}
					}

					BgFgImagesWithTeam[NearestCluster].at<char>(stats.at<int>(i, 1) + y, stats.at<int>(i, 0) + x) = 255;

					bool NearestClusterInPlayersCluster = false;

					for (int k = 0; k < PlayersClusterIds.size(); k++)
					{
						if (NearestCluster == PlayersClusterIds.at(k))
						{
							NearestClusterInPlayersCluster = true;
						}
					}

					if (NearestClusterInPlayersCluster)
					{
						BgFgImageWithJustPlayers.at<char>(stats.at<int>(i, 1) + y, stats.at<int>(i, 0) + x) = 255;
					}
				}
			}
		}
	}

	PlayerBoundingBoxes.clear();
	PlayerMasks.clear();
	PlayerImages.clear();
	PlayerPositions.clear();
	PlayerMaxHistogramPositions.clear();
	PlayersTeam.clear();
	PlayerRealHeights.clear();
	RemovedObjectIds.clear();

	connectedComponentsWithStats(BgFgImageWithJustPlayers, labels, stats, centroids);

	for (int i = 1; i < centroids.rows; i++)
	{
		Mat BoundingBoxTop = TransformPoint(Point2d((double)stats.at<int>(i, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) - CenterOffset.rows), HomographyMatrix);
		Point2d BoundingBoxTopPoint = Point2d(BoundingBoxTop.at<double>(0), BoundingBoxTop.at<double>(1));

		Mat BoundingBoxBottom = TransformPoint(Point2d((double)stats.at<int>(i, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)stats.at<int>(i, 3) - CenterOffset.rows), HomographyMatrix);
		Point2d BoundingBoxBottomPoint = Point2d(BoundingBoxBottom.at<double>(0), BoundingBoxBottom.at<double>(1));

		double RealHeight = sqrt(pow(BoundingBoxBottomPoint.x - BoundingBoxTopPoint.x, 2) + pow(BoundingBoxBottomPoint.y - BoundingBoxTopPoint.y, 2));

		if (RealHeight > MinPlayerHeight)
		{
			Rect ObjectBoundingBox = Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3));
			Mat ObjectLabelRegion = labels(Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3)));

			Mat ObjectMask = Mat(ObjectLabelRegion.rows, ObjectLabelRegion.cols, CV_8UC1);

			for (int y = 0; y < ObjectLabelRegion.rows; y++)
			{
				for (int x = 0; x < ObjectLabelRegion.cols; x++)
				{
					if (ObjectLabelRegion.at<int>(y, x) == i)
					{
						ObjectMask.at<char>(y, x) = 255;
					}
					else
					{
						ObjectMask.at<char>(y, x) = 0;
					}
				}
			}

			Mat PlayerImage;

			ColorImage(ObjectBoundingBox).copyTo(PlayerImage);

			Mat ObjectImagehsv, ObjectImagehsvFloat;
			Mat ObjectImageChannels[3];

			cvtColor(PlayerImage, ObjectImagehsv, COLOR_BGR2HSV);

			ObjectImagehsv.convertTo(ObjectImagehsvFloat, CV_64F);

			split(ObjectImagehsvFloat, ObjectImageChannels);

			for (int y = 0; y < PlayerImage.rows; y++)
			{
				for (int x = 0; x < PlayerImage.cols; x++)
				{
					if (ObjectMask.at<char>(y, x) != 0)
					{
						float MinDistanceFromClustersMean = -1.0;
						int NearestCluster = -1;

						for (int j = 0; j < CurrentClusterNumber; j++)
						{
							float DistanceFromClusterMean = abs(GMM_ModelsForTeam[CurrentClusterNumber].em_model->getMeans().at<double>(j, 0) - ObjectImageChannels[0].at<double>(y, x));

							if (DistanceFromClusterMean < MinDistanceFromClustersMean || MinDistanceFromClustersMean == -1.0)
							{
								MinDistanceFromClustersMean = DistanceFromClusterMean;
								NearestCluster = j;
							}
						}

						BgFgImagesWithTeam[NearestCluster].at<char>(stats.at<int>(i, 1) + y, stats.at<int>(i, 0) + x) = 255;
					}
				}
			}

			vector<int> PlayersClusterIdsImagePart;

			for (int j = 0; j < PlayersClusterIds.size(); j++)
			{
				Mat ObjectImagePartLabels, ObjectImagePartStats, ObjectImagePartCentroids;

				connectedComponentsWithStats(BgFgImagesWithTeam[PlayersClusterIds.at(j)](ObjectBoundingBox) , ObjectImagePartLabels, ObjectImagePartStats, ObjectImagePartCentroids);

				bool CurrentClusterIdInsertedInVector = false;

				for (int k = 1; k < ObjectImagePartCentroids.rows; k++)
				{
					Mat BoundingBoxTopImagePart = TransformPoint(Point2d((double)stats.at<int>(i, 0) + (double)ObjectImagePartStats.at<int>(k, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)ObjectImagePartStats.at<int>(k, 1) - CenterOffset.rows), HomographyMatrix);
					Point2d BoundingBoxTopPointImagePart = Point2d(BoundingBoxTopImagePart.at<double>(0), BoundingBoxTopImagePart.at<double>(1));

					Mat BoundingBoxBottomImagePart = TransformPoint(Point2d((double)stats.at<int>(i, 0) + (double)ObjectImagePartStats.at<int>(k, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)ObjectImagePartStats.at<int>(k, 1) + (double)ObjectImagePartStats.at<int>(k, 3) - CenterOffset.rows), HomographyMatrix);
					Point2d BoundingBoxBottomPointImagePart = Point2d(BoundingBoxBottomImagePart.at<double>(0), BoundingBoxBottomImagePart.at<double>(1));

					double RealHeightImagePart = sqrt(pow(BoundingBoxBottomPointImagePart.x - BoundingBoxTopPointImagePart.x, 2) + pow(BoundingBoxBottomPointImagePart.y - BoundingBoxTopPointImagePart.y, 2));
					
					if (RealHeightImagePart > MinPlayerHeight && RealHeightImagePart < MaxPlayerHeight && !CurrentClusterIdInsertedInVector)
					{
						PlayersClusterIdsImagePart.push_back(PlayersClusterIds.at(j));

						CurrentClusterIdInsertedInVector = true;
					}
				}
			}

			if (PlayersClusterIdsImagePart.size() == 1)
			{
				for (int y = 0; y < PlayerImage.rows; y++)
				{
					for (int x = 0; x < PlayerImage.cols; x++)
					{
						if (ObjectMask.at<char>(y, x) != 0)
						{
							for (int j = 0; j < PlayersClusterIds.size(); j++)
							{
								if (PlayersClusterIdsImagePart.at(0) != PlayersClusterIds.at(j))
								{
									BgFgImagesWithTeam[PlayersClusterIds.at(j)].at<char>(stats.at<int>(i, 1) + y, stats.at<int>(i, 0) + x) = 0;
								}
								else
								{
									BgFgImagesWithTeam[PlayersClusterIds.at(j)].at<char>(stats.at<int>(i, 1) + y, stats.at<int>(i, 0) + x) = 255;
								}
							}
						}
						else
						{
							for (int j = 0; j < PlayersClusterIds.size(); j++)
							{
								BgFgImagesWithTeam[PlayersClusterIds.at(j)].at<char>(stats.at<int>(i, 1) + y, stats.at<int>(i, 0) + x) = 0;
							}
						}
					}
				}
			}
		}
	}

	for (int j = 0; j < PlayersClusterIds.size(); j++)
	{
		Mat labels, stats, centroids;
		connectedComponentsWithStats(BgFgImagesWithTeam[PlayersClusterIds.at(j)], labels, stats, centroids);

		for (int i = 1; i < centroids.rows; i++)
		{
			Mat BoundingBoxTop = TransformPoint(Point2d((double)stats.at<int>(i, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) - CenterOffset.rows), HomographyMatrix);
			Point2d BoundingBoxTopPoint = Point2d(BoundingBoxTop.at<double>(0), BoundingBoxTop.at<double>(1));

			Mat BoundingBoxBottom = TransformPoint(Point2d((double)stats.at<int>(i, 0) - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)stats.at<int>(i, 3) - CenterOffset.rows), HomographyMatrix);
			Point2d BoundingBoxBottomPoint = Point2d(BoundingBoxBottom.at<double>(0), BoundingBoxBottom.at<double>(1));

			double RealHeight = sqrt(pow(BoundingBoxBottomPoint.x - BoundingBoxTopPoint.x, 2) + pow(BoundingBoxBottomPoint.y - BoundingBoxTopPoint.y, 2));

				Rect ObjectBoundingBox = Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3));
				Mat ObjectLabelRegion = labels(Rect(stats.at<int>(i, 0), stats.at<int>(i, 1), stats.at<int>(i, 2), stats.at<int>(i, 3)));

				Mat ObjectMask = Mat(ObjectLabelRegion.rows, ObjectLabelRegion.cols, CV_8UC1);

				for (int y = 0; y < ObjectLabelRegion.rows; y++)
				{
					for (int x = 0; x < ObjectLabelRegion.cols; x++)
					{
						if (ObjectLabelRegion.at<int>(y, x) == i)
						{
							ObjectMask.at<char>(y, x) = 255;
						}
						else
						{
							ObjectMask.at<char>(y, x) = 0;
						}
					}
				}

				Point2d ObjectImagePosition = Point2d((double)stats.at<int>(i, 0) + (double)stats.at<int>(i, 2) / 2 - CenterOffset.cols, (double)stats.at<int>(i, 1) + (double)stats.at<int>(i, 3) - CenterOffset.rows);

				Mat ObjectRealPositionMat = TransformPoint(ObjectImagePosition, HomographyMatrix);

				Point2d ObjectRealPosition = Point2d(ObjectRealPositionMat.at<double>(0), ObjectRealPositionMat.at<double>(1));

				int HistMaxValuePosition = GetHistogramMaxPosition(ColorImage(ObjectBoundingBox), ObjectMask);

				PlayerBoundingBoxes.push_back(ObjectBoundingBox);
				PlayerMasks.push_back(ObjectMask);
				PlayerImages.push_back(ColorImage(ObjectBoundingBox));
				PlayerPositions.push_back(ObjectRealPosition);
				PlayerMaxHistogramPositions.push_back(HistMaxValuePosition);
				PlayersTeam.push_back(PlayersClusterIds.at(j));
				PlayerRealHeights.push_back(RealHeight);
		}
	}

	delete[] BgFgImagesWithTeam;
}

vector<ImageObject*> ImageObjectClassifier::InitializePlayers()
{
	vector<ImageObject*> Players;

	for (int i = 0; i < PlayerPositions.size(); i++)
	{
		if (PlayerRealHeights.at(i) > MinPlayerHeight && PlayerRealHeights.at(i) < MaxPlayerHeight)
		{
			bool RemovedId = false;

			for (int j = 0; j < RemovedObjectIds.size(); j++)
			{
				if (i == RemovedObjectIds.at(j))
				{
					RemovedId = true;
				}
			}

			if (!RemovedId)
			{
				Player* player = new Player(i, PlayersTeam.at(i), PlayerBoundingBoxes.at(i), PlayerPositions.at(i), PlayerRealHeights.at(i), PlayerImages.at(i), PlayerMasks.at(i));
				player->SetGmmModel(GMM_ModelsForTeam[CurrentClusterNumber].em_model);
				player->SetMinPlayerHeight(MinPlayerHeight);
				player->SetMaxPlayerHeight(MaxPlayerHeight);

				Players.push_back(player);
			}
		}
	}

	return Players;
}
