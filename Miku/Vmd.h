﻿//
// Modification from Vmd Parser MMDFormats
// https://github.com/oguna/MMDFormats
// License: CC0 1.0 Universal
//
#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>

#include "FileUtility.h"
#include "Encoding.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <ostream>

namespace Vmd
{
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT4;

	struct BoneFrame
	{
	public:
		std::wstring BoneName;
		int Frame;
		XMFLOAT3 Translate;
		XMFLOAT4 Rotation; // Quaternion
		char interpolation[4][4][4];

		void Fill(std::istream* stream, bool bRH );
	};

	/// 表情フレーム
	class VmdFaceFrame
	{
	public:
		/// 表情名
		std::wstring face_name;
		/// 表情の重み
		float weight;
		/// フレーム番号
		uint32_t frame;

		void Fill(std::istream* stream)
		{
			char buffer[15];
			stream->read((char*) &buffer, sizeof(char) * 15);
			face_name = Utility::sjis_to_utf(buffer);
			stream->read((char*) &frame, sizeof(int));
			stream->read((char*) &weight, sizeof(float));
		}
	};

	/// カメラフレーム
	class CameraFrame
	{
	public:
		/// フレーム番号
		int frame;
		/// 距離
		float distance;
		/// 位置
		float position[3];
		/// 回転
		float orientation[3];
		/// 補間曲線
		char interpolation[6][4];
		/// 視野角
		float angle;
		/// 不明データ
		char unknown[3];

		void Read(std::istream *stream)
		{
			stream->read((char*) &frame, sizeof(int));
			stream->read((char*) &distance, sizeof(float));
			stream->read((char*) position, sizeof(float) * 3);
			stream->read((char*) orientation, sizeof(float) * 3);
			stream->read((char*) interpolation, sizeof(char) * 24);
			stream->read((char*) &angle, sizeof(float));
			stream->read((char*) unknown, sizeof(char) * 3);
		}

		void Write(std::ostream *stream)
		{
			stream->write((char*)&frame, sizeof(int));
			stream->write((char*)&distance, sizeof(float));
			stream->write((char*)position, sizeof(float) * 3);
			stream->write((char*)orientation, sizeof(float) * 3);
			stream->write((char*)interpolation, sizeof(char) * 24);
			stream->write((char*)&angle, sizeof(float));
			stream->write((char*)unknown, sizeof(char) * 3);
		}
	};

	/// ライトフレーム
	class LightFrame
	{
	public:
		/// フレーム番号
		int frame;
		/// 色
		float color[3];
		/// 位置
		float position[3];

		void Read(std::istream* stream)
		{
			stream->read((char*) &frame, sizeof(int));
			stream->read((char*) color, sizeof(float) * 3);
			stream->read((char*) position, sizeof(float) * 3);
		}

		void Write(std::ostream* stream)
		{
			stream->write((char*)&frame, sizeof(int));
			stream->write((char*)color, sizeof(float) * 3);
			stream->write((char*)position, sizeof(float) * 3);
		}
	};

	/// IKの有効無効
	class VmdIkEnable
	{
	public:
		std::wstring ik_name;
		bool enable;
	};

	/// IKフレーム
	class IkFrame
	{
	public:
		int frame;
		bool display;
		std::vector<VmdIkEnable> ik_enable;

		void Read(std::istream *stream)
		{
			char buffer[20];
			stream->read((char*) &frame, sizeof(int));
			stream->read((char*) &display, sizeof(uint8_t));
			int ik_count;
			stream->read((char*) &ik_count, sizeof(int));
			ik_enable.resize(ik_count);
			for (int i = 0; i < ik_count; i++)
			{
				stream->read(buffer, 20);
				ik_enable[i].ik_name = Utility::sjis_to_utf(buffer);
				stream->read((char*) &ik_enable[i].enable, sizeof(uint8_t));
			}
		}

		void Write(std::ostream *stream)
		{
			stream->write((char*)&frame, sizeof(int));
			stream->write((char*)&display, sizeof(uint8_t));
			int ik_count = static_cast<int>(ik_enable.size());
			stream->write((char*)&ik_count, sizeof(int));
			for (int i = 0; i < ik_count; i++)
			{
				const VmdIkEnable& ik_enable = this->ik_enable.at(i);
				stream->write((char*)ik_enable.ik_name.c_str(), 20);
				stream->write((char*)&ik_enable.enable, sizeof(uint8_t));
			}
		}
	};

	class VmdMotion
	{
	public:
		std::wstring Name;
		int Version;
		std::vector<BoneFrame> BoneFrames;
		std::vector<VmdFaceFrame> face_frames;
		std::vector<CameraFrame> camera_frames;
		std::vector<LightFrame> light_frames;
		std::vector<IkFrame> ik_frames;

		static std::unique_ptr<VmdMotion> LoadFromFile( char const *filename, bool bRH )
		{
			std::ifstream stream(filename, std::ios::binary);
			auto result = LoadFromStream( &stream, bRH );
			stream.close();
			return result;
		}

		static std::unique_ptr<VmdMotion> LoadFromStream(std::istream *stream, bool bRH )
		{
			char buffer[30];
			auto result = std::make_unique<VmdMotion>();

			// magic and version
			stream->read((char*) buffer, 30);
			if (strncmp(buffer, "Vocaloid Motion Data", 20))
			{
				std::cerr << "invalid vmd file." << std::endl;
				return nullptr;
			}
			result->Version = std::atoi(buffer + 20);

			// name
			stream->read(buffer, 20);
			result->Name = Utility::sjis_to_utf(buffer);

			// bone frames
			int bone_frame_num;
			stream->read((char*) &bone_frame_num, sizeof(int));
			result->BoneFrames.resize(bone_frame_num);
			for (int i = 0; i < bone_frame_num; i++)
			{
				result->BoneFrames[i].Fill( stream, bRH );
			}

			// face frames
			int face_frame_num;
			stream->read((char*) &face_frame_num, sizeof(int));
			result->face_frames.resize(face_frame_num);
			for (int i = 0; i < face_frame_num; i++)
			{
				result->face_frames[i].Fill(stream);
			}

			// camera frames
			int camera_frame_num;
			stream->read((char*) &camera_frame_num, sizeof(int));
			result->camera_frames.resize(camera_frame_num);
			for (int i = 0; i < camera_frame_num; i++)
			{
				result->camera_frames[i].Read(stream);
			}

			// light frames
			int light_frame_num;
			stream->read((char*) &light_frame_num, sizeof(int));
			result->light_frames.resize(light_frame_num);
			for (int i = 0; i < light_frame_num; i++)
			{
				result->light_frames[i].Read(stream);
			}

			// unknown2
			stream->read(buffer, 4);

			// ik frames
			if (stream->peek() != std::ios::traits_type::eof())
			{
				int ik_num;
				stream->read((char*) &ik_num, sizeof(int));
				result->ik_frames.resize(ik_num);
				for (int i = 0; i < ik_num; i++)
				{
					result->ik_frames[i].Read(stream);
				}
			}

			if (stream->peek() != std::ios::traits_type::eof())
			{
				std::cerr << "vmd stream has unknown data." << std::endl;
			}

			return result;
		}
	};
}

