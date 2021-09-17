#pragma once
#include <string>

enum ExportOptions : uint32_t
{
	ExportModels = 0x2,
	ExportLights = 0x4,
	ExportCameras = 0x8,
	ExportGrasses = 0x16,
	ExportAll = 0xFFFFFFFF
};

namespace Keywords
{	
	namespace SceneImporter
	{
		// Config
		const std::string s_Config			= "config";
		const std::string s_Grasses			= "grasses";

		// Object Array
		const std::string s_Models			= "models";
		const std::string s_Instances		= "instances";
		const std::string s_Lights			= "lights";
		const std::string s_Cameras			= "cameras";
		const std::string s_Patches			= "patches";

		// String Variable
		const std::string s_Name			= "name";
		const std::string s_File			= "file";
		const std::string s_Type			= "type";

		// Float Vector3 Variable
		const std::string s_Pos				= "pos";
		const std::string s_Dir				= "dir";
		const std::string s_Up			    = "up";
		const std::string s_Down			= "down";
		const std::string s_Left			= "left";
		const std::string s_Right			= "right";
		const std::string s_Center			= "center";
		const std::string s_Target			= "target";
		const std::string s_YawPitchRoll    = "yaw_pitch_roll";
		const std::string s_WindDir			= "wind_dir";

		// Float Scalar Variable
		const std::string s_Scale			= "scale";	
		const std::string s_Radius			= "radius";	
		const std::string s_Intensity		= "intensity";
		const std::string s_OpeningAngle	= "opening_angle";
		const std::string s_PenumbraAngle	= "penumbra_angle";
		const std::string s_Aspect			= "aspect";
		const std::string s_FovY			= "fovy";
		const std::string s_Zn				= "zn";
		const std::string s_Zf				= "zf";
		const std::string s_MoveSpeed		= "move_speed";
		const std::string s_RotateSpeed		= "rotate_speed";
		const std::string s_Height          = "height";
		const std::string s_Thickness       = "thickness";
		const std::string s_WindStrength    = "wind_strength";
		const std::string s_PosJitterStrength			= "pos_jitter_strength";
		const std::string s_BendStrengthAlongTangent	= "bend_strength_along_tangent";

		// Int  Scalar Variable
		const std::string s_PatchDim		= "patch_dim";

		// keywords String
		const std::string s_Basic			= "basic";
		const std::string s_Sdf				= "sdf";
		const std::string s_Directional		= "directional";
		const std::string s_Point			= "point";
		const std::string s_Walkthrough		= "walkthrough";
		const std::string s_Static			= "static";
		const std::string s_Dynamic			= "dynamic";
	}
}

