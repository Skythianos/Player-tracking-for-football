#include "Panorama.h"



Panorama::Panorama()
{
}


Panorama::~Panorama()
{
}

void Panorama::Calibrate(vector<CoordWithWindow> ImagePoints, vector<CoordWithWindow> ObjectPoints)
{
	for (int i = 0; i < ImagePoints.size(); i++)
	{
		if (ImagePoints.at(i).Window == 0)
		{
			ImagePointsLeft.push_back(ImagePoints.at(i).Clickcoord);
			ObjectPointsLeft.push_back(ObjectPoints.at(i).Clickcoord);
		}

		if (ImagePoints.at(i).Window == 1)
		{
			ImagePointsCenter.push_back(ImagePoints.at(i).Clickcoord);
			ObjectPointsCenter.push_back(ObjectPoints.at(i).Clickcoord);
		}

		if (ImagePoints.at(i).Window == 2)
		{
			ImagePointsRight.push_back(ImagePoints.at(i).Clickcoord);
			ObjectPointsRight.push_back(ObjectPoints.at(i).Clickcoord);
		}
	}

	if (ImagePointsLeft.size() >= 4 && ObjectPointsLeft.size() >= 4)
	{
		Mat H = findHomography(ImagePointsLeft, ObjectPointsLeft, noArray(), 0);

		HomographyMatrixLeft.H = H;
		HomographyMatrixLeft.Calibrated = 1;
	}
	if (ImagePointsCenter.size() >= 4 && ObjectPointsCenter.size() >= 4)
	{
		Mat H = findHomography(ImagePointsCenter, ObjectPointsCenter, noArray(), 0);

		HomographyMatrixCenter.H = H;
		HomographyMatrixCenter.Calibrated = 1;
	}
	if (ImagePointsRight.size() >= 4 && ObjectPointsRight.size() >= 4)
	{
		Mat H = findHomography(ImagePointsRight, ObjectPointsRight, noArray(), 0);

		HomographyMatrixRight.H = H;
		HomographyMatrixRight.Calibrated = 1;
	}
}

Panorama::TransformedFrameBoarders Panorama::FindBoarders(Mat & src, vector<Point2f>& ImagePoints, Mat & SourceHomographyMatrix, const Mat & DestinationHomographyMatrix)
{
	TransformedFrameBoarders boarders;

	Mat onemat = Mat::eye(1, 1, SourceHomographyMatrix.type());

	int maxrow = 0;
	int maxcol = 0;
	int minrow = 0;
	int mincol = 0;
	int start = 1;

	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			Mat imageaffin;
			Mat ImagePointForTypeConvert, ObjectPointForTypeConvert;
			vector<Mat> concatenatetemp;

			Mat(Point2f(j, i)).convertTo(ImagePointForTypeConvert, SourceHomographyMatrix.type());

			concatenatetemp.push_back(ImagePointForTypeConvert);
			concatenatetemp.push_back(onemat);

			vconcat(concatenatetemp, imageaffin);

			Mat objectpointguess = SourceHomographyMatrix * imageaffin;

			double ratio = 1 / objectpointguess.at<double>(2);
			objectpointguess.at<double>(0) = objectpointguess.at<double>(0) * ratio;
			objectpointguess.at<double>(1) = objectpointguess.at<double>(1) * ratio;
			objectpointguess.at<double>(2) = objectpointguess.at<double>(2) * ratio;

			Mat(Point2d(objectpointguess.at<double>(0), objectpointguess.at<double>(1))).convertTo(ObjectPointForTypeConvert, DestinationHomographyMatrix.type());
			onemat = Mat::eye(1, 1, DestinationHomographyMatrix.type());

			concatenatetemp.clear();

			concatenatetemp.push_back(ObjectPointForTypeConvert);
			concatenatetemp.push_back(onemat);

			vconcat(concatenatetemp, imageaffin);

			Mat imagepointguess = DestinationHomographyMatrix * imageaffin;

			ratio = 1 / imagepointguess.at<double>(2);
			imagepointguess.at<double>(0) = imagepointguess.at<double>(0) * ratio;
			imagepointguess.at<double>(1) = imagepointguess.at<double>(1) * ratio;
			imagepointguess.at<double>(2) = imagepointguess.at<double>(2) * ratio;

			if (start == 1)
			{
				maxrow = minrow = cvRound(imagepointguess.at<double>(1));
				maxcol = mincol = cvRound(imagepointguess.at<double>(0));
				start = 0;
			}
			else
			{
				if (maxrow < cvRound(imagepointguess.at<double>(1)))
					maxrow = cvRound(imagepointguess.at<double>(1));
				if (maxcol < cvRound(imagepointguess.at<double>(0)))
					maxcol = cvRound(imagepointguess.at<double>(0));
				if (minrow > cvRound(imagepointguess.at<double>(1)))
					minrow = cvRound(imagepointguess.at<double>(1));
				if (mincol > cvRound(imagepointguess.at<double>(0)))
					mincol = cvRound(imagepointguess.at<double>(0));
			}
		}
	}

	boarders.minrow = minrow;
	boarders.mincol = mincol;
	boarders.maxrow = maxrow;
	boarders.maxcol = maxcol;

	return boarders;
}

Mat Panorama::GetPerspectiveTransformMatrix(Mat & src, vector<Point2f>& ImagePoints, Mat & SourceHomographyMatrix, const Mat & DestinationHomographyMatrix, Offset & offset)
{
	vector<Point2f> ImagePointsGuessVector;

	Mat onemat = Mat::eye(1, 1, SourceHomographyMatrix.type());

	for (int i = 0; i < ImagePoints.size(); i++)
	{
		Mat imageaffin;
		Mat ImagePointForTypeConvert, ObjectPointForTypeConvert;
		vector<Mat> concatenatetemp;

		Mat(ImagePoints.at(i)).convertTo(ImagePointForTypeConvert, SourceHomographyMatrix.type());

		concatenatetemp.push_back(ImagePointForTypeConvert);
		concatenatetemp.push_back(onemat);

		vconcat(concatenatetemp, imageaffin);

		Mat objectpointguess = SourceHomographyMatrix * imageaffin;

		double ratio = 1 / objectpointguess.at<double>(2);
		objectpointguess.at<double>(0) = objectpointguess.at<double>(0) * ratio;
		objectpointguess.at<double>(1) = objectpointguess.at<double>(1) * ratio;
		objectpointguess.at<double>(2) = objectpointguess.at<double>(2) * ratio;

		Mat(Point2d(objectpointguess.at<double>(0), objectpointguess.at<double>(1))).convertTo(ObjectPointForTypeConvert, DestinationHomographyMatrix.type());
		onemat = Mat::eye(1, 1, DestinationHomographyMatrix.type());

		concatenatetemp.clear();

		concatenatetemp.push_back(ObjectPointForTypeConvert);
		concatenatetemp.push_back(onemat);

		vconcat(concatenatetemp, imageaffin);

		Mat imagepointguess = DestinationHomographyMatrix * imageaffin;

		ratio = 1 / imagepointguess.at<double>(2);
		imagepointguess.at<double>(0) = imagepointguess.at<double>(0) * ratio;
		imagepointguess.at<double>(1) = imagepointguess.at<double>(1) * ratio;
		imagepointguess.at<double>(2) = imagepointguess.at<double>(2) * ratio;

		ImagePointsGuessVector.push_back(Point2f((float)imagepointguess.at<double>(0) - (float)offset.cols, (float)imagepointguess.at<double>(1) - (float)offset.rows));
	}

	Mat M = getPerspectiveTransform(ImagePoints, ImagePointsGuessVector);

	cout << M << endl;

	return M;
}

bool Panorama::CreatePanorama(Mat & left, Mat & center, Mat & right)
{
	TransformedFrameBoarders TransformedFrameBoardersLeft;
	TransformedFrameBoarders TransformedFrameBoardersRight;

	if (ImagePointsLeft.size() == 4 && ImagePointsRight.size() == 4 && HomographyMatrixLeft.Calibrated == true && HomographyMatrixCenter.Calibrated == true && HomographyMatrixRight.Calibrated == true)
	{
		TransformedFrameBoardersLeft = FindBoarders(left, ImagePointsLeft, HomographyMatrixLeft.H, HomographyMatrixCenter.H.inv());
		TransformedFrameBoardersRight = FindBoarders(right, ImagePointsRight, HomographyMatrixRight.H, HomographyMatrixCenter.H.inv());

		int PanoramaMinRow = min(TransformedFrameBoardersLeft.minrow, min(0, TransformedFrameBoardersRight.minrow));
		int PanoramaMaxRow = max(TransformedFrameBoardersLeft.maxrow, max(center.rows, TransformedFrameBoardersRight.maxrow));

		PanoramaSize = Size(TransformedFrameBoardersRight.maxcol - TransformedFrameBoardersLeft.mincol, PanoramaMaxRow - PanoramaMinRow);
		TransformedLeftSize = Size(TransformedFrameBoardersLeft.maxcol - TransformedFrameBoardersLeft.mincol, PanoramaMaxRow - PanoramaMinRow);
		TransformedRightSize = Size(TransformedFrameBoardersRight.maxcol - TransformedFrameBoardersRight.mincol, PanoramaMaxRow - PanoramaMinRow);
		CenterOffset.cols = abs(TransformedFrameBoardersLeft.mincol);
		CenterOffset.rows = abs(PanoramaMinRow);

		Offset OffsetLeft, OffsetRight;

		OffsetLeft.cols = TransformedFrameBoardersLeft.mincol;
		OffsetLeft.rows = PanoramaMinRow;

		OffsetRight.cols = TransformedFrameBoardersLeft.mincol;
		OffsetRight.rows = PanoramaMinRow;

		PerspectiveTransformMatrixLeft = GetPerspectiveTransformMatrix(left, ImagePointsLeft, HomographyMatrixLeft.H, HomographyMatrixCenter.H.inv(), OffsetLeft);
		PerspectiveTransformMatrixRight = GetPerspectiveTransformMatrix(right, ImagePointsRight, HomographyMatrixRight.H, HomographyMatrixCenter.H.inv(), OffsetRight);

		Mat templeft, dst;

		warpPerspective(left, templeft, PerspectiveTransformMatrixLeft, Size(TransformedFrameBoardersLeft.maxcol - TransformedFrameBoardersLeft.mincol, PanoramaMaxRow - PanoramaMinRow));

		warpPerspective(right, dst, PerspectiveTransformMatrixRight, Size(TransformedFrameBoardersRight.maxcol - TransformedFrameBoardersLeft.mincol, PanoramaMaxRow - PanoramaMinRow));

		templeft.copyTo(dst(Rect(0, 0, templeft.cols, templeft.rows)));

		center.copyTo(dst(Rect(abs(TransformedFrameBoardersLeft.mincol), abs(PanoramaMinRow), center.cols, center.rows)));

		namedWindow("Create Panorama result", CV_WINDOW_KEEPRATIO);
		resizeWindow("Create Panorama result", 640, 360);

		imshow("Create Panorama result", dst);
		waitKey(0);

		destroyWindow("Create Panorama result");

		return true;
	}
	else
	{
		return false;
	}
}

bool Panorama::GetPanorama(Mat & left, Mat & center, Mat & right, Mat & panorama)
{
	if (!PerspectiveTransformMatrixLeft.empty() && !PerspectiveTransformMatrixRight.empty() && TransformedLeftSize != Size(0,0) && PanoramaSize != Size(0, 0))
	{
		Mat templeft;

		warpPerspective(left, templeft, PerspectiveTransformMatrixLeft, TransformedLeftSize);

		warpPerspective(right, panorama, PerspectiveTransformMatrixRight, PanoramaSize);

		templeft.copyTo(panorama(Rect(0, 0, templeft.cols, templeft.rows)));

		center.copyTo(panorama(Rect(CenterOffset.cols, CenterOffset.rows, center.cols, center.rows)));

		return true;
	}
	else
	{
		return false;
	}
	
}

Mat Panorama::TransformPoint(Point2d Point, const Mat & HomographyMatrix)
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

bool Panorama::ImportFromFile(FileStorage File)
{
	File["HomographyMatrixLeft"] >> HomographyMatrixLeft.H;
	HomographyMatrixLeft.Calibrated = 1;

	File["HomographyMatrixCenter"] >> HomographyMatrixCenter.H;
	HomographyMatrixCenter.Calibrated = 1;

	File["HomographyMatrixRight"] >> HomographyMatrixRight.H;
	HomographyMatrixRight.Calibrated = 1;

	File["PerspectiveTransformMatrixLeft"] >> PerspectiveTransformMatrixLeft;
	File["PerspectiveTransformMatrixRight"] >> PerspectiveTransformMatrixRight;

	File["TransformedLeftSize"] >> TransformedLeftSize;
	File["TransformedRightSize"] >> TransformedRightSize;

	Size PanoramaSizeTemp;

	File["PanoramaSize"] >> PanoramaSizeTemp;

	PanoramaSize = Size(PanoramaSizeTemp.width, PanoramaSizeTemp.height - 1);

	Point CenterOffsetTemp;

	File["CenterOffset"] >> CenterOffsetTemp;

	CenterOffset.cols = CenterOffsetTemp.x;
	CenterOffset.rows = CenterOffsetTemp.y;

	return true;
}
