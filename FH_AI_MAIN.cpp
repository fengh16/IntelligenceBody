/**
* FH_AI_MAIN.cpp
*
* 包含具体实现智能体AI的接口
*
* 该文件进行上述函数的具体实现
*
* 冯昊
*
* V3.5.2  2017/05/11
*
* CopyRight 2017
*/


#include "FH_AI.h"

extern "C"
{
	//普通AI接口，一回合调用一次
	EXPORT void player_ai(const PlayerInfo &playerInfo_, Commands &pCommand_, PShowInfo &pShowInfo_) {
		cout << "TurnStart" << endl;
		if (No_Turns == 0)
		{
			SetStanderd();
		}
		No_Turns++;
		playerInfo = SDK::interpretPlayerInfo(playerInfo_);
		pShowInfo = &pShowInfo_;
		NotDecentAI::PrePare();
		pCommand_ = SDK::translateCommands(playerInfo, pCommand);
	}
	//高速AI接口，播放器一帧调用一次(内存空间与player_ai独立)
	EXPORT void feedback_ai(vector<HumanOrder> orders, PShowInfo &pShowInfo_) {
		parser_feedback.isFeedBack = true;
		pShowInfo = &pShowInfo_;
		parser_feedback.parse(orders);
		return;
	}
}