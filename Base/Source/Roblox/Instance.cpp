#include "Instance.hpp"

Instance::Instance() :
	process(nullptr),
	self(0)
{}

Instance::Instance(Process* process, uintptr_t address) :
	process(process),
	self(address)
{}

bool Instance::IsValid() {
	return process != 0 && self != 0;
}

std::string Instance::Name() {
	if (!IsValid()) { // return here with invalid data otherwise it will attempt to access an invalid pointer which causes ur program to crash
		return "";
	}
	return process->ReadString(process->Read<uintptr_t>(self + Offsets::Instance::Name)); // boost::copy_on_write_ptr<std::string>
}

std::string Instance::ClassName() {
	if (!IsValid()) {
		return "";
	}

	auto class_descriptor_ptr = process->Read<uintptr_t>(self + Offsets::Instance::ClassDescriptor);
	auto class_name_ptr = process->Read<uintptr_t>(class_descriptor_ptr + Offsets::ClassDescriptor::ClassName); // boost::copy_on_write_ptr<std::string>
	return process->ReadString(class_name_ptr);
}

Instance Instance::Parent() {
	if (!IsValid()) {
		return Instance();
	}
	return Instance(process, process->Read<uintptr_t>(self + Offsets::Instance::Parent)); // Instance* parent
} // we read it as an uintptr_t to derefrence it aka remove the * to get Instance parent; so we can access its members
// and you can use "pointer->member = 0" or "*pointer.member = 0" to derefrence the pointer to get the actual instance of the class

std::vector<Instance> Instance::GetChildren() {
	if (!IsValid()) {
		return std::vector<Instance>();
	}

	auto copy_on_write_ptr = process->Read<uintptr_t>(self + Offsets::Instance::Children); // boost::copy_on_write_ptr
	auto vector = process->Read<vector_block>(copy_on_write_ptr); // std::vector

	std::vector<Instance> children;
	for (auto ptr = vector.first; ptr < vector.last; ptr += 0x10) {
		auto shared_ptr = process->Read<shared_ptr_block>(ptr); // boost::shared_ptr
		children.emplace_back(process, shared_ptr.ptr);
	}
	return children;
}

Instance Instance::FindFirstChild(const std::string& name) {
	if (!IsValid()) {
		return Instance();
	}

	for (auto& instance : GetChildren()) {
		if (instance.Name() == name) {
			return instance;
		}
	}
	return Instance();
}

Instance Instance::FindFirstChildOfClass(const std::string& class_name) {
	if (!IsValid()) {
		return Instance();
	}

	for (auto& instance : GetChildren()) {
		if (instance.ClassName() == class_name) {
			return instance;
		}
	}
	return Instance();
}