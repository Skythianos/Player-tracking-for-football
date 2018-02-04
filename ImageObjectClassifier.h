#pragma once

#include "Globals.h"
#include "Panorama.h"
#include "Player.h"

class ImageObjectClassifier
{
public:
	ImageObjectClassifier();
	ImageObjectClassifier(Mat BgFgImage, Mat ColorImage);
	~ImageObjectClassifier();

	void SetBgFgImage(Mat BgFgImage);
	Mat GetBgFgImage(void);
	void SetColorImage(Mat ColorImage);
	Mat GetColorImage(void);
	vector<Rect> GetPlayerBoundingBoxes(void);
	vector<Point2d> GetPlayerPositions(void);
	vector<Point2d> GetRealPlayerPositions(void);
	vector<int> GetRealPlayersTeam(void);
	vector<int> GetPLayersTeam(void);
	vector<int> GetRemovedObjectIds(void);
	void SetMinPlayerHeight(int MinPlayerHeight);
	void SetMaxPlayerHeight(int MaxPlayerHeight);
	int GetCurrentClusterNumber(void);
	Ptr<EM> GetCurrentEMModel(void);
	vector<Mat> GetPlayerMasks(void);
	vector<Mat> GetPlayerImages(void);
	double GetMinPlayerHeight(void);
	double GetMaxPlayerHeight(void);
	vector<double> GetPlayerRealHeights(void);

	void FindProbablePlayers(const Mat & HomographyMatrix, Panorama::Offset CenterOffset);
	void SetProbablePlayersMaxHistogramPositions();
	bool ClassifyPlayers(int ClusterNumber);
	void ClearVectors();
	bool RemoveObjectFromProbablePlayers(Point2d ObjectPosition);
	bool AddObjectToProbablePlayers(Point2d ObjectPosition);
	void UpdateProbablePlayersWithTeamInformation(const Mat & HomographyMatrix, Panorama::Offset CenterOffset);
	vector<ImageObject*> InitializePlayers();

private:
	Mat BgFgImage;
	Mat ColorImage;
	double MinPlayerHeight;
	double MaxPlayerHeight;
	vector<double> PlayerRealHeights;
	vector<Rect> PlayerBoundingBoxes;
	vector<Mat> PlayerMasks;
	vector<Mat> PlayerImages;
	vector<Point2d> PlayerPositions;
	vector<int> PlayerMaxHistogramPositions;
	vector<int> PlayersTeam;
	vector<int> RemovedObjectIds;
	vector<Point2d> RealPlayerPositions;
	vector<int> RealPlayersTeam;
	GMM_Model GMM_ModelsForTeam[10];
	int CurrentClusterNumber;

	Mat TransformPoint(Point2d Point, const Mat & HomographyMatrix);
	int GetHistogramMaxPosition(Mat &ColorImage, Mat &Mask);

};

