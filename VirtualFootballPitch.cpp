#include "VirtualFootballPitch.h"

VirtualFootballPitch::VirtualFootballPitch()
{
	Ball = Point2d(-1.0, -1.0);

	PitchSize = Size(-1, -1);

	Mat FootballPitchChannelB = Mat::zeros(1080, 1560, CV_8U);
	Mat FootballPitchChannelG = Mat::ones(1080, 1560, CV_8U) * 130;
	Mat FootballPitchChannelR = Mat::zeros(1080, 1560, CV_8U);

	vector<Mat> channels;

	channels.push_back(FootballPitchChannelB);
	channels.push_back(FootballPitchChannelG);
	channels.push_back(FootballPitchChannelR);

	merge(channels, FootballPitch);

	rectangle(FootballPitch, Rect(90, 90, 1380, 900), Scalar(140, 255, 140), 5);

	rectangle(FootballPitch, Rect(90, 90 + 186, 216, 528), Scalar(140, 255, 140), 5);
	rectangle(FootballPitch, Rect(90, 90 + 330, 72, 240), Scalar(140, 255, 140), 5);
	rectangle(FootballPitch, Rect(54, 492, 36, 96), Scalar(140, 255, 140), CV_FILLED);
	ellipse(FootballPitch, Point(234, 540), Size(120, 120), 0, 53.13, -53.13, Scalar(140, 255, 140), 5);
	circle(FootballPitch, Point(234, 540), 4, Scalar(140, 255, 140), CV_FILLED);

	line(FootballPitch, Point(90 + 690, 90), Point(90 + 690, 990), Scalar(140, 255, 140), 5);
	circle(FootballPitch, Point(90 + 690, 450 + 90), 120, Scalar(140, 255, 140), 5);

	rectangle(FootballPitch, Rect(1254, 90 + 186, 216, 528), Scalar(140, 255, 140), 5);
	rectangle(FootballPitch, Rect(1398, 90 + 330, 72, 240), Scalar(140, 255, 140), 5);
	rectangle(FootballPitch, Rect(1470, 492, 36, 96), Scalar(140, 255, 140), CV_FILLED);
	ellipse(FootballPitch, Point(1326, 540), Size(120, 120), 0, 126.87, 233.13, Scalar(140, 255, 140), 5);
	circle(FootballPitch, Point(1326, 540), 4, Scalar(140, 255, 140), CV_FILLED);

	ellipse(FootballPitch, Point(90, 90), Size(24, 24), 0, 90, 0, Scalar(140, 255, 140), 5);
	ellipse(FootballPitch, Point(90, 990), Size(24, 24), 0, 270, 360, Scalar(140, 255, 140), 5);
	ellipse(FootballPitch, Point(1470, 90), Size(24, 24), 0, 180, 90, Scalar(140, 255, 140), 5);
	ellipse(FootballPitch, Point(1470, 990), Size(24, 24), 0, 180, 270, Scalar(140, 255, 140), 5);
}

VirtualFootballPitch::VirtualFootballPitch(Size PitchSize)
{
	VirtualFootballPitch();

	this->PitchSize = PitchSize;

	RatioX = 115 / PitchSize.width;
	RatioY = 75 / PitchSize.height;
}

VirtualFootballPitch::~VirtualFootballPitch()
{
}

Mat VirtualFootballPitch::GetFootballPitchFrame(void)
{
	Mat FootballPitchCopy;

	FootballPitch.copyTo(FootballPitchCopy);

	DrawPlayers(FootballPitchCopy);
	DrawBall(FootballPitchCopy);

	return FootballPitchCopy;
}

void VirtualFootballPitch::AddPlayers(vector<ImageObject*> Players)
{
	if (PitchSize != Size(-1, -1))
	{
		Point2d temp;

		for (int i = 0; i < Players.size(); i++)
		{
			temp.x = Players.at(i)->GetPositions().at(0).x * RatioX;
			temp.y = ((double)PitchSize.height - Players.at(i)->GetPositions().at(0).y) * RatioY;

			Player ActualPlayer = Player(Players.at(i)->GetId(), Players.at(i)->GetTeamId(), Players.at(i)->GetBoundingBoxes().at(0), temp, 0, Mat(), Mat());

			this->Players.push_back(ActualPlayer);
		}
	}
	else
	{
		cout << "Pitch Size is not set!" << endl;
	}
}

void VirtualFootballPitch::SetBall(Point2d Ball)
{
	if (PitchSize != Size(-1, -1))
	{
		Point2d temp;

		temp.x = Ball.x * RatioX;
		temp.y = (PitchSize.height - Ball.y) * RatioY;

		this->Ball = Ball;
	}
	else
	{
		cout << "Pitch Size is not set!" << endl;
	}
}

void VirtualFootballPitch::SetRemovedObjectIds(vector<int> RemovedObjectIds)
{
	for (int i = 0; i < RemovedObjectIds.size(); i++)
	{
		this->RemovedObjectIds.push_back(RemovedObjectIds.at(i));
	}
}

void VirtualFootballPitch::ClearRemovedObjectIds()
{
	RemovedObjectIds.clear();
}

void VirtualFootballPitch::SetPitchSize(Size PitchSize)
{
	this->PitchSize = PitchSize;

	RatioX = 115 / PitchSize.width;
	RatioY = 75 / PitchSize.height;
}

void VirtualFootballPitch::ClearPlayers(void)
{
	Players.clear();
	TeamId.clear();	
	RemovedObjectIds.clear();
}

void VirtualFootballPitch::DrawPlayers(Mat &FootballPitchFrame)
{
	Point Position;

	int fontFace = FONT_HERSHEY_COMPLEX_SMALL;
	double fontScale = 1;

	for (int i = 0; i < Players.size(); i++)
	{
		bool RemovedObject = false;

		for (int j = 0; j < RemovedObjectIds.size(); j++)
		{
			if (RemovedObjectIds.at(j) == Players.at(i).GetId())
			{
				RemovedObject = true;
			}
		}

		int IdTemp = i;
		vector<char> IdCharacters;
		String Id;

		do
		{
			char IdCharacter = '0' + IdTemp % 10;
			IdCharacters.push_back(IdCharacter);
			IdTemp -= IdTemp % 10;
			IdTemp /= 10;
		} 
		while (IdTemp != 0);

		for (int j = IdCharacters.size() - 1; j >= 0; j--)
		{
			Id += IdCharacters.at(j);
		}

		int baseline = 0;

		Size TextSize = getTextSize(Id, fontFace, fontScale, 1, &baseline);

		Position.x = 90 + cvRound(Players.at(i).GetPositions().at(0).x * 12);
		Position.y = 90 + cvRound(Players.at(i).GetPositions().at(0).y * 12);

		Point TextOrg(Position.x - TextSize.width / 2, Position.y + TextSize.height / 2);
		
		if (RemovedObject)
		{
			circle(FootballPitchFrame, Position, 12, Scalar(128, 128, 128), CV_FILLED);
		}
		else
		{
			circle(FootballPitchFrame, Position, 12, colors[Players.at(i).GetTeamId()], CV_FILLED);
		}

		putText(FootballPitchFrame, Id, TextOrg, fontFace, fontScale, Scalar(255, 255, 255));
	}
}

void VirtualFootballPitch::DrawBall(Mat & FootballPitchFrame)
{
	Point Position;

	Position.x = 90 + cvRound(Ball.x * 12);
	Position.y = 90 + cvRound(Ball.y * 12);

	if (Ball == Point2d(-1, -1))
	{
		circle(FootballPitchFrame, Ball, 10, Scalar(255, 255, 255), CV_FILLED);
	}
}
