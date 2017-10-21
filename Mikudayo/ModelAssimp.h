//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//

#pragma once

#include "MiniModel.h"

class AssimpModel : public MiniModel
{
public:

	enum
	{
		format_none = 0,
		format_h3d, // native format

		formats,
	};
	static const char *s_FormatString[];
	static int FormatFromFilename(const char *filename);

    virtual bool Load( const ModelInfo& Info ) override;
	bool Export(const char* filename) const;

private:

    bool Import( const char* filename );
	bool ImportAssimp(const char *filename);

	void Optimize();
	void OptimizeRemoveDuplicateVertices(bool depth);
	void OptimizePostTransform(bool depth);
	void OptimizePreTransform(bool depth);
};

