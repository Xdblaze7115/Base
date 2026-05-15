#pragma once
#include <cstdint>
#include "Update/Offsets.hpp"
#include "Utils/Memory.hpp"

class Instance {
public:
	Process* process;
	uintptr_t self;

	Instance();
	Instance(Process* process, uintptr_t address);

	bool IsValid();

	std::string Name();
	std::string ClassName();

	Instance Parent();
	std::vector<Instance> GetChildren(); // std::vector = array of objects
	Instance FindFirstChild(const std::string& name); // const = read only var and & = do not copy data only refrence it
	Instance FindFirstChildOfClass(const std::string& class_name);
};