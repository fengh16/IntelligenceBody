/**
* FH_AI.h
*
* 包含类RobotCommands，其中包含控制机器人指令的函数
*
* 该文件进行上述类以及函数的声明
*
* 冯昊
*
* V3.5.2  2017/05/11
*
* CopyRight 2017
*/


#pragma once

#include "../sdk/sdk.h"
#include "../sdk/misc.h"
#include "../sdk/const.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <set>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <string>

using namespace std;

//空投时候剩下的能量
#define AirBornLeftEnergy 300

//如果1级需要变成其他，需要的能量
#define ChangeTypeNeedMin 100

//空投时候空投出去的能量
#define AirBornUseEnergy 1750

//采集型机器人的最大等级
#define GatherMaxLevelBase 3

using Pos = SDK::Pos;
using PPlayerInfo = SDK::PPlayerInfo;
using PCommand = SDK::PCommand;
using PShowInfo = SDK::PShowInfo;
using Edge = PPlayerInfo::Edge;
using Robot = PPlayerInfo::Robot;
using HumanOrder = PPlayerInfo::HumanOrder;
using Buff = Robot::Buff;
using Skill = Robot::Skill;
using Operation = PCommand::Operation;
using ShowInfo = PShowInfo::ShowInfo;

extern PPlayerInfo playerInfo;
extern PCommand pCommand;
extern PShowInfo *pShowInfo;


extern int No_Turns;
extern int My_Robot_Num;
extern int En_Robot_Num;
extern int GatherMaxLevel;
extern double AllSpots[MAP_SIZE][MAP_SIZE];
extern double RankAllSpots[MAP_SIZE][MAP_SIZE];
extern long tStick;
extern string FileName;
extern string TimeSting;
extern string TargetFile;
extern char Adder[10];
extern ofstream My_ZNT_Out;
extern string RobotTypeName[];
enum NearestName { NearestDist2, NearestNum, NearestEnDist2, NearestEnNum, NearestMyDist2, NearestMyNum, My_In16Num, En_In16Num, My_BombNum, En_BombNum, My_In16_Defense_Num, My_In16_Attack_Num, WallsNum, BombingMyNum, BombingEnNum, NearestNameNum };
//我方机器人列表，敌方机器人列表
extern vector<Robot *> myRobot, enemyRobot, myFrontRobot;
//当前地图上的空地
extern vector<Pos> buildablePos;
//已经被占据的位置
extern vector<Pos> occupied;
//机器人与他们编号的关系，如果不是自己的，就返回敌方编号+自己编号数
extern map<Robot*, int> MyRobotNos;
//need：每个机器人需要的能量
extern map<Pos, int> need;
//记录一下已经被我方占领的位置，不要再靠近占领了！
extern int AlreadyBuild[MAP_SIZE][MAP_SIZE];
//记录一下现在哪些位置上放了attack机器人，如果没有的话就让他们中的一些变成attack
extern bool AttackMap[MAP_SIZE][MAP_SIZE];
//记录放了defense的机器人
extern bool DefenseMap[MAP_SIZE][MAP_SIZE];
//下面开始初始化每个点的数据记录
extern int Nearest[MAP_SIZE][MAP_SIZE][NearestNameNum];
//机器人与他们位置的关系
extern map<Pos, int> MyPosNos;
//动态数组，记录一下敌方血量数据（如果打死了就不要再打了）
extern int* EnHps;
//记录敌方已经受到天灾的
extern bool* EnScou;
//记录我方现在等级
extern int* AlreadyLevel;
//记录我方将死机器人的
extern bool* MyWantToDie;
//记录我方每个机器人连接的个数
extern int* MyLinksNum;
//我方已经建立的连接，如果不是自己的，就返回敌方编号+自己编号数
extern bool** MyLinks;
//我方已经建立和正在建立的连接
extern bool** MyTryLinks;
//我方机器人可以传输出去的能量
extern int* CanTrasfer;
//我方机器人的传输容量
extern int* MyCap;
//攻击太慢，加个变量试试
extern bool FinalAttack;
//人工操作的需要的
extern bool AirToEnHuman;
//向敌方阵地内分裂，打死你
extern bool FarSplitToEn;
extern bool HumanToEn;
extern bool MyToGrow;
extern bool ToEnFirst;
extern bool FirstExpand;

void PreThings();
void OutPutEn();
void TransferNow();
void DeleteThings();
void SetStanderd();

struct TransUseOrdersStru
{
	int SourcePos;
	int TarPos;
	int NeedEnergy;
};

bool cmpHpOut(const Robot *l, const Robot *r);

enum FHSplitType { Farest, Far, ALittleFar, NotFar };

class RobotCommands
{
private:
	int MyRobotLoopTemp;
	int freeEnergy;
	RobotType MyNowType;

public:
	RobotCommands(int No);
	bool DieJudge();
	void TurnToDefense();
	void RawEvolve();
	void FirstLineToAtt();
	void SplitAllCommands();
	void UpDate();
	void PlainAtt();
	void AccordingTypeCommands();
	void AttackGo();
	void DefenseGo();
	void GatherGo();
	void LinkToEn();
	void Back();
	void TrasCommands();
	void DieSplit();
	bool SplitCommands(FHSplitType s);
};

namespace NotDecentAI
{
	void PrePare();
}

//人工操作解析器
class Parser
{
private:
	//技能发起者
	Pos source;
	//当前选择的机器人
	Pos choose;
	//是否处于释放单目标点技能状态
	bool uTargetSkill;
	//当前使用的技能类型
	OrderType type;

public:
	//是否为反馈解释器(非反馈解释器不ShowInfo)
	bool isFeedBack;
	Parser();
	void Select(Pos target);
	void Deselect();
	//这个解析器会将所有人工操作显示在播放器的控制台中
	void log(const HumanOrder& order);
	//解析按下局部按钮的事件
	void parseLocalButtonClick(const HumanOrder& order);
	//解析按下全局按钮的事件
	void parseGlobalButtonClick(const HumanOrder& order);
	//解析按键事件
	void parseKeyDown(const HumanOrder& order);
	//解析左键点击事件
	void parseLeftMouseClick(const HumanOrder& order);
	//解析右键点击事件
	void parseRightMouseClick(const HumanOrder& order);
	//解析控制台事件
	void parseConsole(const HumanOrder& order);
	void parse(const HumanOrder& order);
	void parse(const vector<HumanOrder>& orders);
};

extern Parser parser, parser_feedback;