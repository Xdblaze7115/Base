#include "DataModel.hpp"

DataModel::DataModel(Process* process, uintptr_t address) :
	Instance(process, address)
{}

DataModel DataModel::FetchDataModel(Process* process) {
	auto fake_datamodel_ptr = process->Read<uintptr_t>(process->image_base + Offsets::FakeDataModel::Pointer);
	auto datamodel_ptr = process->Read<uintptr_t>(fake_datamodel_ptr + Offsets::FakeDataModel::DataModel);
	return DataModel(process, datamodel_ptr);
}

Instance DataModel::GetService(const std::string& class_name) {
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