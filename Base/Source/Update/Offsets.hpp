#pragma once
#include <cstdint>

namespace Offsets {
	namespace TaskScheduler {
		inline uintptr_t Pointer = 0x0;
		inline uintptr_t FpsCap = 0x0;
	}

	namespace VisualEngine {
		inline uintptr_t Pointer = 0x7BD71F8;
		inline uintptr_t RenderView = 0xB40;
		inline uintptr_t ViewMatrix = 0x130;
	}

	namespace FakeDataModel {
		inline uintptr_t Pointer = 0x74F8758;
		inline uintptr_t DataModel = 0x1D0;
	}

	namespace DataModel {
		inline uintptr_t PlaceId = 0x1A0;
		inline uintptr_t GameId = 0x198;
		inline uintptr_t ServerIP = 0x620;
	}

	namespace Instance {
		inline uintptr_t Name = 0xB0;
		inline uintptr_t Parent = 0x70;
		inline uintptr_t Children = 0x78;
		inline uintptr_t ClassDescriptor = 0x18;
	}

	namespace ClassDescriptor {
		inline uintptr_t ClassName = 0x8;
	}

	namespace Player {
		inline uintptr_t UserId = 0x2D8;
		inline uintptr_t DisplayName = 0x130;
		inline uintptr_t LocalPlayer = 0x138;
		inline uintptr_t ModelInstance = 0x3A8;
		inline uintptr_t CameraMaxZoomDistance = 0x330;
		inline uintptr_t CameraMinZoomDistance = 0x334;
	}

	namespace Humanoid {
		inline uintptr_t WalkSpeed = 0x1DC;
		inline uintptr_t WalkSpeedCheck = 0x3C4;
		inline uintptr_t UseJumpPower = 0x1EC;
		inline uintptr_t JumpPower = 0x1B0;
		inline uintptr_t JumpHeight = 0x1AC;
		inline uintptr_t HipHeight = 0x1A0;
	}
}