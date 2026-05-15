workspace "Base"
	architecture "x64"
	configurations {
		"Debug",
		"Release",
		"Dist"
	}

	platforms {
		"Windows"
	}

	output_directory = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	include_directory = {}

	project "Base"
		location "Base"
		language "C++"
		cppdialect "C++20"
		staticruntime "on"

		targetdir ("Build/" .. output_directory .. "/%{prj.name}")
		objdir ("Build/" .. output_directory .. "/%{prj.name}/Intermediates")

		files {
			"%{prj.name}/Source/**",
			"%{prj.name}/Vendor/**"
		}

		includedirs {
			"%{prj.name}/Source",
			"%{prj.name}/Vendor"
		}

		filter "platforms:windows"
			system "Windows"
			systemversion "latest"
			kind "ConsoleApp"

			links {
				"ws2_32",
				"iphlpapi",
			}

			defines {
				"_CRT_SECURE_NO_WARNINGS",
    			"_WINSOCK_DEPRECATED_NO_WARNINGS",
				"WIN32_LEAN_AND_MEAN"
			}

		filter "configurations:Debug"
			defines "POLAR_DEBUG"
			runtime "Debug"
			optimize "off"
			symbols "on"

		filter "configurations:Release"
			defines "POLAR_RELEASE"
			runtime "Release"
			optimize "speed"
			symbols "off"

		filter "configurations:Dist"
			defines "POLAR_DIST"
			runtime "Release"
			optimize "full"
			symbols "off"