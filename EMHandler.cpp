#include "EMHandler.h"

EMHandler::EMHandler(Mat Frame, Mat Mask)
{
	this->Frame = Frame;
	this->Mask = Mask;

	Mat FrameHSV, FrameHSVFloat;

	GaussianBlur(this->Frame, this->Frame, Size(3, 3), 0);
	cvtColor(this->Frame, FrameHSV, COLOR_BGR2HSV);
	FrameHSV.convertTo(FrameHSVFloat, CV_32F);

	split(FrameHSVFloat, hsv);

	ChannelVector.push_back(hsv[0]);
	ChannelVector.push_back(hsv[1]);
	ChannelVector.push_back(hsv[2]);
}


EMHandler::~EMHandler()
{
}

Mat EMHandler::ConvertHueChannelAngles()
{
	Mat dst = Mat(ChannelVector.at(0).rows, ChannelVector.at(0).cols, CV_32FC2);

	for (int y = 0; y < ChannelVector.at(0).rows; y++)
	{
		float* row = ChannelVector.at(0).ptr<float>(y);

		for (int x = 0; x < ChannelVector.at(0).cols; x++)
		{
			if (Mask.at<uchar>(y, x) != 0)
			{
				float i, j;

				if ((2 * row[x] >= 0 && 2 * row[x] < 90) || (2 * row[x] > 270 && 2 * row[x] <= 358))
				{
					i = sqrtf(1 / (1 + tanf((2 * row[x]) * PI / 180) * tanf((2 * row[x]) * PI / 180)));
					j = tanf((2 * row[x]) * PI / 180) * i;
				}
				else if ((2 * row[x] > 90 && 2 * row[x] < 270))
				{
					i = -sqrtf(1 / (1 + tanf((2 * row[x]) * PI / 180) * tanf((2 * row[x]) * PI / 180)));
					j = tanf((2 * row[x]) * PI / 180) * i;
				}
				else if (2 * row[x] == 90)
				{
					i = 0;
					j = 1;
				}
				else if (2 * row[x] == 270)
				{
					i = 0;
					j = -1;
				}

				dst.at<Vec2f>(y, x) = Vec2f(i, j);
			}
		}
	}

	return dst;
}

Mat EMHandler::ConvertMatrixToVector(Mat &matrix)
{
	int masksize = 0;

	for (int y = 0; y < Mask.rows; y++)
	{
		uchar* row = Mask.ptr<uchar>(y);

		for (int x = 0; x < Mask.cols; x++)
		{
			if (Mask.at<uchar>(y, x) != 0)
			{
				masksize++;
			}
		}
	}


	Mat vector = Mat(masksize, 2, CV_32FC1);

	int idx = 0;

	for (int y = 0; y < matrix.rows; y++)
	{
		Vec2f* row = matrix.ptr<Vec2f>(y);

		for (int x = 0; x < matrix.cols; x++)
		{
			if (Mask.at<uchar>(y, x) != 0)
			{
				vector.at<Vec2f>(idx++, 0) = row[x];
			}
		}
	}

	return vector;
}

bool EMHandler::TrainEMModel(const int ClusterNumber, Ptr<EM> &em_model)
{
	bool result = true;

	Mat convertedframe = ConvertHueChannelAngles();
	Mat samples = ConvertMatrixToVector(convertedframe);

	em_model = EM::create();
	em_model->setClustersNumber(ClusterNumber);
	em_model->setCovarianceMatrixType(EM::COV_MAT_SPHERICAL);
	em_model->setTermCriteria(TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 300, 0.1));

	result = em_model->trainEM(samples);

	return result;
}

void EMHandler::ClassifyImage(Mat &FrameClassified, Mat &FrameGMM, const Ptr<EM> &em_model, const int ClusterNumber, const Rect roi)
{
	Mat sample = Mat(1, 2, CV_32FC1);

	Mat convertedframe = ConvertHueChannelAngles();

	for (int y = roi.y; y < roi.y + roi.height; y++)
	{
		Vec2f* row = convertedframe.ptr<Vec2f>(y);

		for (int x = roi.x; x < roi.x + roi.width; x++)
		{
			if (Mask.at<uchar>(y, x) != 0)
			{
				sample.at<float>(0, 0) = row[x][0];
				sample.at<float>(0, 1) = row[x][1];

				const int result = cvRound(em_model->predict(sample));

				Scalar c = colors[result];

				FrameGMM.at<Point3_<uchar>>(y, x) = Point3_<uchar>(c(0), c(1), c(2));
				FrameClassified.at<uchar>(y, x) = (uchar)result;
			}
		}
	}
}
