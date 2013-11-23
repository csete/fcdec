// FUNcube telemetry decoder test program
// Copyright (c) Phil Ashby, Duncan Hills, Howard Long, 2013
// Released under the terms of the Creative Commons BY-SA-NC
// https://creativecommons.org/licenses/by-nc-sa/3.0/

#pragma once

class IDecodeResult
{
public:
	virtual void OnDecodeSuccess(U8* result, U32 length, U32 centreBin) = 0;
};

