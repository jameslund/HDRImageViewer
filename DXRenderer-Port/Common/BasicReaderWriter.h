//********************************************************* 
// 
// Copyright (c) Microsoft. All rights reserved. 
// This code is licensed under the MIT License (MIT). 
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY 
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR 
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT. 
// 
//*********************************************************

#pragma once

#include "pch.h"

using namespace winrt;

// A simple reader/writer class that provides support for reading and writing
// files on disk. Provides synchronous and asynchronous methods.
class BasicReaderWriter
{
private:
    Windows::Storage::StorageFolder m_location{ nullptr };

public:
    BasicReaderWriter();
    BasicReaderWriter(Windows::Storage::StorageFolder const& folder);

    std::vector<byte> ReadData(std::wstring filename);

    concurrency::task<std::vector<byte>> ReadDataAsync(std::wstring filename);

    void WriteData(
        std::wstring filename,
        const std::vector<byte> fileData
        );

    Windows::Foundation::IAsyncAction WriteDataAsync(
        std::wstring filename,
        const std::vector<byte> fileData
        );
};
