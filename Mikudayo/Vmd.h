//
// Vmd Parser MMDFormats
// https://github.com/oguna/MMDFormats
//
// VMD Memo
// http://blog.goo.ne.jp/torisu_tetosuki/
//
#pragma once

#include <iosfwd>
#include <DirectXMath.h>
#include <vector>
#include <string>

namespace Utility
{
	using StorageType = std::istream::char_type;
	using bufferstream = std::basic_istream<StorageType, char_traits<StorageType>>;
}

namespace Vmd
{
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT4;
	using namespace Utility;

	using NameFieldBuf = char[15];
	using NameBuf = char[20];

	struct BoneFrame
	{
		std::wstring BoneName;
		int32_t Frame;
		XMFLOAT3 Offset; // Bone location relative offset
		XMFLOAT4 Rotation; // Quaternion
		char Interpolation[4][4][4];

		void Fill( bufferstream& is, bool bRH );
	};

	struct FaceFrame
	{
		std::wstring FaceName;
		float Weight;
		uint32_t Frame;
		void Fill( bufferstream& is );
	};

	struct CameraFrame
	{
		uint32_t Frame;
		float Distance;
		XMFLOAT3 Position; // Center of rotation
		XMFLOAT3 Rotation; // Euler angle
		uint8_t Interpolation[6][4];
		uint32_t ViewAngle;
		uint8_t TurnOffPerspective; // 0:On, 1:Off

		void Fill( bufferstream& is, bool bRH );
	};

	struct LightFrame
	{
		uint32_t Frame;
		XMFLOAT3 Color;
		XMFLOAT3 Position;

		void Fill( bufferstream& is, bool bRH );
	};

	struct SelfShadowFrame
	{
		uint32_t Frame;
		uint8_t Mode; // 00-02
		float Distance; // 0.1 - (dist * 0.00001)

		void Fill( bufferstream& is );
	};

	struct IkEnable
	{
		std::wstring IkName;
		uint8_t Enable;
	};

	struct IkFrame
	{
		uint32_t Frame;
		uint8_t Visible;
		std::vector<IkEnable> IkEnable;

		void Fill( bufferstream& is );
	};

	class VMD
	{
	public:
		std::wstring Name;
		int Version;
		std::vector<BoneFrame> BoneFrames;
		std::vector<FaceFrame> FaceFrames;
		std::vector<CameraFrame> CameraFrames;
		std::vector<LightFrame> LightFrames;
		std::vector<SelfShadowFrame> SelfShadowFrames;
		std::vector<IkFrame> IKFrames;

        VMD() : m_IsValid(false) {}
		void Fill( bufferstream& is, bool bRH );
        bool IsValid() const { return m_IsValid; }
        bool m_IsValid;
	};
}