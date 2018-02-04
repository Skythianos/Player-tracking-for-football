#include "Camera.h"
#include "EMHandler.h"

Camera::Camera()
{
	for (int i = 0; i < 10; i++)
	{
		GMM_Models[i].trained = false;
	}

	CurrentClusterNumber = -1;
}

Camera::Camera(Mat Frame)
{
	Camera::Camera();

	this->Mask = Mat::ones(Frame.rows, Frame.cols, CV_8U) * 255;

	this->Frame = Frame;
}

Camera::Camera(Mat Frame, Mat Mask)
{
	Camera::Camera(Frame);

	this->Mask = Mask;
}

Camera::~Camera()
{
}

void Camera::SetFrame(Mat Frame)
{
	this->Frame = Frame;

	if (Mask.empty())
	{
		this->Mask = Mat::ones(Frame.rows, Frame.cols, CV_8U) * 255;
	}
}

void Camera::SetMask(Mat Mask)
{
	this->Mask = Mask;
}

bool Camera::ClassifyImage(Mat &FrameClassified, Mat &FrameGMM, const int ClusterNumber, const int ThreadNumber)
{
	bool result = true;
	double t;

	vector<thread> threads(ThreadNumber);
	const int sliceHeight = Frame.rows / ThreadNumber;
	// Az utolsó téglalap magassága eltérhet
	const int lastSliceHeight = Frame.rows - (ThreadNumber - 1) * sliceHeight;
	// A téglalapok szélessége a kép szélességének felel meg.
	const int width = Frame.cols;

	FrameClassified = Mat(Frame.rows, Frame.cols, CV_8U);
	FrameGMM = Mat(Frame.rows, Frame.cols, CV_8UC3);

	EMHandler* em = new EMHandler(Frame, Mask);

	Ptr<EM> em_model;

	CurrentClusterNumber = ClusterNumber;

	if (GMM_Models[ClusterNumber].trained == false)
	{
		t = (double)getTickCount();

		result = em->TrainEMModel(ClusterNumber, em_model);
	
		t = ((double)getTickCount() - t) / getTickFrequency();

		cout << "Trainig time: " << t << endl;

		if (result == true)
		{
			GMM_Models[ClusterNumber].em_model = em_model;

			GMM_Models[ClusterNumber].trained = true;
		}
	}
	else
	{
		em_model = GMM_Models[ClusterNumber].em_model;

		cout << "EM Model already trained." << endl;
	}

	Mat means = em_model->getMeans();
	Mat weights = em_model->getWeights();

	cout << means << endl;
	cout << weights << endl;

	if (result == true)
	{
		t = (double)getTickCount();

		// Minden szálra
		for (int i = 0; i < ThreadNumber; i++)
		{
			// A munkaterület
			Rect roi(0, i*sliceHeight, width, (i == ThreadNumber - 1 ? lastSliceHeight : sliceHeight));

			// A szál indítása        
			threads[i] = thread([em, &FrameClassified, &FrameGMM, &em_model, ClusterNumber, roi]() { em->ClassifyImage(FrameClassified, FrameGMM, em_model, ClusterNumber, roi); });
		}

		// A fõ szál nem tesz  mást, mint megvárja a többieket.
		for (int i = 0; i < ThreadNumber; i++)
		{
			threads[i].join();
		}

		t = ((double)getTickCount() - t) / getTickFrequency();

		cout << "Classification time: " << t << endl;
	}
	else
	{
		cout << "Trainig failed" << endl;
	}

	delete em;

	return result;
}

void Camera::CreateMask(vector<Point2f> ROIPoints, Point2f InitPoint)
{
	MaskLine temp;
	vector<MaskLine> masklines;

	if (ROIPoints.size() > 2)
	{
		ROIPoints.push_back(ROIPoints.at(0));
	}

	InitPoint.y = Frame.rows - InitPoint.y - 1;

	for (int i = 0; i < ROIPoints.size(); i++)
	{
		ROIPoints.at(i).y = Frame.rows - ROIPoints.at(i).y - 1;
	}

	for (int i = 0; i < ROIPoints.size() - 1; i++)
	{
		if (ROIPoints.at(i + 1).x - ROIPoints.at(i).x == 0)
		{
			temp.vertical = 1;
			temp.m = 0;
			temp.b = 0;
			temp.x = ROIPoints.at(i).x;

			if (ROIPoints.at(i).x < InitPoint.x)
			{
				temp.direction = 3;
			}
			else
			{
				temp.direction = 2;
			}
		}
		else
		{
			temp.vertical = 0;
			temp.m = (ROIPoints.at(i + 1).y - ROIPoints.at(i).y) / (ROIPoints.at(i + 1).x - ROIPoints.at(i).x);
			temp.b = ROIPoints.at(i).y - (temp.m * ROIPoints.at(i).x);
			temp.x = 0;

			if ((temp.m * InitPoint.x) + temp.b < InitPoint.y)
			{
				temp.direction = 1;
			}
			else
			{
				temp.direction = 0;
			}
		}

		masklines.push_back(temp);
	}

	if (ROIPoints.size() > 2)
	{
		ROIPoints.erase(ROIPoints.begin() + ROIPoints.size() - 1);
	}

	for (int y = 0; y < Frame.rows; y++)
	{
		for (int x = 0; x < Frame.cols; x++)
		{
			Mask.at<uchar>(Frame.rows - y - 1, x) = 255;
			for (int i = 0; i < masklines.size(); i++)
			{
				if (masklines.at(i).direction == 1)
				{
					if (y < (masklines.at(i).m * x) + masklines.at(i).b)
					{
						Mask.at<uchar>(Frame.rows - y - 1, x) = 0;
					}
				}
				if (masklines.at(i).direction == 0)
				{
					if (y >(masklines.at(i).m * x) + masklines.at(i).b)
					{
						Mask.at<uchar>(Frame.rows - y - 1, x) = 0;
					}
				}
				if (masklines.at(i).direction == 3)
				{
					if (x < masklines.at(i).x)
					{
						Mask.at<uchar>(Frame.rows - y - 1, x) = 0;
					}
				}
				if (masklines.at(i).direction == 2)
				{
					if (x > masklines.at(i).x)
					{
						Mask.at<uchar>(Frame.rows - y - 1, x) = 0;
					}
				}
			}
		}
	}
}

Mat Camera::GetConfigFrame()
{
	return Frame;
}

Mat Camera::GetMask()
{
	return Mask;
}

bool Camera::AddClusterToBackGround()
{
	bool result;

	if (CurrentClusterNumber != -1 && GMM_Models[CurrentClusterNumber].trained == true)
	{
		int Cluster;

		cout << "Cluster number = " << CurrentClusterNumber << endl;

		cout << "Choosable IDs: ";

		for (int i = 0; i < CurrentClusterNumber; i++)
		{
			int ClusterIsBackground = 0;
			for (int j = 0; j < BackGroundIds.size(); j++)
			{
				if (i == BackGroundIds.at(j))
				{
					ClusterIsBackground = 1;
				}
			}

			if (ClusterIsBackground == 0)
			{
				cout << i << ", ";
			}
		}

		cout << endl;

		cin >> Cluster;

		if (Cluster < CurrentClusterNumber)
		{
			BackGroundIds.push_back(Cluster);
		}

		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}

bool Camera::RemoveClusterFromBackGround()
{
	bool result;

	if (CurrentClusterNumber != -1 && GMM_Models[CurrentClusterNumber].trained == true)
	{
		int Cluster;

		cout << "Cluster number = " << CurrentClusterNumber << endl;

		cout << "Choosable IDs: ";

		for (int i = 0; i < CurrentClusterNumber; i++)
		{
			int ClusterIsBackground = 0;
			for (int j = 0; j < BackGroundIds.size(); j++)
			{
				if (i == BackGroundIds.at(j))
				{
					ClusterIsBackground = 1;
				}
			}

			if (ClusterIsBackground == 1)
			{
				cout << i << ", ";
			}
		}

		cout << endl;

		cin >> Cluster;

		for (int j = 0; j < BackGroundIds.size(); j++)
		{
			if (Cluster == BackGroundIds.at(j))
			{
				BackGroundIds.erase(BackGroundIds.begin() + j);
			}
		}

		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}

vector<int> Camera::GetBackGroundIds()
{
	return BackGroundIds;
}

void Camera::CreateBgFgImage(Mat &FrameClassified, Mat &BgFgImage)
{
	for (int y = 0; y < BgFgImage.rows; y++)
	{
		for (int x = 0; x < BgFgImage.cols; x++)
		{
			if (Mask.at<uchar>(y, x) != 0)
			{
				BgFgImage.at<uchar>(y, x) = 255;

				for (int i = 0; i < GetBackGroundIds().size(); i++)
				{
					if ((uchar)GetBackGroundIds().at(i) == FrameClassified.at<uchar>(y, x))
					{
						BgFgImage.at<uchar>(y, x) = 0;
					}
				}
			}
		}
	}
}

int Camera::GetCurrentClusterNumber()
{
	return CurrentClusterNumber;
}