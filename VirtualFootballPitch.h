#pragma once

#include "Globals.h"
#include "Player.h"

class VirtualFootballPitch
{
public:
	VirtualFootballPitch();
	VirtualFootballPitch(Size PitchSize);
	~VirtualFootballPitch();

	Mat GetFootballPitchFrame(void);
	void AddPlayers(vector<ImageObject*> Players);
	void SetBall(Point2d Ball);
	void SetRemovedObjectIds(vector<int> RemovedObjectIds);
	void ClearRemovedObjectIds();
	void SetPitchSize(Size PitchSize);
	void ClearPlayers(void);

private:
	Mat FootballPitch;
	vector<Player> Players;
	vector<int> TeamId;
	vector<int> RemovedObjectIds;
	Point2d Ball;
	Size PitchSize;
	double RatioX;
	double RatioY;

	void DrawPlayers(Mat &FootballPitchFrame);
	void DrawBall(Mat &FootballPitchFrame);
};

