#include "Vmd.h"
#include "Encoding.h"

namespace Vmd
{
	using namespace Utility;

	void BoneFrame::Fill( bufferstream& is, bool bRH )
	{
		NameFieldBuf buffer;
		Read( is, buffer );
		BoneName = sjis_to_utf( buffer );
		Read( is, Frame );
		ReadPosition( is, Offset, bRH );
		ReadRotation( is, Rotation, bRH );
		Read( is, Interpolation );
	}

	void FaceFrame::Fill( bufferstream& is )
	{
		NameFieldBuf buffer;
		Read( is, buffer );
		FaceName = sjis_to_utf( buffer );
		Read( is, Frame );
		Read( is, Weight );
	}

	void CameraFrame::Fill( bufferstream& is, bool bRH )
	{
		Read( is, Frame );
		Read( is, Distance );
		ReadPosition( is, Position, bRH );
		ReadRotation( is, Rotation, bRH );
		Read( is, Interpolation );
		Read( is, ViewAngle );
		Read( is, TurnOffPerspective );
	}

	void SelfShadowFrame::Fill( bufferstream & is)
	{
		Read( is, Frame );
		Read( is, Mode );
		Read( is, Distance);
	}

	void LightFrame::Fill( bufferstream & is, bool bRH )
	{
		Read( is, Frame );
		Read( is, Color );
		ReadPosition( is, Position, bRH );
	}

	void IkFrame::Fill( bufferstream & is )
	{
		Read( is, Frame );
		Read( is, Visible );
		int32_t numIK;
		Read( is, numIK );
		IkEnable.resize( numIK );
		for (auto i = 0; i < numIK; i++)
		{
			NameBuf buffer;
			Read( is, buffer );
			IkEnable[i].IkName = Utility::sjis_to_utf( buffer );
			Read( is, IkEnable[i].Enable );
		}
	}

	void VMD::Fill( bufferstream & is, bool bRH )
	{
        m_IsValid = false;

		// magic and version
		NameBuf Magic;
		Read( is, Magic );
		if (strncmp( Magic, "Vocaloid Motion Data", 20 ))
		{
			std::cerr << "invalid vmd file." << std::endl;
			return;
		}
		char verBuf[10];
		Read( is, verBuf );
		Version = std::atoi( verBuf );

		// Name
		NameBuf nameBuf;
		Read( is, nameBuf );
		Name = Utility::sjis_to_utf( nameBuf );

		// Bone frames
		int32_t BoneFrameNum;
		Read( is, BoneFrameNum );
		BoneFrames.resize( BoneFrameNum );
		for (int i = 0; i < BoneFrameNum; i++)
			BoneFrames[i].Fill( is, bRH );

		// Face frames
		int32_t FaceFrameNum;
		Read( is, FaceFrameNum );
		FaceFrames.resize( FaceFrameNum );
		for (int i = 0; i < FaceFrameNum; i++)
			FaceFrames[i].Fill( is );

		// camera frames
		int32_t CameraFrameNum;
		Read( is, CameraFrameNum );
		CameraFrames.resize( CameraFrameNum );
		for (int i = 0; i < CameraFrameNum; i++)
			CameraFrames[i].Fill( is, bRH );

		// light frames
		int32_t LightFrameNum;
		Read( is, LightFrameNum );
		LightFrames.resize( LightFrameNum );
		for (auto i = 0; i < LightFrameNum; i++)
			LightFrames[i].Fill( is, bRH );

		int32_t SelfShadowFrameNum;
		Read( is, SelfShadowFrameNum );
		SelfShadowFrames.resize( SelfShadowFrameNum );
		for (auto i = 0; i < SelfShadowFrameNum; i++)
			SelfShadowFrames[i].Fill( is );

		// Ik frames
		if (is.peek() != std::ios::traits_type::eof())
		{
			int32_t IkNum;
			Read( is, IkNum );
			IKFrames.resize( IkNum );
			for (auto i = 0; i < IkNum; i++)
				IKFrames[i].Fill( is );
		}

		if (is.peek() != std::ios::traits_type::eof())
			std::cerr << "vmd stream has unknown data." << std::endl;

        m_IsValid = true;
	}
}
