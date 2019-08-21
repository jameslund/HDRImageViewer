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

#include "pch.h"
#include "BasicReaderWriter.h"

using namespace winrt;

using namespace Windows::Storage;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;

using namespace DXRenderer;

BasicReaderWriter::BasicReaderWriter()
{
    m_location = Windows::ApplicationModel::Package::Current().InstalledLocation();
}

BasicReaderWriter::BasicReaderWriter(StorageFolder const& folder)
{
    m_location = folder;
    if (m_location.Path().size() == 0)
    {
        // Applications are not permitted to access certain
        // folders, such as the Documents folder, using this
        // code path.  In such cases, the Path property for
        // the folder will be an empty string.
        winrt::throw_hresult(E_INVALIDARG);
    }
}

std::vector<byte> BasicReaderWriter::ReadData(std::wstring filename)
{
    return ReadDataAsync(filename).get();
}

concurrency::task<std::vector<byte>> BasicReaderWriter::ReadDataAsync(std::wstring filename)
{
    StorageFile file = co_await m_location.GetFileAsync(filename);
    IBuffer buffer = co_await FileIO::ReadBufferAsync(file);

    std::vector<byte> fileData(buffer.Length());
    DataReader::FromBuffer(buffer).ReadBytes(fileData);

    return fileData;
}

void BasicReaderWriter::WriteData(
    std::wstring filename,
    const std::vector<byte> fileData
)
{
    WriteDataAsync(filename, fileData).get();
}

IAsyncAction BasicReaderWriter::WriteDataAsync(
    std::wstring filename,
    const std::vector<byte> fileData
    )
{
    auto file = co_await m_location.CreateFileAsync(filename, CreationCollisionOption::ReplaceExisting);
    co_await FileIO::WriteBytesAsync(file, fileData);
}