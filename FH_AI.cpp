/**
* FH_AI.cpp
*
* 包含类RobotCommands，其中包含控制机器人指令的函数
*
* 该文件进行上述类以及函数的具体实现
*
* 冯昊
*
* V3.5.2  2017/05/11
*
* CopyRight 2017
*/


#include "FH_AI.h"

PPlayerInfo playerInfo;
PCommand pCommand;
PShowInfo *pShowInfo;

int No_Turns = 0;
int My_Robot_Num;
int En_Robot_Num;
int GatherMaxLevel;
double AllSpots[MAP_SIZE][MAP_SIZE];
double RankAllSpots[MAP_SIZE][MAP_SIZE];
long tStick = time(NULL);
string FileName = "FHFile";
string TimeSting;
string TargetFile;
char Adder[10];
ofstream My_ZNT_Out;
string RobotTypeName[] = { "RawRobot", "AttackRobot", "DefenseRobot", "GatherRobot" };
vector<Robot *> myRobot, enemyRobot, myFrontRobot;
vector<Pos> buildablePos;
vector<Pos> occupied;
map<Robot*, int> MyRobotNos;
map<Pos, int> need;
int AlreadyBuild[MAP_SIZE][MAP_SIZE] = { 0 };
bool AttackMap[MAP_SIZE][MAP_SIZE] = { 0 };
bool DefenseMap[MAP_SIZE][MAP_SIZE] = { 0 };
int Nearest[MAP_SIZE][MAP_SIZE][NearestNameNum];
map<Pos, int> MyPosNos;
int* EnHps;
bool* EnScou;
int* AlreadyLevel;
bool* MyWantToDie;
int* MyLinksNum;
bool** MyLinks;
bool** MyTryLinks;
int* CanTrasfer;
int* MyCap;
bool FinalAttack = false;
bool AirToEnHuman = false;
bool FarSplitToEn = false;
bool HumanToEn = false;
bool MyToGrow = false;
bool ToEnFirst = true;
bool FirstExpand = true;

bool cmpHpOut(const Robot *l, const Robot *r)
{
	return l->hp * r->max_hp < r->hp * l->max_hp;
}


namespace NotDecentAI
{
	void PrePare()
	{
		PreThings();
		//用数组方式进行遍历，遍历自己的机器人
		for (int No_MyRobot = 0; No_MyRobot < myRobot.size(); No_MyRobot++)
		{
			RobotCommands MYR(No_MyRobot);
			MYR.DieJudge();
			MYR.TurnToDefense();
			MYR.RawEvolve();
			MYR.FirstLineToAtt();
			MYR.SplitAllCommands();
			MYR.UpDate();
			MYR.PlainAtt();
			MYR.AccordingTypeCommands();
			MYR.Back();
			MYR.TrasCommands();
		}
		OutPutEn();
		TransferNow();
		DeleteThings();
	}
}


//人工操作解析器
Parser::Parser()
{
	uTargetSkill = false;
	source = Pos(-1, -1);
}

void Parser::Select(Pos target)
{
	if (isFeedBack) pShowInfo->SelectGrid(target);
	choose = target;
}

void Parser::Deselect()
{
	if (isFeedBack) pShowInfo->DeselectGrid();
	choose = Pos(-1, -1);
}

void Parser::log(const HumanOrder& order)
{
	switch (order.type)
	{
	case HumanOrderType::Console:
	{
		pShowInfo->ShowString("Console:" + order.info);
	}
	break;
	case HumanOrderType::KeyDown: pShowInfo->ShowString(string("KeyDown id:") + to_string(order.id) + " name:" + order.info); break;
	case HumanOrderType::KeyUp: pShowInfo->ShowString(string("KeyUp id:") + to_string(order.id) + " name:" + order.info); break;
	case HumanOrderType::LeftMouseDown: pShowInfo->ShowString(string("LeftMouseDown:") + order.target.ToString()); break;
	case HumanOrderType::LeftMouseUp: pShowInfo->ShowString(string("LeftMouseUp:") + order.target.ToString()); break;
	case HumanOrderType::LeftMouseClick: pShowInfo->ShowString(string("LeftMouseClick:") + order.target.ToString()); break;
	case HumanOrderType::RightMouseClick: pShowInfo->ShowString(string("RightMouseClick:") + order.target.ToString()); break;
	case HumanOrderType::LocalButtonClick: pShowInfo->ShowString(string("LocalButtonClick id:") + to_string(order.id) + " name:" + order.info); break;
	case HumanOrderType::GlobalButtonClick: pShowInfo->ShowString(string("GlobalButtonClick id:") + to_string(order.id) + " name:" + order.info); break;
	}
}

//解析按下局部按钮的事件
void Parser::parseLocalButtonClick(const HumanOrder& order)
{
	source = order.target;
	if (!isFeedBack && playerInfo.RobotAt(source) == nullptr) return;
	switch (order.id)
	{
		/* 这一段将会标记当前所执行的操作类型 */
	case 0: type = OrderType::PlainAttack; uTargetSkill = true; break;
	case 1: type = OrderType::Split; uTargetSkill = true; break;
	case 2: type = OrderType::Connect; uTargetSkill = true; break;
	case 3: type = OrderType::Disconnect; uTargetSkill = true; break;
	case 4: type = OrderType::Recover; uTargetSkill = true; break;
	case 5: type = OrderType::Airborne; uTargetSkill = true; break;
	case 6: if (!isFeedBack) pCommand.AddOrder(OrderType::Overload, source); break;
	case 7: type = OrderType::Scourge; uTargetSkill = true; break;
	case 8: type = OrderType::Bombing; uTargetSkill = true; break;
	case 9: if (!isFeedBack) pCommand.AddOrder(OrderType::Shielding, source); break;
	case 16: type = OrderType::Evolve; break;
	case 17: type = OrderType::Vestigial; break;
		/* 这一段操作将会完成升降级的操作流程 */
	default:
		if (type == OrderType::Evolve || type == OrderType::Vestigial)
			if (!isFeedBack)
				switch (order.id) {
				case 48: pCommand.AddOrder(type, source, RobotType::RawRobot); break;
				case 50: pCommand.AddOrder(type, source, RobotType::AttackRobot); break;
				case 49: pCommand.AddOrder(type, source, RobotType::DefenseRobot); break;
				case 51: pCommand.AddOrder(type, source, RobotType::GatherRobot); break;
				}
	}
	if (uTargetSkill && isFeedBack)pShowInfo->DeselectGrid();
}

//解析按下全局按钮的事件
void Parser::parseGlobalButtonClick(const HumanOrder& order)
{
	switch (order.id) {
	case 0: if (isFeedBack)pShowInfo->Surrender();
	}
}

//解析按键事件
void Parser::parseKeyDown(const HumanOrder& order)
{
	//技能快捷键
	if (!isFeedBack && playerInfo.RobotAt(choose) != nullptr) {
		switch (order.id) {
		case 97: type = OrderType::PlainAttack; uTargetSkill = true; break;					//A
		case 115: type = OrderType::Split; uTargetSkill = true; break;						//S
		case 99: type = OrderType::Connect; uTargetSkill = true; break;						//C
		case 100: type = OrderType::Disconnect; uTargetSkill = true; break;					//D
		case 114: type = OrderType::Recover; uTargetSkill = true; break;					//R
		case 102: type = OrderType::Airborne; uTargetSkill = true; break;					//F
		case 111: if (!isFeedBack) pCommand.AddOrder(OrderType::Overload, source); break;	//O
		case 116: type = OrderType::Scourge; uTargetSkill = true; break;					//T
		case 98: type = OrderType::Bombing; uTargetSkill = true; break;						//B
		case 104: if (!isFeedBack) pCommand.AddOrder(OrderType::Shielding, source); break;	//H
		case 101: if (!isFeedBack) pCommand.Evolve(source); break;							//E
		case 118: if (!isFeedBack) pCommand.Vestigial(source); break;						//V
		}
	}
}

//解析左键点击事件
void Parser::parseLeftMouseClick(const HumanOrder& order)
{
	//释放单目标点技能
	if (uTargetSkill)
	{
		uTargetSkill = false;
		if (isFeedBack)
			pShowInfo->PosFlash(order.target);
		else
			pCommand.AddOrder(type, source, order.target);
	}
	else Select(order.target);
}

//解析右键点击事件
void Parser::parseRightMouseClick(const HumanOrder& order)
{
	Deselect();
}

//解析控制台事件
void Parser::parseConsole(const HumanOrder& order)
{
	if (order.info == "Final" || order.info == "final")
	{
		FinalAttack = true;
		pShowInfo->ShowString("Final Attack Started !!!");
	}
	else if (order.info == "CF" || order.info == "cf")
	{
		FinalAttack = false;
		pShowInfo->ShowString("Final Attack Canceled !!!");
	}
	else if (order.info == "AE" || order.info == "ae")
	{
		AirToEnHuman = true;
		pShowInfo->ShowString("Air to En Started !!!");
	}
	else if (order.info == "CAE" || order.info == "cae")
	{
		AirToEnHuman = false;
		pShowInfo->ShowString("Air to En Canceled !!!");
	}
	else if (order.info == "SE" || order.info == "se")
	{
		FarSplitToEn = true;
		pShowInfo->ShowString("Split to En Started !!!");
	}
	else if (order.info == "CSE" || order.info == "cse")
	{
		FarSplitToEn = false;
		pShowInfo->ShowString("Split to En Canceled !!!");
	}
	else if (order.info == "HE" || order.info == "he")
	{
		HumanToEn = true;
		pShowInfo->ShowString("Human to En Started !!!");
	}
	else if (order.info == "CHE" || order.info == "che")
	{
		HumanToEn = false;
		pShowInfo->ShowString("Human to En Canceled !!!");
	}
	else if (order.info == "GROW" || order.info == "grow")
	{
		MyToGrow = true;
		pShowInfo->ShowString("Grow to En Started !!!");
	}
	else if (order.info == "CG" || order.info == "cg")
	{
		MyToGrow = false;
		pShowInfo->ShowString("Grow to En Canceled !!!");
	}

	else
		pShowInfo->ShowString("Help: \n  Final / final to FinalAttack, CF / cf to cancel, \n  AE / ae to AirToEn, CAE / cae to cancel, \n  SE / se to SplitToEn, CSE / cse to cancel, \n  HE / he to HumanEn, CHE / che to cancel, \n  GROW / grow to GrowEn, CG / cg to cancel!");
}

void Parser::parse(const HumanOrder& order)
{
	if (isFeedBack)
		log(order);
	switch (order.type)
	{
	case HumanOrderType::Console: parseConsole(order); break;
	case HumanOrderType::KeyDown: parseKeyDown(order); break;
	case HumanOrderType::KeyUp: break;
	case HumanOrderType::LeftMouseDown: break;
	case HumanOrderType::LeftMouseUp: break;
	case HumanOrderType::LeftMouseClick: parseLeftMouseClick(order); break;
	case HumanOrderType::RightMouseClick: parseRightMouseClick(order); break;
	case HumanOrderType::LocalButtonClick: parseLocalButtonClick(order); break;
	case HumanOrderType::GlobalButtonClick: parseGlobalButtonClick(order); break;
	}
}

void Parser::parse(const vector<HumanOrder>& orders)
{
	for (unsigned int i = 0; i < orders.size(); i++)
		parse(orders[i]);
}
Parser parser, parser_feedback;

RobotCommands::RobotCommands(int No) : MyRobotLoopTemp(No)
{
	//记录一下现在是什么类型机器人
	MyNowType = myRobot[MyRobotLoopTemp]->type;
	MyWantToDie[MyRobotLoopTemp] = false;
	//输出信息，应该没什么问题
	My_ZNT_Out << "\n______________________________________\n";
	My_ZNT_Out << "MyRobot: " << ++My_Robot_Num << "\t  Level: " << myRobot[MyRobotLoopTemp]->level << "\t  Energy: " << myRobot[MyRobotLoopTemp]->energy << "\t  HP: " << myRobot[MyRobotLoopTemp]->hp << "     \t" << RobotTypeName[MyNowType] << endl;
	My_ZNT_Out << "Pos: " << myRobot[MyRobotLoopTemp]->pos.x << " , " << myRobot[MyRobotLoopTemp]->pos.y << "\nPosInfo:" << "NearestDist2: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][NearestDist2] << " \tNum:  " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][NearestNum] << "\nNearestEnDist2: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][NearestEnDist2] << " \tNum:   " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][NearestEnNum] << "\nNearestMyDist2: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][NearestMyDist2] << " \tNum:  " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][NearestMyNum] << "\n16My: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16Num] << " \t16En: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] << "\nBombMy: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_BombNum] << " \tBombEn: " << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_BombNum] << endl << "\n" << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] << "   " << "\n" << Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][WallsNum] << endl;

	//计算剩余能量，放到freeEnergy里面
	freeEnergy = myRobot[MyRobotLoopTemp]->energy - myRobot[MyRobotLoopTemp]->consumption + myRobot[MyRobotLoopTemp]->efficiency * AllSpots[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y];
	My_ZNT_Out << "FreeEnergy" << freeEnergy << endl;
	if (freeEnergy - myRobot[MyRobotLoopTemp]->consumption <= 0)
		need[myRobot[MyRobotLoopTemp]->pos] = myRobot[MyRobotLoopTemp]->consumption - freeEnergy;
	else
		need[myRobot[MyRobotLoopTemp]->pos] = 0;
}


bool RobotCommands::DieJudge()
{
	//如果快死了，先降级到最低，然后把能量输出出去
	if (myRobot[MyRobotLoopTemp]->hp < myRobot[MyRobotLoopTemp]->max_hp * 0.0625 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 0)
	{
		MyWantToDie[MyRobotLoopTemp] = true;
		for (int j = 1; j < AlreadyLevel[MyRobotLoopTemp]; j++)
		{
			freeEnergy += Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp] - j, MyNowType = DefenseRobot) / 2;
		}
		for (int tratemp = myRobot.size() - 1; tratemp >= 0; tratemp--)
		{
			if (MyWantToDie[tratemp])
				continue;
			if (!MyLinks[MyRobotLoopTemp][tratemp])
				continue;
			int traNum = freeEnergy % 1000 > myRobot[MyRobotLoopTemp]->transport_capacity ? myRobot[MyRobotLoopTemp]->transport_capacity : freeEnergy % 1000;
			pCommand.AddOrder(Transfer, myRobot[MyRobotLoopTemp]->pos, myRobot[tratemp]->pos, freeEnergy % 1000);
			break;
		}
		DieSplit();
	}
	return false;
}

void RobotCommands::TurnToDefense()
{
	//如果实在hold不住了，变成防御型机器人玩玩，因为可以自己补血，应该问题不大。这时候不能是防御型或者其他类型
	if (myRobot[MyRobotLoopTemp]->hp < myRobot[MyRobotLoopTemp]->max_hp * 0.375 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 0 && MyNowType != DefenseRobot && MyNowType != RawRobot)
	{
		if (myRobot[MyRobotLoopTemp]->level > 1)
		{
			pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, DefenseRobot);
			freeEnergy += Skill::Cost(EvolveSkill, myRobot[MyRobotLoopTemp]->level - 1, MyNowType) / 2;
			AlreadyLevel[MyRobotLoopTemp]--;
			MyNowType = DefenseRobot;
			My_ZNT_Out << "Vestigial to DefenseRobot!" << endl;
			DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
			for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
				for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
				{
					if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
						Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]++;
				}
		}
		else if (myRobot[MyRobotLoopTemp]->energy >= ChangeTypeNeedMin)
		{
			pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, RawRobot);
			pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, DefenseRobot);
			freeEnergy -= ChangeTypeNeedMin;
			MyNowType = DefenseRobot;
			My_ZNT_Out << "Change to DefenseRobot!" << endl;
			DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
			for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
				for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
				{
					if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
						Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]++;
				}
		}
		else
			need[myRobot[MyRobotLoopTemp]->pos] += ChangeTypeNeedMin;
	}
}

void RobotCommands::RawEvolve()
{
	//如果是RawRobot，首先要升级！
	if (MyNowType == RawRobot)
	{
		//这里必须升级，所以就算是会掉血也要升级！
		//如果离敌方很远，就变成grow
		if (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_BombNum] == 0)
		{
			My_ZNT_Out << "RawRobot use Evolve to Gather!" << endl;
			pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, GatherRobot);
			freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
			AlreadyLevel[MyRobotLoopTemp]++;
			MyNowType = GatherRobot;
		}
		else if (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16Num])
		{
			if (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num] < 1 || (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num] < 2 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 2))
			{
				My_ZNT_Out << "RawRobot use Evolve to Defense!" << endl;
				pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, DefenseRobot);
				freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
				AlreadyLevel[MyRobotLoopTemp]++;
				MyNowType = DefenseRobot;
				DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
				for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
					for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
					{
						if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
							Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]++;
					}
			}
			else if (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] < 2)
			{
				My_ZNT_Out << "RawRobot use Evolve to Attack!" << endl;
				pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, AttackRobot);
				freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
				AlreadyLevel[MyRobotLoopTemp]++;
				MyNowType = AttackRobot;
				AttackMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
				for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
					for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
					{
						if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
							Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num]++;
					}
			}
		}
		//如果直接打不着，就变成attack
		else
		{
			if (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] < 1 || (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] < 2 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 2))
			{
				My_ZNT_Out << "RawRobot use Evolve to Attack!" << endl;
				pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, AttackRobot);
				freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
				AlreadyLevel[MyRobotLoopTemp]++;
				MyNowType = AttackRobot;
				AttackMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
				for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
					for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
					{
						if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
							Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num]++;
					}
			}
			else if (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num] < 2)
			{
				My_ZNT_Out << "RawRobot use Evolve to Defense!" << endl;
				pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, DefenseRobot);
				freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
				AlreadyLevel[MyRobotLoopTemp]++;
				MyNowType = DefenseRobot;
				DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
				for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
					for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
					{
						if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
							Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]++;
					}
			}
		}
	}
}

void RobotCommands::FirstLineToAtt()
{
	//如果是采集型而且在前线，就变成攻击型
	if (myRobot[MyRobotLoopTemp]->hp > myRobot[MyRobotLoopTemp]->max_hp * 0.7)
		return;
	if ((AlreadyLevel[MyRobotLoopTemp] <= 3 || !myRobot[MyRobotLoopTemp]->canUse(AirborneSkill))/* && myRobot[MyRobotLoopTemp]->energy < 2000*/ && MyNowType == GatherRobot && No_Turns >= 3 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 0 && (/*Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] < 1 || */(Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] >= 2 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] < 1)))
	{
		if (myRobot[MyRobotLoopTemp]->level > 1)
		{
			pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, AttackRobot);
			freeEnergy += Skill::Cost(EvolveSkill, myRobot[MyRobotLoopTemp]->level - 1, MyNowType) / 2;
			AlreadyLevel[MyRobotLoopTemp]--;
			MyNowType = AttackRobot;
			My_ZNT_Out << "Vestigial to AttackRobot!" << endl;
			AttackMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
			for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
				for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
				{
					if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
						Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num]++;
				}
		}
		else if (myRobot[MyRobotLoopTemp]->energy >= ChangeTypeNeedMin)
		{
			pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, RawRobot);
			pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, AttackRobot);
			freeEnergy -= ChangeTypeNeedMin;
			MyNowType = AttackRobot;
			My_ZNT_Out << "Change to AttackRobot!" << endl;
			AttackMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
			for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
				for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
				{
					if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
						Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num]++;
				}
		}
		else
			need[myRobot[MyRobotLoopTemp]->pos] += ChangeTypeNeedMin;
	}//如果是采集型而且在前线，就变成攻击型
	else if ((AlreadyLevel[MyRobotLoopTemp] <= 3 || !myRobot[MyRobotLoopTemp]->canUse(AirborneSkill))/* && myRobot[MyRobotLoopTemp]->energy < 2000*/ && MyNowType == GatherRobot && No_Turns >= 3 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 0 && (/*Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num] < 1 || */(Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] >= 2 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num] < 1)))
	{
		if (myRobot[MyRobotLoopTemp]->level > 1)
		{
			pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, DefenseRobot);
			freeEnergy += Skill::Cost(EvolveSkill, myRobot[MyRobotLoopTemp]->level - 1, MyNowType) / 2;
			AlreadyLevel[MyRobotLoopTemp]--;
			MyNowType = DefenseRobot;
			My_ZNT_Out << "Vestigial to DefenseRobot!" << endl;
			DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
			for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
				for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
				{
					if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
						Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]++;
				}
		}
		else if (myRobot[MyRobotLoopTemp]->energy >= ChangeTypeNeedMin)
		{
			pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, RawRobot);
			pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, DefenseRobot);
			freeEnergy -= ChangeTypeNeedMin;
			MyNowType = DefenseRobot;
			My_ZNT_Out << "Change to DefenseRobot!" << endl;
			DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = true;
			for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
				for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
				{
					if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
						Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]++;
				}
		}
		else
			need[myRobot[MyRobotLoopTemp]->pos] += ChangeTypeNeedMin;
	}
}

void RobotCommands::SplitAllCommands()
{
	if (freeEnergy < FixedSplitCost && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16Num] + Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][WallsNum] < 4)
	{
		need[myRobot[MyRobotLoopTemp]->pos] += FixedSplitCost - freeEnergy + 80;
	}
	//如果能够分裂，就分裂呗
	if (freeEnergy > FixedSplitCost)
	{
		//首先找最好的位置，离自己特别远（大于等于14）
		if (FarSplitToEn || FinalAttack)
		{
			if (SplitCommands(Farest))
				return;
		}
		if (!FinalAttack)
		{
			if (HumanToEn)
			{
				if (SplitCommands(ALittleFar))
					return;
			}
			if (SplitCommands(Far))
				return;
			if (myRobot.size() + enemyRobot.size() < 45 && myRobot.size() < 20 && !FinalAttack && (Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] == 0 || No_Turns < 25))
				return;
			if (myRobot[MyRobotLoopTemp]->canUse(AirborneSkill))
				return;
			//之后找最差的位置，离自己比较远（大于等于5）
			if (SplitCommands(NotFar))
				return;
		}
	}
}

void RobotCommands::UpDate()
{
	if (!MyWantToDie[MyRobotLoopTemp])
	{
		if (myRobot[MyRobotLoopTemp]->canUse(AirborneSkill) && freeEnergy >= AirBornUseEnergy)
		{
			while (freeEnergy > Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType) + AirBornUseEnergy)
			{
				if (AlreadyLevel[MyRobotLoopTemp] < GatherMaxLevel)
				{
					My_ZNT_Out << "Evolve!" << endl;
					pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, MyNowType);
					freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
					AlreadyLevel[MyRobotLoopTemp]++;
				}
				else
					break;
			}
		}
		else
		{
			while (freeEnergy > Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType))
			{
				int levelneed = 3 + Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num];
				if (AlreadyLevel[MyRobotLoopTemp] > levelneed)
					break;
				if (MyNowType != GatherRobot || (MyNowType == GatherRobot && AlreadyLevel[MyRobotLoopTemp] < GatherMaxLevel))
				{
					My_ZNT_Out << "Evolve!" << endl;
					pCommand.AddOrder(Evolve, myRobot[MyRobotLoopTemp]->pos, MyNowType);
					freeEnergy -= Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
					AlreadyLevel[MyRobotLoopTemp]++;
				}
				else
					break;
			}
			if (MyNowType != GatherRobot && AlreadyLevel[MyRobotLoopTemp] < 4 && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] > 0)
				need[myRobot[MyRobotLoopTemp]->pos] += Skill::Cost(EvolveSkill, AlreadyLevel[MyRobotLoopTemp], MyNowType);
		}
	}
}

void RobotCommands::PlainAtt()
{
	for (int temp = 0; temp < enemyRobot.size(); temp++)
	{
		if (myRobot[MyRobotLoopTemp]->pos.dist2(enemyRobot[temp]->pos) <= myRobot[MyRobotLoopTemp]->attack_range && EnHps[temp] >= -100)
		{
			pCommand.AddOrder(PlainAttack, myRobot[MyRobotLoopTemp]->pos, enemyRobot[temp]->pos);
			EnHps[temp] -= myRobot[MyRobotLoopTemp]->attack * defenseBaseValue / (defenseBaseValue + enemyRobot[temp]->defense);
			My_ZNT_Out << "PlainAttack To Enemy " << temp << endl;
			break;
		}
	}
}

void RobotCommands::AccordingTypeCommands()
{
	//下面开始给每种类型的机器人分配任务
	switch (MyNowType)
	{
	case AttackRobot:
		AttackGo();
		break;
	case DefenseRobot:
		DefenseGo();
		break;
	case GatherRobot:
		GatherGo();
		break;
	default:
		break;
	}
	LinkToEn();
}

void RobotCommands::AttackGo()
{
	if (myRobot[MyRobotLoopTemp]->canUse(ScourgeSkill))
	{
		//天灾！先选出没有被天灾的机器人
		for (int temp = 0; temp < enemyRobot.size(); temp++)
		{
			if (myRobot[MyRobotLoopTemp]->pos.dist2(enemyRobot[temp]->pos) <= myRobot[MyRobotLoopTemp]->attack_range && EnHps[temp] >= -100 && !EnScou[temp] && !MyLinks[MyRobotLoopTemp][MyPosNos[enemyRobot[temp]->pos]] && !MyLinks[MyPosNos[enemyRobot[temp]->pos]][MyRobotLoopTemp])
			{
				My_ZNT_Out << "Scourge To Enemy " << temp << endl;
				pCommand.AddOrder(Scourge, myRobot[MyRobotLoopTemp]->pos, enemyRobot[temp]->pos);
				EnScou[temp] = true;
				//找出和它连着的机器人
				for (int tp = 0; tp < enemyRobot.size(); tp++)
				{
					if (tp == temp || EnScou[tp])
						continue;
					if (MyLinks[temp + myRobot.size()][tp + myRobot.size()] || MyLinks[tp + myRobot.size()][temp + myRobot.size()])
						EnScou[tp] = true;
				}
				goto ScoOver;
			}
		}
		for (int temp = 0; temp < enemyRobot.size(); temp++)
		{
			if (myRobot[MyRobotLoopTemp]->pos.dist2(enemyRobot[temp]->pos) <= myRobot[MyRobotLoopTemp]->attack_range && EnHps[temp] >= -100)
			{
				My_ZNT_Out << "Scourge To Enemy " << temp << endl;
				pCommand.AddOrder(Scourge, myRobot[MyRobotLoopTemp]->pos, enemyRobot[temp]->pos);
				EnScou[temp] = true;
				//找出和它连着的机器人
				for (int tp = 0; tp < enemyRobot.size(); tp++)
				{
					if (tp == temp || EnScou[tp])
						continue;
					if (MyLinks[temp + myRobot.size()][tp + myRobot.size()] || MyLinks[tp + myRobot.size()][temp + myRobot.size()])
						EnScou[tp] = true;
				}
				goto ScoOver;
			}
		}
		for (int temp = 0; temp < enemyRobot.size(); temp++)
		{
			if (myRobot[MyRobotLoopTemp]->pos.dist2(enemyRobot[temp]->pos) <= myRobot[MyRobotLoopTemp]->attack_range)
			{
				My_ZNT_Out << "Scourge To Enemy " << temp << endl;
				pCommand.AddOrder(Scourge, myRobot[MyRobotLoopTemp]->pos, enemyRobot[temp]->pos);
				EnScou[temp] = true;
				//找出和它连着的机器人
				for (int tp = 0; tp < enemyRobot.size(); tp++)
				{
					if (tp == temp || EnScou[tp])
						continue;
					if (MyLinks[temp + myRobot.size()][tp + myRobot.size()] || MyLinks[tp + myRobot.size()][temp + myRobot.size()])
						EnScou[tp] = true;
				}
				goto ScoOver;
			}
		}
	}
ScoOver:;
	//轰炸！！！
	if (myRobot[MyRobotLoopTemp]->canUse(BombingSkill))
	{
		for (int temp = 0; temp < enemyRobot.size(); temp++)
		{
			if (Nearest[enemyRobot[temp]->pos.x][enemyRobot[temp]->pos.y][NearestMyDist2] <= 4)
				continue;
			if (myRobot[MyRobotLoopTemp]->pos.dist2(enemyRobot[temp]->pos) <= BombingRange && EnHps[temp] >= -100)
			{
				EnHps[temp] -= 3 * myRobot[MyRobotLoopTemp]->attack * defenseBaseValue / (defenseBaseValue + enemyRobot[temp]->defense);
				pCommand.AddOrder(Bombing, myRobot[MyRobotLoopTemp]->pos, enemyRobot[temp]->pos);
				goto BomOver;
			}
		}
		Pos BombTempPos(-1, -1);
		int BombValue = -1000;
		for (int temp = 0; temp < enemyRobot.size(); temp++)
		{
			if (myRobot[MyRobotLoopTemp]->pos.dist2(enemyRobot[temp]->pos) <= BombingRange + 4 && EnHps[temp] >= -100)
			{
				for (int px = -2; px <= 2; px++)
				{
					for (int py = -2; py <= 2; py++)
					{
						if (((enemyRobot[temp]->pos.x + px - myRobot[MyRobotLoopTemp]->pos.x) * (enemyRobot[temp]->pos.x + px - myRobot[MyRobotLoopTemp]->pos.x) + (enemyRobot[temp]->pos.y + py - myRobot[MyRobotLoopTemp]->pos.y) * (enemyRobot[temp]->pos.y + py - myRobot[MyRobotLoopTemp]->pos.y)) > BombingRange)
							continue;
						if (Nearest[enemyRobot[temp]->pos.x + px][enemyRobot[temp]->pos.y + py][BombingMyNum] == 0)
						{
							EnHps[temp] -= 3 * myRobot[MyRobotLoopTemp]->attack * defenseBaseValue / (defenseBaseValue + enemyRobot[temp]->defense);
							pCommand.AddOrder(Bombing, myRobot[MyRobotLoopTemp]->pos, Pos(enemyRobot[temp]->pos.x + px, enemyRobot[temp]->pos.y + py));
							goto BomOver;
						}
						if (Nearest[enemyRobot[temp]->pos.x + px][enemyRobot[temp]->pos.y + py][BombingEnNum] - Nearest[enemyRobot[temp]->pos.x + px][enemyRobot[temp]->pos.y + py][BombingMyNum] > BombValue)
						{
							BombValue = Nearest[enemyRobot[temp]->pos.x + px][enemyRobot[temp]->pos.y + py][BombingEnNum] - Nearest[enemyRobot[temp]->pos.x + px][enemyRobot[temp]->pos.y + py][BombingMyNum];
							BombTempPos.x = enemyRobot[temp]->pos.x + px;
							BombTempPos.y = enemyRobot[temp]->pos.y + py;
						}
					}
				}
			}
		}
		if (BombTempPos != Pos(-1, -1))
		{
			pCommand.AddOrder(Bombing, myRobot[MyRobotLoopTemp]->pos, BombTempPos);
			goto BomOver;
		}
	}
BomOver:;
}

void RobotCommands::DefenseGo()
{
	if (myRobot[MyRobotLoopTemp]->canUse(RecoverSkill))
	{
		if (myRobot[MyRobotLoopTemp]->hp < myRobot[MyRobotLoopTemp]->max_hp * 0.25)
		{
			pCommand.AddOrder(Recover, myRobot[MyRobotLoopTemp]->pos, myRobot[MyRobotLoopTemp]->pos);
			cout << myRobot[MyRobotLoopTemp]->pos.x << " , " << myRobot[MyRobotLoopTemp]->pos.y << " , recover!!!\n";
			goto RecoverOver;
		}
		vector<const Robot *> targets;
		for (Edge e : playerInfo.edges)
			if (e.source == myRobot[MyRobotLoopTemp]->pos && !e.LeftTime && MyPosNos[e.target] < myRobot.size())
				targets.push_back(playerInfo.RobotAt(e.target));
		if (!targets.empty())
		{
			sort(targets.begin(), targets.end(), cmpHpOut);
			pCommand.AddOrder(Recover, myRobot[MyRobotLoopTemp]->pos, targets.front()->pos);
			cout << myRobot[MyRobotLoopTemp]->pos.x << " , " << myRobot[MyRobotLoopTemp]->pos.y << " , recover!!!\n";
		}
		else
		{
			pCommand.AddOrder(Recover, myRobot[MyRobotLoopTemp]->pos, myRobot[MyRobotLoopTemp]->pos);
			cout << myRobot[MyRobotLoopTemp]->pos.x << " , " << myRobot[MyRobotLoopTemp]->pos.y << " , recover!!!\n";
		}
	}
RecoverOver:;
	if (myRobot[MyRobotLoopTemp]->canUse(ShieldingSkill))
	{
		for (Robot* MyShieldingTar : myFrontRobot)
			if (myRobot[MyRobotLoopTemp]->pos.dist2(MyShieldingTar->pos) < ShieldingRadius)
			{
				for (auto BufNow : MyShieldingTar->buffs)
					if (BufNow.type == ShieldingBuff)
						goto AgainShield;
				pCommand.AddOrder(Shielding, myRobot[MyRobotLoopTemp]->pos, MyShieldingTar->pos);
				goto ShieldOver;
			AgainShield:;
			}
		for (Robot* MyShieldingTar : myRobot)
			if (myRobot[MyRobotLoopTemp]->pos.dist2(MyShieldingTar->pos) < ShieldingRadius)
			{
				for (auto BufNow : MyShieldingTar->buffs)
					if (BufNow.type == ShieldingBuff)
						goto Again2Shield;
				pCommand.AddOrder(Shielding, myRobot[MyRobotLoopTemp]->pos, MyShieldingTar->pos);
				goto ShieldOver;
			Again2Shield:;
			}
	}
ShieldOver:;
	for (auto Mytarget : myFrontRobot)
	{
		if (MyLinksNum[MyRobotLoopTemp] >= myRobot[MyRobotLoopTemp]->outdeg)
			break;
		if (myRobot[MyRobotLoopTemp]->pos.dist2(Mytarget->pos) <= SplitRange && myRobot[MyRobotLoopTemp]->pos.dist2(Mytarget->pos) > 0 && (!MyTryLinks[MyRobotLoopTemp][MyRobotNos[Mytarget]]))
		{
			pCommand.AddOrder(Connect, myRobot[MyRobotLoopTemp]->pos, Mytarget->pos);
			MyLinksNum[MyRobotLoopTemp]++;
		}
	}
}

void RobotCommands::GatherGo()
{
	if ((freeEnergy < FixedSplitCost && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16Num] + Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][WallsNum] < 4))
	{
		for (Buff myb : myRobot[MyRobotLoopTemp]->buffs)
			if (myb.type == OverloadHighBuff || myb.type == OverloadLowBuff)
				goto LoadOver;
		pCommand.AddOrder(Overload, myRobot[MyRobotLoopTemp]->pos);
	}
	else if (myRobot[MyRobotLoopTemp]->energy > 2500)
	{
		for (Buff myb : myRobot[MyRobotLoopTemp]->buffs)
			if (myb.type == OverloadHighBuff || myb.type == OverloadLowBuff)
				goto LoadOver;
		for (Edge e : playerInfo.edges)
		{
			if (e.source == myRobot[MyRobotLoopTemp]->pos && e.LeftTime == 1)
			{
				pCommand.AddOrder(Overload, myRobot[MyRobotLoopTemp]->pos);
				goto LoadOver;
			}
		}
	}
LoadOver:;

	if (myRobot[MyRobotLoopTemp]->canUse(AirborneSkill))
	{
		if (freeEnergy <= AirBornUseEnergy)
		{
			need[myRobot[MyRobotLoopTemp]->pos] += AirBornUseEnergy - freeEnergy + 100;
		}
		else
		{

			int AirBornEn = freeEnergy > AirBornLeftEnergy + AirBornUseEnergy ? freeEnergy - AirBornLeftEnergy : AirBornUseEnergy;

			if (No_Turns <= 30 && AirBornEn > 2400)
				AirBornEn = 2400;
			if (AirBornEn < 1000)
				AirBornEn = 1000;

			Pos TargetFarPos(-1, -1);
			int FarNum = 120;

			if (!FirstExpand)
			{
				if (ToEnFirst)
				{
					Robot* EnTaFirst = NULL;
					for (Robot* EnTarGet : enemyRobot)
					{
						//扰乱一下
						if (EnTaFirst == NULL || EnTaFirst->energy < EnTarGet->energy)
							EnTaFirst = EnTarGet;
					}
					if (EnTaFirst)
					{
						for (Pos BuildPos : buildablePos)
						{
							if (BuildPos.dist2(EnTaFirst->pos) < 2)
							{
								AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
								freeEnergy -= AirBornEn;
								pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
								My_ZNT_Out << "GoToEn Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
								ToEnFirst = false;
								goto AirOver;
							}
						}
					}
				}
			}
			else
				FirstExpand = false;


			if (!FinalAttack)
			{
				//120以上的，而且离边界比较远
				for (Pos BuildPos : buildablePos)
				{
					if (Nearest[BuildPos.x][BuildPos.y][NearestDist2] >= 120 && BuildPos.x >= 4 && BuildPos.y >= 4 && BuildPos.x < MAP_SIZE - 4 && BuildPos.y < MAP_SIZE - 4)
					{
						for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
							for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
								if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 120)
									goto Air0Again;
						AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
						My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
						goto AirOver;
					}
				Air0Again:;
				}


				for (Robot* EnTarGet : enemyRobot)
				{
					if (No_Turns > 4 && Nearest[EnTarGet->pos.x][EnTarGet->pos.y][En_BombNum] == 1)
					{
						for (Pos BuildPos : buildablePos)
						{
							if (BuildPos.dist2(EnTarGet->pos) <= 13 && Nearest[BuildPos.x][BuildPos.y][En_BombNum] < 3)
							{
								for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
									for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
										if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 4)
											goto AirddEnddAgain;
								AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
								freeEnergy -= AirBornEn;
								pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
								My_ZNT_Out << "GoToEn Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
								goto AirOver;
							}
						AirddEnddAgain:;
						}
					}
				}

				for (Robot* EnTarGet : enemyRobot)
				{
					if (No_Turns > 4 && Nearest[EnTarGet->pos.x][EnTarGet->pos.y][En_BombNum] == 2)
					{
						for (Pos BuildPos : buildablePos)
						{
							if (BuildPos.dist2(EnTarGet->pos) <= 13 && Nearest[BuildPos.x][BuildPos.y][En_BombNum] < 3)
							{
								for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
									for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
										if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 4)
											goto AiddrddEnAgain;
								AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
								freeEnergy -= AirBornEn;
								pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
								My_ZNT_Out << "GoToEn Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
								goto AirOver;
							}
						AiddrddEnAgain:;
						}
					}
				}
				//80以上的
				for (Pos BuildPos : buildablePos)
				{
					if (Nearest[BuildPos.x][BuildPos.y][NearestDist2] >= 80)
					{
						for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
							for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
								if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 80)
									goto Air05Again;
						AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
						My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
						goto AirOver;
					}
				Air05Again:;
				}

				if (AirToEnHuman)
				{
					Pos AirHumanTarPos(-1, -1);
					int AirTarNum = 0;
					for (Pos BuildPos : buildablePos)
					{
						if (Nearest[BuildPos.x][BuildPos.y][En_In16Num] > AirTarNum && Nearest[BuildPos.x][BuildPos.y][NearestMyDist2] > 4 && Nearest[BuildPos.x][BuildPos.y][NearestEnDist2] < 3 && Nearest[BuildPos.x][BuildPos.y][En_In16Num] < 3 && Nearest[BuildPos.x][BuildPos.y][En_In16Num] > Nearest[BuildPos.x][BuildPos.y][My_In16Num])
						{
							AirTarNum = Nearest[BuildPos.x][BuildPos.y][En_In16Num];
							AirHumanTarPos = BuildPos;
						}
					}
					if (AirHumanTarPos != Pos(-1, -1))
					{
						AlreadyBuild[AirHumanTarPos.x][AirHumanTarPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, AirHumanTarPos, AirBornEn - 1000);
						My_ZNT_Out << "GoToEn Air!!! To " << AirHumanTarPos.x << " , " << AirHumanTarPos.y << endl;
						goto AirOver;
					}
					AirHumanTarPos = Pos(-1, -1);
					AirTarNum = 0;
					for (Pos BuildPos : buildablePos)
					{
						if (Nearest[BuildPos.x][BuildPos.y][En_In16Num] > AirTarNum && Nearest[BuildPos.x][BuildPos.y][NearestMyDist2] > 4 && Nearest[BuildPos.x][BuildPos.y][NearestEnDist2] < 3)
						{
							AirTarNum = Nearest[BuildPos.x][BuildPos.y][En_In16Num];
							AirHumanTarPos = BuildPos;
						}
					}
					if (AirHumanTarPos != Pos(-1, -1))
					{
						AlreadyBuild[AirHumanTarPos.x][AirHumanTarPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, AirHumanTarPos, AirBornEn - 1000);
						My_ZNT_Out << "GoToEn Air!!! To " << AirHumanTarPos.x << " , " << AirHumanTarPos.y << endl;
						goto AirOver;
					}
				}

				//加个30吧~
				for (Pos BuildPos : buildablePos)
				{
					if (Nearest[BuildPos.x][BuildPos.y][NearestDist2] >= 40 && Nearest[BuildPos.x][BuildPos.y][NearestDist2] < 80)
					{
						for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
							for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
								if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 36)
									goto Air0255Again;
						AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
						My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
						goto AirOver;
					}
				Air0255Again:;
				}

				cout << "Didn't go ! ------------------------------~~~~~~~~~~~~~~~~~~~~~~-------------" << endl;
				for (Pos BuildPos : buildablePos)
				{
					if (Nearest[BuildPos.x][BuildPos.y][NearestDist2] < 40 && Nearest[BuildPos.x][BuildPos.y][NearestDist2] >= 16/* && Nearest[BuildPos.x][BuildPos.y][En_In16Num] == 0*/)
					{
						for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
							for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
								if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 16)
									goto Air15Again;
						AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
						My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
						goto AirOver;
					}
				Air15Again:;
				}

				for (Robot* EnTarGet : enemyRobot)
				{
					if (No_Turns > 5 && Nearest[EnTarGet->pos.x][EnTarGet->pos.y][En_In16Num] == 1 && Nearest[EnTarGet->pos.x][EnTarGet->pos.y][En_BombNum] > 2)
					{
						for (Pos BuildPos : buildablePos)
						{
							if (BuildPos.dist2(EnTarGet->pos) <= 13 && Nearest[BuildPos.x][BuildPos.y][En_In16Num] < 2)
							{
								for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
									for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
										if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 4)
											goto AirddEnaaAgain;
								AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
								freeEnergy -= AirBornEn;
								pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
								My_ZNT_Out << "GoToEn Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
								goto AirOver;
							}
						AirddEnaaAgain:;
						}
					}
				}


				for (Pos BuildPos : buildablePos)
				{
					if (Nearest[BuildPos.x][BuildPos.y][NearestDist2] < 16 && Nearest[BuildPos.x][BuildPos.y][NearestDist2] >= 9)
					{
						for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
							for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
								if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 9)
									goto Air2Again;
						AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
						My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
						goto AirOver;
					}
				Air2Again:;
				}

			}
			for (Robot* EnTarGet : enemyRobot)
			{//扰乱一下
				for (Pos BuildPos : buildablePos)
				{
					if (BuildPos.dist2(EnTarGet->pos) <= 2)
					{
						for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
							for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
								if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 3)
									goto AirEn255Again;
						AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
						freeEnergy -= AirBornEn;
						pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
						My_ZNT_Out << "GoToEn Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
						goto AirOver;
					}
				AirEn255Again:;
				}
			}



			for (Pos BuildPos : buildablePos)
			{
				if (Nearest[BuildPos.x][BuildPos.y][NearestMyDist2] > 3 && Nearest[BuildPos.x][BuildPos.y][En_In16Num] > 0)
				{
					for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
						for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
							if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 3)
								goto Air25Again;
					AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
					freeEnergy -= AirBornEn;
					pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
					My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
					goto AirOver;
				}
			Air25Again:;
			}
			for (Pos BuildPos : buildablePos)
			{
				if (Nearest[BuildPos.x][BuildPos.y][NearestMyDist2] > 3)
				{
					for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
						for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
							if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 3)
								goto Air2555Again;
					AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
					freeEnergy -= AirBornEn;
					pCommand.AddOrder(Airborne, myRobot[MyRobotLoopTemp]->pos, BuildPos, AirBornEn - 1000);
					My_ZNT_Out << "Air!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
					goto AirOver;
				}
			Air2555Again:;
			}
			cout << "+++++++++++++++++++++++++++++++++++------------------------------------" << endl;
		}
	}
AirOver:;
	for (int j = 0; j < myRobot.size(); j++)
	{
		if (MyLinks[MyRobotLoopTemp][j])
		{
			if (Nearest[myRobot[j]->pos.x][myRobot[j]->pos.y][My_In16Num] + Nearest[myRobot[j]->pos.x][myRobot[j]->pos.y][WallsNum] > 4)
				pCommand.AddOrder(Disconnect, myRobot[MyRobotLoopTemp]->pos, myRobot[j]->pos);
		}
	}
	for (auto Mytarget : myRobot)
	{
		if (MyLinksNum[MyRobotLoopTemp] >= myRobot[MyRobotLoopTemp]->outdeg)
			break;
		if (myRobot[MyRobotLoopTemp]->pos.dist2(Mytarget->pos) <= SplitRange && myRobot[MyRobotLoopTemp]->pos.dist2(Mytarget->pos) > 0 && (!MyTryLinks[MyRobotLoopTemp][MyRobotNos[Mytarget]]) && Nearest[Mytarget->pos.x][Mytarget->pos.y][My_In16Num] + Nearest[Mytarget->pos.x][Mytarget->pos.y][WallsNum] <= 4)
		{
			pCommand.AddOrder(Connect, myRobot[MyRobotLoopTemp]->pos, Mytarget->pos);
			MyLinksNum[MyRobotLoopTemp]++;
		}
	}
}

void RobotCommands::LinkToEn()
{
	//和敌方连接
	for (auto Mytarget : enemyRobot)
	{
		if (MyLinksNum[MyRobotLoopTemp] >= myRobot[MyRobotLoopTemp]->outdeg)
			break;
		if (myRobot[MyRobotLoopTemp]->pos.dist2(Mytarget->pos) <= SplitRange && myRobot[MyRobotLoopTemp]->pos.dist2(Mytarget->pos) > 0 && (!MyTryLinks[MyRobotLoopTemp][MyRobotNos[Mytarget]]))
		{
			pCommand.AddOrder(Connect, myRobot[MyRobotLoopTemp]->pos, Mytarget->pos);
			MyLinksNum[MyRobotLoopTemp]++;
		}
	}
}

void RobotCommands::Back()
{
	if (myRobot[MyRobotLoopTemp]->hp > myRobot[MyRobotLoopTemp]->max_hp * 0.6)
	{
		//退化需谨慎。当周围没有敌方机器人而且周围自己的攻击机器人存在的时候，可以退一下
		if (MyNowType == AttackRobot && (((Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_BombNum] == 0/* && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num] > 1*/) || (myRobot[MyRobotLoopTemp]->efficiency * AllSpots[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] < myRobot[MyRobotLoopTemp]->consumption && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] == 0)) || (MyToGrow && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] == 0)))
		{
			if (myRobot[MyRobotLoopTemp]->level > 1)
			{
				pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, GatherRobot);
				AttackMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = false;
				for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
					for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
					{
						if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
							Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Attack_Num]--;
					}
			}
		}
		else if (MyNowType == DefenseRobot && (((Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_BombNum] == 0/* && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num] > 1*/) || (myRobot[MyRobotLoopTemp]->efficiency * AllSpots[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] < myRobot[MyRobotLoopTemp]->consumption && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] == 0)) || (MyToGrow && Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][En_In16Num] == 0)))
		{
			if (myRobot[MyRobotLoopTemp]->level > 1)
			{
				pCommand.AddOrder(Vestigial, myRobot[MyRobotLoopTemp]->pos, GatherRobot);
				DefenseMap[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y] = false;
				for (int ti = (myRobot[MyRobotLoopTemp]->pos.x - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.x - 4); ti <= (myRobot[MyRobotLoopTemp]->pos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.x + 4); ++ti)
					for (int tj = (myRobot[MyRobotLoopTemp]->pos.y - 4 < 0 ? 0 : myRobot[MyRobotLoopTemp]->pos.y - 4); tj <= (myRobot[MyRobotLoopTemp]->pos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : myRobot[MyRobotLoopTemp]->pos.y + 4); ++tj)
					{
						if (myRobot[MyRobotLoopTemp]->pos.dist2(Pos(ti, tj)) <= 16)
							Nearest[myRobot[MyRobotLoopTemp]->pos.x][myRobot[MyRobotLoopTemp]->pos.y][My_In16_Defense_Num]--;
					}
			}
		}
	}
}

void RobotCommands::TrasCommands()
{
	freeEnergy -= myRobot[MyRobotLoopTemp]->consumption;
	MyCap[MyRobotLoopTemp] = myRobot[MyRobotLoopTemp]->transport_capacity;
	if (freeEnergy > 0)
	{
		CanTrasfer[MyRobotLoopTemp] = myRobot[MyRobotLoopTemp]->transport_capacity < freeEnergy ? myRobot[MyRobotLoopTemp]->transport_capacity : freeEnergy;
	}
	else
		CanTrasfer[MyRobotLoopTemp] = 0;
}

void RobotCommands::DieSplit()
{
	for (Pos BuildPos : buildablePos)
	{
		if (BuildPos.dist2(myRobot[MyRobotLoopTemp]->pos) <= SplitRange && Nearest[BuildPos.x][BuildPos.y][NearestMyDist2] >= 3)
		{
			for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
				for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
					if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < 3)
						goto Split455555Again;
			AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
			freeEnergy -= FixedSplitCost;
			pCommand.AddOrder(Split, myRobot[MyRobotLoopTemp]->pos, BuildPos);
			My_ZNT_Out << "Split!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
			if (freeEnergy < (myRobot[MyRobotLoopTemp]->canUse(AirborneSkill) ? FixedSplitCost + 1000 : FixedSplitCost))
				break;
		}
	Split455555Again:;
	}
}

bool RobotCommands::SplitCommands(FHSplitType splitType)
{
	int FarInt = 5;
	switch (splitType)
	{
	case Farest:
		FarInt = 14;
		break;
	case Far:
		FarInt = 9;
		break;
	case ALittleFar:
		FarInt = 8;
		break;
	case NotFar:
		FarInt = 5;
		break;
	default:
		break;
	}
	for (Pos BuildPos : buildablePos)
	{
		if (BuildPos.dist2(myRobot[MyRobotLoopTemp]->pos) <= SplitRange && Nearest[BuildPos.x][BuildPos.y][NearestMyDist2] >= FarInt)
		{
			for (int ti = (BuildPos.x - 4 < 0 ? 0 : BuildPos.x - 4); ti <= (BuildPos.x + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.x + 4); ++ti)
				for (int tj = (BuildPos.y - 4 < 0 ? 0 : BuildPos.y - 4); tj <= (BuildPos.y + 4 >= MAP_SIZE ? MAP_SIZE - 1 : BuildPos.y + 4); ++tj)
					if (AlreadyBuild[ti][tj] && BuildPos.dist2(Pos(ti, tj)) < FarInt)
						goto SplitAgain;
			AlreadyBuild[BuildPos.x][BuildPos.y] = 1;
			freeEnergy -= FixedSplitCost;
			pCommand.AddOrder(Split, myRobot[MyRobotLoopTemp]->pos, BuildPos);
			My_ZNT_Out << "Split!!! To " << BuildPos.x << " , " << BuildPos.y << endl;
			if (freeEnergy < FixedSplitCost)
				return true;
		}
	SplitAgain:;
	}
	return false;
}

//准备一下输出文件，加上时间戳，将输出重定向同时解决相同名字会导致崩溃的问题
void SetStanderd()
{
	itoa(tStick / 60 % 1000000, Adder, 10);
	TargetFile = FileName + Adder + ".txt";
	while (1)
	{
		ifstream *In_Temp = NULL;
		In_Temp = new ifstream(TargetFile, ios::in);
		if (!*In_Temp)
		{
			In_Temp->close();
			delete In_Temp;
			break;
		}
		In_Temp->close();
		delete In_Temp;
		TargetFile = "FH_" + TargetFile;
	}
	My_ZNT_Out.open(TargetFile, ios::out);
}



void TransferNow()
{
	vector<TransUseOrdersStru> Orders_Of_Traf;
	//从大到小排序一下
	auto CmpNeeds = [](const Robot* l, const Robot* r)
	{
		return (need[l->pos] == 0 ? 99999 : need[l->pos]) > (need[r->pos] == 0 ? 99999 : need[r->pos]);
	};
	vector<Robot*> myRobot2;
	for (Robot* pppppp : myRobot)
		myRobot2.push_back(pppppp);
	sort(myRobot2.begin(), myRobot2.end(), CmpNeeds);
	for (int ppp = myRobot2.size() - 1; ppp >= 0; ppp--)
	{
		int TrasferNum;
		int Collected;
		int StillNeed;
		int PPPStillNeed;
		if (need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] == 0 || need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] == 99999)
			continue;
		for (Robot* Out : myRobot2)
		{
			if (CanTrasfer[MyPosNos[Out->pos]] <= 0)
				continue;
			if (!MyLinks[MyPosNos[Out->pos]][MyPosNos[myRobot2[ppp]->pos]])
				continue;
			cout << Out->transport_capacity << endl;
			cout << MyPosNos[Out->pos] << "   To  " << MyPosNos[myRobot2[ppp]->pos] << "  need:  " << need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] << "  Can:  " << CanTrasfer[MyPosNos[Out->pos]];
			if (CanTrasfer[MyPosNos[Out->pos]] > need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos])
				TrasferNum = need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos];
			else
				TrasferNum = CanTrasfer[MyPosNos[Out->pos]];
			pCommand.AddOrder(Transfer, myRobot[MyPosNos[Out->pos]]->pos, myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos, TrasferNum);
			need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] -= TrasferNum;
			CanTrasfer[MyPosNos[Out->pos]] -= TrasferNum;
			MyCap[MyPosNos[Out->pos]] -= TrasferNum;
			cout << "  Now:  " << TrasferNum << endl;
			if (need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] <= 0)
				goto PPPOver;
		}
		CanTrasfer[MyPosNos[myRobot2[ppp]->pos]] = 0;
		PPPStillNeed = need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos];
		for (Robot* Out : myRobot2)
		{
			StillNeed = PPPStillNeed > MyCap[MyPosNos[Out->pos]] ? MyCap[MyPosNos[Out->pos]] : PPPStillNeed;
			if (MyCap[MyPosNos[Out->pos]] <= 0)
				continue;
			if (!MyLinks[MyPosNos[Out->pos]][MyPosNos[myRobot2[ppp]->pos]])
				continue;
			Collected = 0;
			for (Robot* DosOut : myRobot2)
			{
				if (CanTrasfer[MyPosNos[DosOut->pos]] <= 0)
					continue;
				if (!MyLinks[MyPosNos[DosOut->pos]][MyPosNos[Out->pos]])
					continue;
				if (CanTrasfer[MyPosNos[DosOut->pos]] > StillNeed)
				{
					TrasferNum = StillNeed;
				}
				else
				{
					TrasferNum = CanTrasfer[MyPosNos[DosOut->pos]];
				}
				pCommand.AddOrder(Transfer, myRobot[MyPosNos[DosOut->pos]]->pos, myRobot[MyPosNos[Out->pos]]->pos, TrasferNum);
				StillNeed -= TrasferNum;
				CanTrasfer[MyPosNos[DosOut->pos]] -= TrasferNum;
				MyCap[MyPosNos[DosOut->pos]] -= TrasferNum;
				Collected += TrasferNum;
				if (StillNeed == 0)
					goto DosOver;
			}
		DosOver:;
			TrasferNum = Collected;
			if (TrasferNum <= 0)
			{
				continue;
			}
			pCommand.AddOrder(Transfer, myRobot[MyPosNos[Out->pos]]->pos, myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos, TrasferNum);
			need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] -= TrasferNum;
			MyCap[MyPosNos[Out->pos]] -= TrasferNum;
			PPPStillNeed -= TrasferNum;
			cout << "  Now:  " << TrasferNum << endl;
			if (need[myRobot[MyPosNos[myRobot2[ppp]->pos]]->pos] <= 0)
				goto PPPOver;
		}
	PPPOver:;
	}
}

void OutPutEn()
{
	My_ZNT_Out << "\n______________________________________\n";
	for (En_Robot_Num = 0; En_Robot_Num < enemyRobot.size(); En_Robot_Num++)
	{
		My_ZNT_Out << "\n______________________________________\n";
		My_ZNT_Out << "EnRobot: " << En_Robot_Num + 1 << "\t  Level: " << enemyRobot[En_Robot_Num]->level << "\t  Energy: " << enemyRobot[En_Robot_Num]->energy << "\t  HP: " << enemyRobot[En_Robot_Num]->hp << "     \t" << RobotTypeName[enemyRobot[En_Robot_Num]->type] << endl;
		My_ZNT_Out << "Pos: " << enemyRobot[En_Robot_Num]->pos.x << " , " << enemyRobot[En_Robot_Num]->pos.y << "\nPosInfo:" << "NearestDist2: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][NearestDist2] << " \tNum:  " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][NearestNum] << "\nNearestEnDist2: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][NearestEnDist2] << " \tNum:   " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][NearestEnNum] << "\nNearestMyDist2: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][NearestMyDist2] << " \tNum:  " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][NearestMyNum] << "\n16My: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][My_In16Num] << " \t16En: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][En_In16Num] << "\nBombMy: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][My_BombNum] << " \tBombEn: " << Nearest[enemyRobot[En_Robot_Num]->pos.x][enemyRobot[En_Robot_Num]->pos.y][En_BombNum] << endl;
	}
}

void DeleteThings()
{
	delete[] EnHps;
	delete[] EnScou;
	delete[] AlreadyLevel;
	delete[] MyWantToDie;
	delete[] MyLinksNum;
	for (int j = 0; j < myRobot.size() + enemyRobot.size(); j++)
	{
		delete[] MyLinks[j];
		delete[] MyTryLinks[j];
	}
	delete[] MyLinks;
	delete[] MyTryLinks;
	delete[] CanTrasfer;
	delete[] MyCap;
	MyRobotNos.clear();
	MyPosNos.clear();
	myRobot.clear();
	enemyRobot.clear();
	myFrontRobot.clear();
	buildablePos.clear();
	occupied.clear();
	need.clear();
}

void PreThings()
{
	GatherMaxLevel = GatherMaxLevelBase;
	if (No_Turns > 15)
	{
		AirToEnHuman = true;
		FarSplitToEn = true;
	}
	//对于所有的点，计算如果建立在那里机器人的话获得的能量
	//排序的如果不可建造，就让它的能量系数大一点
	for (int i = 0; i < MAP_SIZE; ++i)
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			AllSpots[i][j] = 0;
			RankAllSpots[i][j] = 0;
			AlreadyBuild[i][j] = 0;
			AttackMap[i][j] = 0;
			DefenseMap[i][j] = 0;
		}

	My_ZNT_Out << "_____________________________________\n\nNow: Turns Num: " << No_Turns << " , Now Info:\n";
	//清空要输出的指令
	pCommand.cmds.clear();

	if (No_Turns > 30)
		GatherMaxLevel++;
	//后面还有针对机器人个数的判断

	if (No_Turns > 180)
		GatherMaxLevel++;

	if (No_Turns > 280)
		GatherMaxLevel++;

	My_Robot_Num = 0;
	En_Robot_Num = 0;

	for (int i = 0; i < MAP_SIZE; ++i)
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			Nearest[i][j][NearestDist2] = Nearest[i][j][NearestEnDist2] = Nearest[i][j][NearestMyDist2] = 2 << 20;
			Nearest[i][j][NearestNum] = Nearest[i][j][NearestEnNum] = Nearest[i][j][NearestMyNum] = Nearest[i][j][My_In16Num] = Nearest[i][j][En_In16Num] = Nearest[i][j][My_BombNum] = Nearest[i][j][BombingMyNum] = Nearest[i][j][BombingEnNum] = Nearest[i][j][En_BombNum] = Nearest[i][j][My_In16_Attack_Num] = Nearest[i][j][My_In16_Defense_Num] = 0;
			Nearest[i][j][WallsNum] = 0;
			if (i <= 2 || i >= MAP_SIZE - 3)
				Nearest[i][j][WallsNum]++;
			if (j <= 2 || j >= MAP_SIZE - 3)
				Nearest[i][j][WallsNum]++;
		}

	//向机器人列表安排，并且算出Nearest
	for (unsigned int i = 0; i < playerInfo.robots.size(); i++)
	{
		Robot& obj = playerInfo.robots[i];
		occupied.push_back(obj.pos);
		for (int i = 0; i < MAP_SIZE; i++)
			for (int j = 0; j < MAP_SIZE; j++)
			{
				int temp = (i - obj.pos.x) * (i - obj.pos.x) + (j - obj.pos.y) * (j - obj.pos.y);
				if (temp == 0)
					continue;
				if (temp == Nearest[i][j][NearestDist2])
				{
					Nearest[i][j][NearestNum]++;
				}
				else if (temp < Nearest[i][j][NearestDist2])
				{
					Nearest[i][j][NearestDist2] = temp;
					Nearest[i][j][NearestNum] = 1;
				}
				if (obj.team == playerInfo.team)
				{
					if (temp == Nearest[i][j][NearestMyDist2])
					{
						Nearest[i][j][NearestMyNum]++;
					}
					else if (temp < Nearest[i][j][NearestMyDist2])
					{
						Nearest[i][j][NearestMyDist2] = temp;
						Nearest[i][j][NearestMyNum] = 1;
					}
					if (temp <= BombingRange)
					{
						Nearest[i][j][My_BombNum]++;
						if (temp <= 16)
						{
							Nearest[i][j][My_In16Num]++;
							if (obj.type == AttackRobot)
								Nearest[i][j][My_In16_Attack_Num]++;
							else if (obj.type == DefenseRobot)
								Nearest[i][j][My_In16_Defense_Num]++;
						}
						if (temp <= 4)
						{
							Nearest[i][j][BombingMyNum]++;
						}
					}
				}
				else
				{
					if (temp == Nearest[i][j][NearestEnDist2])
					{
						Nearest[i][j][NearestEnNum]++;
					}
					else if (temp < Nearest[i][j][NearestEnDist2])
					{
						Nearest[i][j][NearestEnDist2] = temp;
						Nearest[i][j][NearestEnNum] = 1;
					}
					if (temp <= BombingRange)
					{
						Nearest[i][j][En_BombNum]++;
						if (temp <= 16)
							Nearest[i][j][En_In16Num]++;
					}
					if (temp <= 4)
					{
						Nearest[i][j][BombingEnNum]++;
					}
				}
			}
		if (obj.team == playerInfo.team)
		{
			myRobot.push_back(&obj);
			if (Nearest[obj.pos.x][obj.pos.y][En_In16Num] > 0)
				myFrontRobot.push_back(&obj);
			AlreadyBuild[obj.pos.x][obj.pos.y] = 1;
			if (obj.type == DefenseRobot)
				DefenseMap[obj.pos.x][obj.pos.y] = true;
			else if (obj.type == AttackRobot)
				AttackMap[obj.pos.x][obj.pos.y] = true;
		}
		else
		{
			enemyRobot.push_back(&obj);
		}
	}

	//找出能够放机器人的点
	for (int x = 0; x < MAP_SIZE; ++x)
		for (int y = 0; y < MAP_SIZE; ++y)
			if (MAP[x][y] == 1 && playerInfo.RobotAt(Pos(x, y)) == NULL)
				buildablePos.push_back(Pos(x, y));

	//根据机器人个数调整
	if (myRobot.size() > 20)
		GatherMaxLevel++;

	//我方按hp百分比排序，低的排在前面
	auto cmpHp = [](const Robot *l, const Robot *r) {return l->hp * r->max_hp < r->hp * l->max_hp; };
	sort(myRobot.begin(), myRobot.end(), cmpHp);
	sort(myFrontRobot.begin(), myFrontRobot.end(), cmpHp);

	//敌方按hp与defence level作用起来的总作用排序，低的在前面
	auto cmpHpEn = [](const Robot *l, const Robot *r) {return l->hp * (r->defense + defenseBaseValue) * (r->level + 1) < r->hp * (l->defense + defenseBaseValue) * (l->level + 1); };
	sort(enemyRobot.begin(), enemyRobot.end(), cmpHpEn);

	//动态数组，记录一下敌方血量数据（如果打死了就不要再打了）
	EnHps = new int[enemyRobot.size()];

	//记录敌方已经受到天灾的
	EnScou = new bool[enemyRobot.size()];

	//记录我方现在等级
	AlreadyLevel = new int[myRobot.size()];

	//记录我方将死机器人的
	MyWantToDie = new bool[myRobot.size()];

	//记录我方每个机器人连接的个数
	MyLinksNum = new int[myRobot.size() + enemyRobot.size()];

	//我方已经建立的连接，如果不是自己的，就返回敌方编号+自己编号数
	MyLinks = new bool*[myRobot.size() + enemyRobot.size()];

	//我方已经建立和正在建立的连接
	MyTryLinks = new bool*[myRobot.size() + enemyRobot.size()];

	//我方机器人可以传输出去的能量
	CanTrasfer = new int[myRobot.size()];
	MyCap = new int[myRobot.size()];

	//初始化MyRobotNos
	for (int myT = 0; myT < myRobot.size(); myT++)
	{
		MyRobotNos[myRobot[myT]] = myT;
		MyPosNos[myRobot[myT]->pos] = myT;
		MyLinksNum[myT] = 0;
	}

	for (int myT = 0; myT < enemyRobot.size(); myT++)
	{
		MyRobotNos[enemyRobot[myT]] = myT + myRobot.size();
		MyPosNos[enemyRobot[myT]->pos] = myT + myRobot.size();
		MyLinksNum[myT + myRobot.size()] = 0;
	}

	for (int j = 0; j < myRobot.size() + enemyRobot.size(); j++)
	{
		MyLinks[j] = new bool[myRobot.size() + enemyRobot.size()];
		MyTryLinks[j] = new bool[myRobot.size() + enemyRobot.size()];
		for (int i = 0; i < myRobot.size() + enemyRobot.size(); i++)
		{
			MyLinks[j][i] = false;
			MyTryLinks[j][i] = false;
		}
	}

	//记录一下已经有的链接
	for (Edge e : playerInfo.edges)
	{
		MyTryLinks[MyPosNos[e.source]][MyPosNos[e.target]] = true;
		MyLinksNum[MyPosNos[e.source]]++;
		if (!e.LeftTime)
			MyLinks[MyPosNos[e.source]][MyPosNos[e.target]] = true;
	}

	//初始化上面三个数组
	for (int temp = 0; temp < myRobot.size(); temp++)
	{
		AlreadyLevel[temp] = myRobot[temp]->level;
	}
	for (int EnTemp = 0; EnTemp < enemyRobot.size(); EnTemp++)
	{
		EnScou[EnTemp] = false;
		EnHps[EnTemp] = enemyRobot[EnTemp]->hp;
		for (auto BufNow : enemyRobot[EnTemp]->buffs)
			if (BufNow.type == ScourgeBuff)
				EnScou[EnTemp] = true;
	}


	for (int i = 0; i < MAP_SIZE; ++i)
		for (int j = 0; j < MAP_SIZE; ++j)
		{
			//对能采集到的点遍历
			for (int ti = (i - 2 < 0 ? 0 : i - 2); ti <= (i + 2 >= MAP_SIZE ? MAP_SIZE - 1 : i + 2); ++ti)
				for (int tj = (j - 2 < 0 ? 0 : j - 2); tj <= (j + 2 >= MAP_SIZE ? MAP_SIZE - 1 : j + 2); ++tj)
					if ((i - ti) * (i - ti) + (j - tj) * (j - tj) <= GatherRange)
					{
						if (Nearest[i][j][0] > (i - ti) * (i - ti) + (j - tj) * (j - tj))
						{
							RankAllSpots[i][j] += ENERGY[ti][tj] * (2 - MAP[ti][tj]);
							AllSpots[i][j] += ENERGY[ti][tj];
						}
						else if (Nearest[i][j][0] == (i - ti) * (i - ti) + (j - tj) * (j - tj))
						{
							RankAllSpots[i][j] += (double)ENERGY[ti][tj] * (2 - MAP[ti][tj]) / (1 + Nearest[i][j][1]);
							AllSpots[i][j] += (double)ENERGY[ti][tj] / (1 + Nearest[i][j][1]);
						}
					}
		}


	auto ComAllSpots = [](Pos l, Pos r)
	{
		return RankAllSpots[l.x][l.y] > RankAllSpots[r.x][r.y];
	};
	sort(buildablePos.begin(), buildablePos.end(), ComAllSpots);
	//按照能量排序

	cout << "En Num: " << enemyRobot.size() << "\tMy Num : " << myRobot.size() << "   My Minus En :  " << (int)myRobot.size() - (int)enemyRobot.size() << endl;
}