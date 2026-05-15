#include <Windows.h>
#include "Logger/Logger.hpp"
#include "Update/Offsets.hpp"
#include "Utils/Memory.hpp"
#include "Roblox/DataModel.hpp"

int main() {
	Logger* logger = new Logger();
	Ntdll::Initialize();

	Process* roblox = new Process("RobloxPlayerBeta.exe", "RobloxCrashHandler.exe");
	roblox->Attach();

	DataModel datamodel = DataModel::FetchDataModel(roblox);
	Logger::Singleton()->Printf(LOG_INFO, "Found DataModel: 0x%p", datamodel.self);
	Logger::Singleton()->Printf(LOG_INFO, "DataModel ClassName: %s", datamodel.ClassName().c_str()); // c_str = char data member of std::string which the console needs since std::string is a class not a data type
	Logger::Singleton()->Printf(LOG_INFO, "DataModel Name: %s", datamodel.Name().c_str());

	Instance players = datamodel.GetService("Players");
	Logger::Singleton()->Printf(LOG_INFO, "Found Players service: 0x%p", players.self);

	// made a custom logger for nice colours and formatting!

	// your goal is to
	// get the players service DataModel.GetService("Players") then read players + Offsets::Player::LocalPlayer then
	// construct an Instance class using the process and address of the local player then localplayer + Offsets::Player::ModelInstance then
	// construct another Instance class and call it "Character" then get the Humanoid aka the Instance that basically controls the character then
	// write <float> humanoid.self + Offsets::Humanoid::WalkSpeed and also write <float> humanoid.self + Offsets::Humanoid::WalkSpeedCheck with the exact same value
	// otherwise u will be kicked since its an old anti cheat measure

	delete roblox;
	delete logger;
	return 0;
}