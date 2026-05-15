#pragma once
#include "Instance.hpp"

// class Inheriting : public Instance = Inherit members and methods from Instance to this new class
class DataModel : public Instance {
public:
	DataModel(Process* process, uintptr_t address);

	static DataModel FetchDataModel(Process* process);
	Instance GetService(const std::string& class_name);
};