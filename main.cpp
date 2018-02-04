
#include "Globals.h"
#include "Camera.h"
#include "Panorama.h"
#include "VirtualFootballPitch.h"
#include "ImageObjectClassifier.h"
#include "ObjectTracker.h"
#include "ImageObject.h"
#include "Player.h"

int pause = 0;
int PauseForMenuActions = 0;

MenuState MainMenu;
ProgramState State;

double MinPlayerHeight;
double MaxPlayerHeight;

Mat leftframe;
Mat centerframe;
Mat rightframe;
Mat leftframegmm;
Mat centerframegmm;
Mat rightframegmm;
Mat leftframebgfg;
Mat centerframebgfg;
Mat rightframebgfg;
Mat leftframeclassified;
Mat centerframeclassified;
Mat rightframeclassified;

Mat panoramaframe;
Mat panoramaframegmm;
Mat panoramaframebgfg;
Mat panoramaframeclassified;

Mat ROIMaskLeft;
Mat ROIMaskCenter;
Mat ROIMaskRight;
Mat ROIMaskPanorama;

int StartBgFgVideo = 0;
int CutROIMode = 0;
int CreateROIMask = 0;
int StartSetPanorama = 0;
int RemoveObjectFromProbablePlayers = 0;
int AddObjectToProbablePlayers = 0;
int PlayersInit = 0;
int ShowDebugPictures = 0;
extern int ShowDebugPictures;

vector<CoordWithWindow> clickcoord;
vector<CoordWithWindow> InitPoints;

vector<Point2f> ROIPointsLeft;
vector<Point2f> ROIPointsCenter;
vector<Point2f> ROIPointsRight;
vector<Point2f> ROIPointsPanorama;

Camera LeftCameraConfig;
Camera CenterCameraConfig;
Camera RightCameraConfig;

vector<CoordWithWindow> ImagePointsForCreatePanorama;
vector<CoordWithWindow> ObjectPointsForCreatePanorama;

Panorama PanoramaVideo;
Camera PanoramaConfig;

void ClusterNumberForInitPlayerHandler(int position, void *userdata);

VirtualFootballPitch VirtualPitch;

ImageObjectClassifier ObjectClassifier;

VirtualFootballPitch VirtualPitchForInitPlayer;

int ClusterNumberForPlayerInitTrack = 2;
int ClusterNumberForPlayerInitMaximum = 10;
Size PitchSizeForPlayerInit;

vector<ImageObject*> Players;
ObjectTracker Tracker;

void VirtualPitchForInitPlayerMouseCallback(int event, int x, int y, int flags, void *userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		Point2d ObjectPosition;

		ObjectPosition.x = (double)(x - 90) / 12;
		ObjectPosition.y = (double)(1080 - y - 90) / 12;

		if (AddObjectToProbablePlayers)
		{
			ObjectClassifier.AddObjectToProbablePlayers(ObjectPosition);
		}
		else if (RemoveObjectFromProbablePlayers)
		{
			ObjectClassifier.RemoveObjectFromProbablePlayers(ObjectPosition);
		}

		VirtualPitchForInitPlayer.ClearRemovedObjectIds();

		VirtualPitchForInitPlayer.SetRemovedObjectIds(ObjectClassifier.GetRemovedObjectIds());

		imshow("Virtual football pitch for initializa players", VirtualPitchForInitPlayer.GetFootballPitchFrame());

		waitKey(100);
	}
}

void onMouseCallback(int event, int x, int y, int flags, void *userdata)
{
	CoordWithWindow temp;
	
	int position = *(int*)userdata;

	if (event == EVENT_LBUTTONDOWN)
	{
		temp.Clickcoord.x = x;
		temp.Clickcoord.y = y;

		clickcoord.clear();
		temp.Window = position;

		clickcoord.push_back(temp);
	}
	if (event == EVENT_RBUTTONDOWN)
	{
		if (!pause)
			pause = 1;
		else
			pause = 0;
	}

	if (event == EVENT_RBUTTONDBLCLK)
	{
		int mode;

		switch (MainMenu.Menu)
		{
		case 0:
			cout << "1: Add cluster to background\r\n" <<
				"2: Remove cluster from backgorund\r\n" <<
				"3: Back/foreground video\r\n" <<
				"4: Debug pictures\r\n" <<
				"5: Mask\r\n" <<
				"6: Panorama\r\n" <<
				"7: Initialize players\r\n";

			cin >> mode;

			break;

		case 1:

			cout << "Add cluster mode" << endl;

			cout << "1: Left\r\n" <<
				"2: Center\r\n" <<
				"3: Right\r\n" <<
				"4: Panorama\r\n" << 
				"5: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;

		case 2:

			cout << "Remove cluster mode" << endl;

			cout << "1: Left\r\n" <<
				"2: Center\r\n" <<
				"3: Right\r\n" <<
				"4: Panorama\r\n" <<
				"5: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;

		case 3:

			cout << "Back/Foreground video mode" << endl;

			cout << "1: Start\r\n" <<
				"2: Stop\r\n" <<
				"3: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;

		case 4:

			cout << "Debug pictures mode" << endl;

			cout << "1: Show\r\n" <<
				"2: Hide\r\n" << 
				"3: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;

		case 5:

			cout << "Mask mode" << endl;

			cout << "1: Add cutting point\r\n" <<
				"2: Add init point\r\n" <<
				"3: Create Mask\r\n" <<
				"4: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;

		case 6:

			cout << "Panorama mode" << endl;

			cout << "1: Add image points\r\n" <<
				"2: Create panorama\r\n" << 
				"3: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;

		case 7:

			cout << "Initialize players mode" << endl;

			cout << "1: Set cluster number\r\n" <<
				"2: Add object to players\r\n" <<
				"3: Remove object from players\r\n" <<
				"4: Update players with team information\r\n" <<
				"5: Initialize players\r\n" << 
				"6: Exit\r\n";

			cin >> MainMenu.SubMenu;

			break;
		}

		if (MainMenu.Menu == 0)
		{
			MainMenu.Menu = mode;
		}

		if (MainMenu.Menu == 5)
		{
			State.CutROIMode = 1;
		}
		else if (MainMenu.Menu == 6)
		{
			State.StartSetPanorama = 1;
		}
	}
}

void ClusterNumberForInitPlayerHandler(int position, void *userdata)
{
	ObjectClassifier.ClassifyPlayers(position);

	vector<Point2d> ProbablePlayerPositions = ObjectClassifier.GetPlayerPositions();
	vector<int> ProbablePlayersTeam = ObjectClassifier.GetPLayersTeam();
	vector<int> RemovedObjectsFromProbablePlayers = ObjectClassifier.GetRemovedObjectIds();

	vector<ImageObject*> ProbablePlayers;

	for (int i = 0; i < ProbablePlayerPositions.size(); i++)
	{
		Player* probableplayer = new Player(i, ProbablePlayersTeam.at(i), Rect(0, 0, 0, 0), ProbablePlayerPositions.at(i), 0, Mat(), Mat());

		ProbablePlayers.push_back(probableplayer);
	}

	VirtualPitchForInitPlayer.ClearPlayers();
	VirtualPitchForInitPlayer.ClearRemovedObjectIds();
	VirtualPitchForInitPlayer.AddPlayers(ProbablePlayers);
	VirtualPitchForInitPlayer.SetRemovedObjectIds(RemovedObjectsFromProbablePlayers);

	Mat panoramaframetemp = panoramaframe.clone();

	vector<Rect> PlayerBoundingBoxesOnPanorama = ObjectClassifier.GetPlayerBoundingBoxes();

	for (int i = 0; i < PlayerBoundingBoxesOnPanorama.size(); i++)
	{
		rectangle(panoramaframetemp, PlayerBoundingBoxesOnPanorama.at(i), Scalar(0, 0, 255), 5);
	}

	imshow("panorama", panoramaframetemp);
	imshow("Virtual football pitch for initializa players", VirtualPitchForInitPlayer.GetFootballPitchFrame());

	waitKey(0);

	for (int i = 0; i < ProbablePlayerPositions.size(); i++)
	{
		delete ProbablePlayers.at(i);
	}

	ProbablePlayers.clear();
}

void onGMMClusterNumberTrackBarHandle(int position, void *userdata)
{
	int WindowPoisiton = *(int*)userdata;

	if (WindowPoisiton == 0)
	{
		LeftCameraConfig.ClassifyImage(leftframeclassified, leftframegmm, position, 1);

		imshow("left gmm", leftframegmm);
	}
	else if (WindowPoisiton == 1)
	{
		CenterCameraConfig.SetFrame(centerframe);

		CenterCameraConfig.ClassifyImage(centerframeclassified, centerframegmm, position, 1);

		imshow("center gmm", centerframegmm);
	}
	else if (WindowPoisiton == 2)
	{
		RightCameraConfig.SetFrame(rightframe);

		RightCameraConfig.ClassifyImage(rightframeclassified, rightframegmm, position, 1);

		imshow("right gmm", rightframegmm);
	}
	else if (WindowPoisiton == 3)
	{
		PanoramaConfig.SetFrame(panoramaframe);

		PanoramaConfig.ClassifyImage(panoramaframeclassified, panoramaframegmm, position, 1);

		imshow("panorama gmm", panoramaframegmm);
	}
}

void PlayerHeightTrackbarHandler(int position, void *userdata)
{
	int id = *(int*)userdata;

	if (id == 0)
	{
		MinPlayerHeight = (double)position;
		ObjectClassifier.SetMinPlayerHeight(MinPlayerHeight);
	}
	else if (id == 1)
	{
		MaxPlayerHeight = (double)position;
		ObjectClassifier.SetMaxPlayerHeight(MaxPlayerHeight);
	}
}

int main(int argc, char** argv)
{
	namedWindow("left", CV_WINDOW_KEEPRATIO);
	namedWindow("center", CV_WINDOW_KEEPRATIO);
	namedWindow("right", CV_WINDOW_KEEPRATIO);

	namedWindow("left gmm", CV_WINDOW_KEEPRATIO);
	namedWindow("center gmm", CV_WINDOW_KEEPRATIO);
	namedWindow("right gmm", CV_WINDOW_KEEPRATIO);

	namedWindow("left bgfg", CV_WINDOW_KEEPRATIO);
	namedWindow("center bgfg", CV_WINDOW_KEEPRATIO);
	namedWindow("right bgfg", CV_WINDOW_KEEPRATIO);
	
	namedWindow("Control", CV_WINDOW_KEEPRATIO);

	namedWindow("panorama", CV_WINDOW_KEEPRATIO);
	namedWindow("panorama gmm", CV_WINDOW_KEEPRATIO);
	namedWindow("panorama bgfg", CV_WINDOW_KEEPRATIO);

	namedWindow("Virtual football pitch", CV_WINDOW_KEEPRATIO);

	int LeftFrameClusterNumber = 2;
	int CenterFrameClusterNumber = 3;
	int RightFrameClusterNumber = 3;
	int PanoramaFrameClusterNumber = 3;
	int LeftFrameClusterMaximumNumber = 10;
	int CenterFrameClusterMaximumNumber = 10;
	int RightFrameClusterMaximumNumber = 10;
	int PanoramaFrameClusterMaximumNumber = 10;
	int LeftWindowPosition = 0;
	int CenterWindowPosition = 1;
	int RightWindowPosition = 2;
	int PanoramaWindowPosition = 3;

	createTrackbar("GMM Cluster Number", "left gmm", &LeftFrameClusterNumber, LeftFrameClusterMaximumNumber, onGMMClusterNumberTrackBarHandle, &LeftWindowPosition);
	createTrackbar("GMM Cluster Number", "center gmm", &CenterFrameClusterNumber, CenterFrameClusterMaximumNumber, onGMMClusterNumberTrackBarHandle, &CenterWindowPosition);
	createTrackbar("GMM Cluster Number", "right gmm", &RightFrameClusterNumber, RightFrameClusterMaximumNumber, onGMMClusterNumberTrackBarHandle, &RightWindowPosition);
	createTrackbar("GMM Cluster Number", "panorama gmm", &PanoramaFrameClusterNumber, PanoramaFrameClusterMaximumNumber, onGMMClusterNumberTrackBarHandle, &PanoramaWindowPosition);

	int MinPlayerHeightTrack = 10;
	int MaxPlayerHeightTrack = 30;
	int MinPlayerHeightMaximum = 100;
	int MaxPlayerHeightMaximum = 100;
	int MinPlayerHeightId = 0;
	int MaxPlayerHeightId = 1;

	createTrackbar("Minimum Player Height", "Control", &MinPlayerHeightTrack, MinPlayerHeightMaximum, PlayerHeightTrackbarHandler, &MinPlayerHeightId);
	createTrackbar("Maximum Player Height", "Control", &MaxPlayerHeightTrack, MaxPlayerHeightMaximum, PlayerHeightTrackbarHandler, &MaxPlayerHeightId);

	int positionleft = 0;
	int positioncenter = 1;
	int positionright = 2;
	int positionpanorama = 3;

	setMouseCallback("left", onMouseCallback, &positionleft);
	setMouseCallback("center", onMouseCallback, &positioncenter);
	setMouseCallback("right", onMouseCallback, &positionright);
	setMouseCallback("panorama", onMouseCallback, &positionpanorama);

	String VideoFilesLocation = String(argv[1]);
	String VideoFileLeft = VideoFilesLocation + String(argv[2]);
	String VideoFileCenter = VideoFilesLocation + String(argv[3]);
	String VideoFileRight = VideoFilesLocation + String(argv[4]);

	VideoCapture left = VideoCapture(VideoFileLeft);
	VideoCapture center = VideoCapture(VideoFileCenter);
	VideoCapture right = VideoCapture(VideoFileRight);

	left.read(leftframe);
	center.read(centerframe);
	right.read(rightframe);

	leftframebgfg = Mat(leftframe.rows, leftframe.cols, CV_8U);
	centerframebgfg = Mat(centerframe.rows, centerframe.cols, CV_8U);
	rightframebgfg = Mat(rightframe.rows, rightframe.cols, CV_8U);
	leftframeclassified = Mat(leftframe.rows, leftframe.cols, CV_8U);
	centerframeclassified = Mat(centerframe.rows, centerframe.cols, CV_8U);
	rightframeclassified = Mat(rightframe.rows, rightframe.cols, CV_8U);

	Mat leftframeROI = Mat::ones(leftframe.rows, leftframe.cols, leftframe.type()) * 255;
	Mat centerframeROI = Mat::ones(centerframe.rows, centerframe.cols, centerframe.type()) * 255;
	Mat rightframeROI = Mat::ones(rightframe.rows, rightframe.cols, rightframe.type()) * 255;
	Mat panoramaframeROI;

	Mat leftframeROIDisplay = Mat::ones(leftframe.rows, leftframe.cols, leftframe.type()) * 255;
	Mat centerframeROIDisplay = Mat::ones(centerframe.rows, centerframe.cols, centerframe.type()) * 255;
	Mat rightframeROIDisplay = Mat::ones(rightframe.rows, rightframe.cols, rightframe.type()) * 255;
	Mat panoramaframeROIDisplay;

	ROIMaskLeft = Mat::ones(leftframe.rows, leftframe.cols, CV_8U) * 255;
	ROIMaskCenter = Mat::ones(centerframe.rows, centerframe.cols, CV_8U) * 255;
	ROIMaskRight = Mat::ones(rightframe.rows, rightframe.cols, CV_8U) * 255;

	LeftCameraConfig = Camera();
	CenterCameraConfig = Camera();
	RightCameraConfig = Camera();

	PanoramaVideo = Panorama();
	PanoramaConfig = Camera();
	int PanoramaMatSet = 0;

	VirtualPitch = VirtualFootballPitch();

	MinPlayerHeight = 10;
	MaxPlayerHeight = 30;

	Tracker = ObjectTracker();

	MainMenu.Menu = 0;
	MainMenu.SubMenu = 0;

	CoordWithWindow temp;
	vector<ImageObject*> ProbablePlayers;
	State.CutROIMode = 0;
	State.PlayersInit = 0;
	State.StartSetPanorama = 0;

	while (1)
	{
		switch (MainMenu.Menu)
		{
		case 0:
			break;

		case 1:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Left" << endl;

				if (!LeftCameraConfig.AddClusterToBackGround())
				{
					cout << "Left frame is not trained!" << endl;
				}
				else
				{
					LeftCameraConfig.CreateBgFgImage(leftframeclassified, leftframebgfg);

					imshow("left bgfg", leftframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Center" << endl;

				if (!CenterCameraConfig.AddClusterToBackGround())
				{
					cout << "Center frame is not trained!" << endl;
				}
				else
				{
					CenterCameraConfig.CreateBgFgImage(centerframeclassified, centerframebgfg);

					imshow("center bgfg", centerframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Right" << endl;

				if (!RightCameraConfig.AddClusterToBackGround())
				{
					cout << "Right frame is not trained!" << endl;
				}
				else
				{
					RightCameraConfig.CreateBgFgImage(rightframeclassified, rightframebgfg);

					imshow("right bgfg", rightframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 4:
				cout << "Panorama" << endl;

				if (!PanoramaConfig.AddClusterToBackGround())
				{
					cout << "Panorama frame is not trained!" << endl;
				}
				else
				{
					PanoramaConfig.CreateBgFgImage(panoramaframeclassified, panoramaframebgfg);

					imshow("panorama bgfg", panoramaframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 5:
				cout << "Exit" << endl;
				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		case 2:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Left" << endl;

				if (!LeftCameraConfig.RemoveClusterFromBackGround())
				{
					cout << "Left frame is not trained!" << endl;
				}
				else
				{
					LeftCameraConfig.CreateBgFgImage(leftframeclassified, leftframebgfg);

					imshow("left bgfg", leftframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Center" << endl;

				if (!CenterCameraConfig.RemoveClusterFromBackGround())
				{
					cout << "Center frame is not trained!" << endl;
				}
				else
				{
					CenterCameraConfig.CreateBgFgImage(centerframeclassified, centerframebgfg);

					imshow("center bgfg", centerframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Right" << endl;

				if (!RightCameraConfig.RemoveClusterFromBackGround())
				{
					cout << "Right frame is not trained!" << endl;
				}
				else
				{
					RightCameraConfig.CreateBgFgImage(rightframeclassified, rightframebgfg);

					imshow("right bgfg", rightframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 4:
				cout << "Panorama" << endl;

				if (!PanoramaConfig.RemoveClusterFromBackGround())
				{
					cout << "Panorama frame is not trained!" << endl;
				}
				else
				{
					PanoramaConfig.CreateBgFgImage(panoramaframeclassified, panoramaframebgfg);

					imshow("panorama bgfg", panoramaframebgfg);
				}

				MainMenu.SubMenu = 0;
				break;

			case 5:
				cout << "Exit" << endl;
				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		case 3:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Start" << endl;

				StartBgFgVideo = 1;

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Stop" << endl;

				StartBgFgVideo = 0;

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Exit" << endl;
				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		case 4:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Show" << endl;

				ShowDebugPictures = 1;

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Hide" << endl;

				ShowDebugPictures = 0;

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Exit" << endl;
				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		case 5:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Add cutting point" << endl;

				PauseForMenuActions = 1;

				if (clickcoord.back().Window == 0)
				{
					ROIPointsLeft.push_back(clickcoord.back().Clickcoord);
				}
				if (clickcoord.back().Window == 1)
				{
					ROIPointsCenter.push_back(clickcoord.back().Clickcoord);
				}
				if (clickcoord.back().Window == 2)
				{
					ROIPointsRight.push_back(clickcoord.back().Clickcoord);
				}
				if (clickcoord.back().Window == 3)
				{
					ROIPointsPanorama.push_back(clickcoord.back().Clickcoord);
				}

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Add init point" << endl;

				InitPoints.push_back(clickcoord.back());

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Create mask" << endl;

				CreateROIMask = 1;

				PauseForMenuActions = 0;

				MainMenu.SubMenu = 0;
				break;

			case 4:
				cout << "Exit" << endl;

				State.CutROIMode = 0;

				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		case 6:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Add image point" << endl;

				if (!clickcoord.empty())
				{
					ImagePointsForCreatePanorama.push_back(clickcoord.back());
				}

				temp.Window = clickcoord.back().Window;

				cout << "x: ";

				cin >> temp.Clickcoord.x;

				cout << "y: ";

				cin >> temp.Clickcoord.y;

				ObjectPointsForCreatePanorama.push_back(temp);

				cout << "ImagePoint: " << clickcoord.back().Clickcoord << endl;
				cout << "ObjectPoint: " << temp.Clickcoord << endl;

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Create panorama" << endl;

				PanoramaVideo.Calibrate(ImagePointsForCreatePanorama, ObjectPointsForCreatePanorama);
				PanoramaVideo.CreatePanorama(leftframe, centerframe, rightframe);

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Exit" << endl;

				State.StartSetPanorama = 0;

				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		case 7:

			switch (MainMenu.SubMenu)
			{
			case 0:
				break;

			case 1:
				cout << "Set cluster number" << endl;

				PauseForMenuActions = 1;

				ObjectClassifier = ImageObjectClassifier(panoramaframebgfg, panoramaframe);
				ObjectClassifier.ClearVectors();
				ObjectClassifier.FindProbablePlayers(PanoramaVideo.HomographyMatrixCenter.H, PanoramaVideo.CenterOffset);

				RemoveObjectFromProbablePlayers = 0;
				AddObjectToProbablePlayers = 0;

				namedWindow("Virtual football pitch for initializa players", WINDOW_KEEPRATIO);
				setMouseCallback("Virtual football pitch for initializa players", VirtualPitchForInitPlayerMouseCallback);
				createTrackbar("Cluster number", "Virtual football pitch for initializa players", &ClusterNumberForPlayerInitTrack, ClusterNumberForPlayerInitMaximum, &ClusterNumberForInitPlayerHandler);

				if (!ObjectPointsForCreatePanorama.empty())
				{
					PitchSizeForPlayerInit.width = ObjectPointsForCreatePanorama.at(0).Clickcoord.x;
					PitchSizeForPlayerInit.height = ObjectPointsForCreatePanorama.at(0).Clickcoord.y;

					for (int i = 0; i < ObjectPointsForCreatePanorama.size(); i++)
					{
						if (ObjectPointsForCreatePanorama.at(i).Clickcoord.x > PitchSizeForPlayerInit.width)
						{
							PitchSizeForPlayerInit.width = ObjectPointsForCreatePanorama.at(i).Clickcoord.x;
						}

						if (ObjectPointsForCreatePanorama.at(i).Clickcoord.y > PitchSizeForPlayerInit.height)
						{
							PitchSizeForPlayerInit.height = ObjectPointsForCreatePanorama.at(i).Clickcoord.y;
						}
					}
				}

				VirtualPitchForInitPlayer = VirtualFootballPitch();

				VirtualPitchForInitPlayer.SetPitchSize(PitchSizeForPlayerInit);

				imshow("Virtual football pitch for initializa players", VirtualPitchForInitPlayer.GetFootballPitchFrame());

				waitKey(100);

				MainMenu.SubMenu = 0;
				break;

			case 2:
				cout << "Add object to players" << endl;

				RemoveObjectFromProbablePlayers = 0;
				AddObjectToProbablePlayers = 1;

				MainMenu.SubMenu = 0;
				break;

			case 3:
				cout << "Remove object from players" << endl;

				RemoveObjectFromProbablePlayers = 1;
				AddObjectToProbablePlayers = 0;

				MainMenu.SubMenu = 0;
				break;

			case 4:
				cout << "Update players with team information" << endl;

				ObjectClassifier.UpdateProbablePlayersWithTeamInformation(PanoramaVideo.HomographyMatrixCenter.H, PanoramaVideo.CenterOffset);

				for (int i = 0; i < ObjectClassifier.GetPlayerPositions().size(); i++)
				{
					if (ObjectClassifier.GetPlayerRealHeights().at(i) > ObjectClassifier.GetMinPlayerHeight() && ObjectClassifier.GetPlayerRealHeights().at(i) < ObjectClassifier.GetMaxPlayerHeight())
					{
						Player* probableplayer = new Player(i, ObjectClassifier.GetPLayersTeam().at(i), Rect(0, 0, 0, 0), ObjectClassifier.GetPlayerPositions().at(i), 0, Mat(), Mat());

						ProbablePlayers.push_back(probableplayer);
					}
				}

				VirtualPitchForInitPlayer.ClearPlayers();
				VirtualPitchForInitPlayer.ClearRemovedObjectIds();
				VirtualPitchForInitPlayer.AddPlayers(ProbablePlayers);
				VirtualPitchForInitPlayer.SetRemovedObjectIds(ObjectClassifier.GetRemovedObjectIds());

				imshow("Virtual football pitch for initializa players", VirtualPitchForInitPlayer.GetFootballPitchFrame());

				waitKey(100);

				for (int i = 0; i < ProbablePlayers.size(); i++)
				{
					delete ProbablePlayers.at(i);
				}

				ProbablePlayers.clear();

				MainMenu.SubMenu = 0;
				break;

			case 5:
				cout << "Initialize players" << endl;

				PauseForMenuActions = 0;

				Players = ObjectClassifier.InitializePlayers();

				VirtualPitchForInitPlayer.ClearPlayers();
				VirtualPitchForInitPlayer.ClearRemovedObjectIds();

				RemoveObjectFromProbablePlayers = 0;
				AddObjectToProbablePlayers = 0;

				VirtualPitchForInitPlayer.AddPlayers(Players);

				imshow("Virtual football pitch for initializa players", VirtualPitchForInitPlayer.GetFootballPitchFrame());

				waitKey(0);

				destroyWindow("Virtual football pitch for initializa players");

				for (int i = 0; i < Players.size(); i++)
				{
					Tracker.AddObjects(Players.at(i));
				}

				Tracker.SetGmmModel(ObjectClassifier.GetCurrentEMModel());

				PlayersInit = 1;

				MainMenu.SubMenu = 0;
				break;

			case 6:
				cout << "Exit" << endl;
				MainMenu.Menu = 0;
				MainMenu.SubMenu = 0;
				break;

			}

			break;

		}

		if (!pause && !PauseForMenuActions)
		{
			if (!left.read(leftframe))
				break;

			if (!center.read(centerframe))
				break;

			if (!right.read(rightframe))
				break;

			if (PanoramaVideo.GetPanorama(leftframe, centerframe, rightframe, panoramaframe))
			{
				if (!PanoramaMatSet)
				{
					panoramaframeROI = Mat::ones(panoramaframe.rows, panoramaframe.cols, panoramaframe.type()) * 255;

					panoramaframeROIDisplay = Mat::ones(panoramaframe.rows, panoramaframe.cols, panoramaframe.type()) * 255;

					ROIMaskPanorama = Mat::ones(panoramaframe.rows, panoramaframe.cols, CV_8U) * 255;

					panoramaframebgfg = Mat(panoramaframe.rows, panoramaframe.cols, CV_8U);

					panoramaframeclassified = Mat(panoramaframe.rows, panoramaframe.cols, CV_8U);

					PanoramaMatSet = 1;
				}

				imshow("panorama", panoramaframe);
			}

			if (State.StartSetPanorama == 1)
			{
				if (!clickcoord.empty())
				{
					CoordWithWindow temp = clickcoord.back();
					if (temp.Window == 0)
					{
						circle(leftframe, temp.Clickcoord, 10, Scalar(0, 0, 255));
					}
					else if (temp.Window == 1)
					{
						circle(centerframe, temp.Clickcoord, 10, Scalar(0, 0, 255));
					}
					else if (temp.Window == 2)
					{
						circle(rightframe, temp.Clickcoord, 10, Scalar(0, 0, 255));
					}
				}

				if (!ImagePointsForCreatePanorama.empty())
				{
					for (int i = 0; i < ImagePointsForCreatePanorama.size(); i++)
					{
						if (ImagePointsForCreatePanorama.at(i).Window == 0)
						{
							circle(leftframe, ImagePointsForCreatePanorama.at(i).Clickcoord, 10, Scalar(0, 255, 255));
						}
						else if (ImagePointsForCreatePanorama.at(i).Window == 1)
						{
							circle(centerframe, ImagePointsForCreatePanorama.at(i).Clickcoord, 10, Scalar(0, 255, 255));
						}
						else if (ImagePointsForCreatePanorama.at(i).Window == 2)
						{
							circle(rightframe, ImagePointsForCreatePanorama.at(i).Clickcoord, 10, Scalar(0, 255, 255));
						}
					}
				}
			}

			if (State.CutROIMode == 1)
			{
				leftframeROI = leftframe.clone();
				centerframeROI = centerframe.clone();
				rightframeROI = rightframe.clone(); 
				panoramaframeROI = panoramaframe.clone();

				if (clickcoord.size() != 0)
				{
					if (clickcoord.at(0).Window == 0)
					{
						circle(leftframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsLeft.size() > 0)
						{
							line(leftframeROI, ROIPointsLeft.at(ROIPointsLeft.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
					if (clickcoord.at(0).Window == 1)
					{
						circle(centerframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsCenter.size() > 0)
						{
							line(centerframeROI, ROIPointsCenter.at(ROIPointsCenter.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
					if (clickcoord.at(0).Window == 2)
					{
						circle(rightframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsRight.size() > 0)
						{
							line(rightframeROI, ROIPointsRight.at(ROIPointsRight.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
					if (clickcoord.at(0).Window == 3)
					{
						circle(panoramaframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsPanorama.size() > 0)
						{
							line(panoramaframeROI, ROIPointsPanorama.at(ROIPointsPanorama.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
				}

				for (int i = 0; i < ROIPointsLeft.size(); i++)
				{
					if (i != 0)
					{
						line(leftframeROI, ROIPointsLeft.at(i - 1), ROIPointsLeft.at(i), Scalar(0, 0, 255));
					}

					circle(leftframeROI, ROIPointsLeft.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsLeft.size() > 2)
				{
					line(leftframeROI, ROIPointsLeft.at(0), ROIPointsLeft.at(ROIPointsLeft.size() - 1), Scalar(0, 0, 255));
				}

				for (int i = 0; i < ROIPointsCenter.size(); i++)
				{
					if (i != 0)
					{
						line(centerframeROI, ROIPointsCenter.at(i - 1), ROIPointsCenter.at(i), Scalar(0, 0, 255));
					}

					circle(centerframeROI, ROIPointsCenter.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsCenter.size() > 2)
				{
					line(centerframeROI, ROIPointsCenter.at(0), ROIPointsCenter.at(ROIPointsCenter.size() - 1), Scalar(0, 0, 255));
				}

				for (int i = 0; i < ROIPointsRight.size(); i++)
				{
					if (i != 0)
					{
						line(rightframeROI, ROIPointsRight.at(i - 1), ROIPointsRight.at(i), Scalar(0, 0, 255));
					}

					circle(rightframeROI, ROIPointsRight.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsRight.size() > 2)
				{
					line(rightframeROI, ROIPointsRight.at(0), ROIPointsRight.at(ROIPointsRight.size() - 1), Scalar(0, 0, 255));
				}

				for (int i = 0; i < ROIPointsPanorama.size(); i++)
				{
					if (i != 0)
					{
						line(panoramaframeROI, ROIPointsPanorama.at(i - 1), ROIPointsPanorama.at(i), Scalar(0, 0, 255));
					}

					circle(panoramaframeROI, ROIPointsPanorama.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsPanorama.size() > 2)
				{
					line(panoramaframeROI, ROIPointsPanorama.at(0), ROIPointsPanorama.at(ROIPointsPanorama.size() - 1), Scalar(0, 0, 255));
				}
			}

			if (StartBgFgVideo)
			{
				PanoramaConfig.SetFrame(panoramaframe);
				PanoramaConfig.ClassifyImage(panoramaframeclassified, panoramaframegmm, PanoramaConfig.GetCurrentClusterNumber(), 1);
				PanoramaConfig.CreateBgFgImage(panoramaframeclassified, panoramaframebgfg);

				erode(panoramaframebgfg, panoramaframebgfg, getStructuringElement(MORPH_DILATE, Size(5, 5)));
				dilate(panoramaframebgfg, panoramaframebgfg, getStructuringElement(MORPH_DILATE, Size(5, 5)));
				
				if (PlayersInit)
				{
					ObjectClassifier.SetColorImage(panoramaframe);
					ObjectClassifier.SetBgFgImage(panoramaframebgfg);

					ObjectClassifier.UpdateProbablePlayersWithTeamInformation(PanoramaVideo.HomographyMatrixCenter.H, PanoramaVideo.CenterOffset);

					for (int i = 0; i < ObjectClassifier.GetPlayerPositions().size(); i++)
					{
						Player* probableplayer = new Player(i, ObjectClassifier.GetPLayersTeam().at(i), ObjectClassifier.GetPlayerBoundingBoxes().at(i), ObjectClassifier.GetPlayerPositions().at(i), ObjectClassifier.GetPlayerRealHeights().at(i), ObjectClassifier.GetPlayerImages().at(i), ObjectClassifier.GetPlayerMasks().at(i));

						ProbablePlayers.push_back(probableplayer);
					}

					Tracker.TrackAll(panoramaframe, panoramaframebgfg, ProbablePlayers, PanoramaVideo.CenterOffset, PanoramaVideo.HomographyMatrixCenter.H);

					vector<ImageObject*> PlayersForDisplay = Tracker.GetObjects();

					for (int i = 0; i < PlayersForDisplay.size(); i++)
					{
						if (PlayersForDisplay.at(i)->TrackStatus.State == NoObjectInSearchRadius)
						{
							rectangle(panoramaframe, PlayersForDisplay.at(i)->GetBoundingBoxes().at(0), Scalar(0, 0, 255), 5);
						}
						else if (PlayersForDisplay.at(i)->TrackStatus.State == DifferentIdsForColorAndPosition)
						{
							rectangle(panoramaframe, PlayersForDisplay.at(i)->GetBoundingBoxes().at(0), Scalar(0, 255, 255), 5);
						}
						else if (PlayersForDisplay.at(i)->TrackStatus.State == TrackDone)
						{
							rectangle(panoramaframe, PlayersForDisplay.at(i)->GetBoundingBoxes().at(0), Scalar(255, 0, 0), 5);
						}
					}

					for (int i = 0; i < ObjectClassifier.GetPlayerPositions().size(); i++)
					{
						delete ProbablePlayers.at(i);
					}

					ProbablePlayers.clear();

					cout << "Tracked objects number: " << Tracker.GetObjects().size() << endl;

					VirtualPitch.ClearPlayers();
					VirtualPitch.AddPlayers(Tracker.GetObjects());

					imshow("Virtual football pitch", VirtualPitch.GetFootballPitchFrame());
					waitKey(10);
				}

				imshow("panorama bgfg", panoramaframebgfg);
			}

			leftframeROIDisplay = Mat();
			centerframeROIDisplay = Mat();
			rightframeROIDisplay = Mat();

			leftframeROIDisplay = Mat(leftframe.rows, leftframe.cols, leftframe.type());
			centerframeROIDisplay = Mat(centerframe.rows, centerframe.cols, centerframe.type());
			rightframeROIDisplay = Mat(rightframe.rows, rightframe.cols, rightframe.type());

			leftframe.copyTo(leftframeROIDisplay, ROIMaskLeft);
			centerframe.copyTo(centerframeROIDisplay, ROIMaskCenter);
			rightframe.copyTo(rightframeROIDisplay, ROIMaskRight);

			if (State.CutROIMode == 1)
			{
				imshow("left", leftframeROI);
				imshow("center", centerframeROI);
				imshow("right", rightframeROI);
			}
			else
			{
				imshow("left", leftframeROIDisplay);
				imshow("center", centerframeROIDisplay);
				imshow("right", rightframeROIDisplay);
			}

			if (!panoramaframe.empty())
			{
				panoramaframeROIDisplay = Mat();
				panoramaframeROIDisplay = Mat(panoramaframe.rows, panoramaframe.cols, panoramaframe.type());
				panoramaframe.copyTo(panoramaframeROIDisplay, ROIMaskPanorama);

				if (State.CutROIMode == 1)
				{
					imshow("panorama", panoramaframeROI);
				}
				else
				{
					imshow("panorama", panoramaframeROIDisplay);
				}
			}

			waitKey(50);
		}
		else
		{
			if (State.StartSetPanorama == 1)
			{
				if (!clickcoord.empty())
				{
					CoordWithWindow temp = clickcoord.back();
					if (temp.Window == 0)
					{
						circle(leftframe, temp.Clickcoord, 10, Scalar(0, 0, 255));
					}
					else if (temp.Window == 1)
					{
						circle(centerframe, temp.Clickcoord, 10, Scalar(0, 0, 255));
					}
					else if (temp.Window == 2)
					{
						circle(rightframe, temp.Clickcoord, 10, Scalar(0, 0, 255));
					}
				}

				if (!ImagePointsForCreatePanorama.empty())
				{
					for (int i = 0; i < ImagePointsForCreatePanorama.size(); i++)
					{
						if (ImagePointsForCreatePanorama.at(i).Window == 0)
						{
							circle(leftframe, ImagePointsForCreatePanorama.at(i).Clickcoord, 10, Scalar(0, 255, 255));
						}
						else if (ImagePointsForCreatePanorama.at(i).Window == 1)
						{
							circle(centerframe, ImagePointsForCreatePanorama.at(i).Clickcoord, 10, Scalar(0, 255, 255));
						}
						else if (ImagePointsForCreatePanorama.at(i).Window == 2)
						{
							circle(rightframe, ImagePointsForCreatePanorama.at(i).Clickcoord, 10, Scalar(0, 255, 255));
						}
					}
				}

				if (!ObjectPointsForCreatePanorama.empty())
				{
					Size PitchSize;

					PitchSize.width = ObjectPointsForCreatePanorama.at(0).Clickcoord.x;
					PitchSize.height = ObjectPointsForCreatePanorama.at(0).Clickcoord.y;

					for (int i = 0; i < ObjectPointsForCreatePanorama.size(); i++)
					{
						if (ObjectPointsForCreatePanorama.at(i).Clickcoord.x > PitchSize.width)
						{
							PitchSize.width = ObjectPointsForCreatePanorama.at(i).Clickcoord.x;
						}

						if (ObjectPointsForCreatePanorama.at(i).Clickcoord.y > PitchSize.height)
						{
							PitchSize.height = ObjectPointsForCreatePanorama.at(i).Clickcoord.y;
						}
					}

					VirtualPitch.SetPitchSize(PitchSize);
				}
			}

			if (State.CutROIMode == 1)
			{
				leftframeROI = leftframe.clone();
				centerframeROI = centerframe.clone();
				rightframeROI = rightframe.clone();
				panoramaframeROI = panoramaframe.clone();

				if (clickcoord.size() != 0)
				{
					if (clickcoord.at(0).Window == 0)
					{
						circle(leftframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsLeft.size() > 0)
						{
							line(leftframeROI, ROIPointsLeft.at(ROIPointsLeft.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
					if (clickcoord.at(0).Window == 1)
					{
						circle(centerframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsCenter.size() > 0)
						{
							line(centerframeROI, ROIPointsCenter.at(ROIPointsCenter.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
					if (clickcoord.at(0).Window == 2)
					{
						circle(rightframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsRight.size() > 0)
						{
							line(rightframeROI, ROIPointsRight.at(ROIPointsRight.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
					if (clickcoord.at(0).Window == 3)
					{
						circle(panoramaframeROI, clickcoord.at(0).Clickcoord, 5, Scalar(255, 0, 0));

						if (ROIPointsPanorama.size() > 0)
						{
							line(panoramaframeROI, ROIPointsPanorama.at(ROIPointsPanorama.size() - 1), clickcoord.at(0).Clickcoord, Scalar(0, 0, 255));
						}
					}
				}

				for (int i = 0; i < ROIPointsLeft.size(); i++)
				{
					if (i != 0)
					{
						line(leftframeROI, ROIPointsLeft.at(i - 1), ROIPointsLeft.at(i), Scalar(0, 0, 255));
					}

					circle(leftframeROI, ROIPointsLeft.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsLeft.size() > 2)
				{
					line(leftframeROI, ROIPointsLeft.at(0), ROIPointsLeft.at(ROIPointsLeft.size() - 1), Scalar(0, 0, 255));
				}

				for (int i = 0; i < ROIPointsCenter.size(); i++)
				{
					if (i != 0)
					{
						line(centerframeROI, ROIPointsCenter.at(i - 1), ROIPointsCenter.at(i), Scalar(0, 0, 255));
					}

					circle(centerframeROI, ROIPointsCenter.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsCenter.size() > 2)
				{
					line(centerframeROI, ROIPointsCenter.at(0), ROIPointsCenter.at(ROIPointsCenter.size() - 1), Scalar(0, 0, 255));
				}

				for (int i = 0; i < ROIPointsRight.size(); i++)
				{
					if (i != 0)
					{
						line(rightframeROI, ROIPointsRight.at(i - 1), ROIPointsRight.at(i), Scalar(0, 0, 255));
					}

					circle(rightframeROI, ROIPointsRight.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsRight.size() > 2)
				{
					line(rightframeROI, ROIPointsRight.at(0), ROIPointsRight.at(ROIPointsRight.size() - 1), Scalar(0, 0, 255));
				}

				for (int i = 0; i < ROIPointsPanorama.size(); i++)
				{
					if (i != 0)
					{
						line(panoramaframeROI, ROIPointsPanorama.at(i - 1), ROIPointsPanorama.at(i), Scalar(0, 0, 255));
					}

					circle(panoramaframeROI, ROIPointsPanorama.at(i), 5, Scalar(255, 0, 0));
				}

				if (ROIPointsPanorama.size() > 2)
				{
					line(panoramaframeROI, ROIPointsPanorama.at(0), ROIPointsPanorama.at(ROIPointsPanorama.size() - 1), Scalar(0, 0, 255));
				}
			}

			if (CreateROIMask == 1)
			{
				leftframeROI = Mat();
				centerframeROI = Mat();
				rightframeROI = Mat();
				panoramaframeROI = Mat();

				leftframeROI = Mat(leftframe.rows, leftframe.cols, leftframe.type());
				centerframeROI = Mat(centerframe.rows, centerframe.cols, centerframe.type());
				rightframeROI = Mat(rightframe.rows, rightframe.cols, rightframe.type());
				panoramaframeROI = Mat(panoramaframe.rows, panoramaframe.cols, panoramaframe.type());

				for (int i = 0; i < InitPoints.size(); i++)
				{
					if (InitPoints.at(i).Window == 0)
					{
						LeftCameraConfig.SetFrame(leftframe);

						LeftCameraConfig.CreateMask(ROIPointsLeft, InitPoints.at(i).Clickcoord);

						ROIMaskLeft = LeftCameraConfig.GetMask();
					}
					if (InitPoints.at(i).Window == 1)
					{
						CenterCameraConfig.SetFrame(centerframe);

						CenterCameraConfig.CreateMask(ROIPointsCenter, InitPoints.at(i).Clickcoord);

						ROIMaskCenter = CenterCameraConfig.GetMask();
					}
					if (InitPoints.at(i).Window == 2)
					{
						RightCameraConfig.SetFrame(rightframe);

						RightCameraConfig.CreateMask(ROIPointsRight, InitPoints.at(i).Clickcoord);

						ROIMaskRight = RightCameraConfig.GetMask();
					}
					if (InitPoints.at(i).Window == 3)
					{
						PanoramaConfig.SetFrame(panoramaframe);

						PanoramaConfig.CreateMask(ROIPointsPanorama, InitPoints.at(i).Clickcoord);

						ROIMaskPanorama = PanoramaConfig.GetMask();
					}
				}

				leftframe.copyTo(leftframeROI, ROIMaskLeft);
				centerframe.copyTo(centerframeROI, ROIMaskCenter);
				rightframe.copyTo(rightframeROI, ROIMaskRight);
				panoramaframe.copyTo(panoramaframeROI, ROIMaskPanorama);

				CreateROIMask = 0;

				imshow("left", leftframeROI);
				imshow("center", centerframeROI);
				imshow("right", rightframeROI);
				imshow("panorama", panoramaframeROI);

				waitKey(0);
			}

			leftframeROIDisplay = Mat();
			centerframeROIDisplay = Mat();
			rightframeROIDisplay = Mat();

			leftframeROIDisplay = Mat(leftframe.rows, leftframe.cols, leftframe.type());
			centerframeROIDisplay = Mat(centerframe.rows, centerframe.cols, centerframe.type());
			rightframeROIDisplay = Mat(rightframe.rows, rightframe.cols, rightframe.type());

			leftframe.copyTo(leftframeROIDisplay, ROIMaskLeft);
			centerframe.copyTo(centerframeROIDisplay, ROIMaskCenter);
			rightframe.copyTo(rightframeROIDisplay, ROIMaskRight);

			if (State.CutROIMode == 1)
			{
				imshow("left", leftframeROI);
				imshow("center", centerframeROI);
				imshow("right", rightframeROI);
			}
			else
			{
				imshow("left", leftframeROIDisplay);
				imshow("center", centerframeROIDisplay);
				imshow("right", rightframeROIDisplay);
			}

			if (!panoramaframe.empty())
			{
				panoramaframeROIDisplay = Mat();
				panoramaframeROIDisplay = Mat(panoramaframe.rows, panoramaframe.cols, panoramaframe.type());
				panoramaframe.copyTo(panoramaframeROIDisplay, ROIMaskPanorama);

				if (State.CutROIMode == 1)
				{
					imshow("panorama", panoramaframeROI);
				}
				else
				{
					imshow("panorama", panoramaframeROIDisplay);
				}
			}

			waitKey(50);
		}
	}
}