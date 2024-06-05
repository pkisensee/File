///////////////////////////////////////////////////////////////////////////////
//
//  FileAttrib.cpp
//
//  File attribute information, include file size
//
//  Copyright © Pete Isensee (PKIsensee@msn.com).
//  All rights reserved worldwide.
//
//  Permission to copy, modify, reproduce or redistribute this source code is
//  granted provided the above copyright notice is retained in the resulting 
//  source code.
// 
//  This software is provided "as is" and without any express or implied
//  warranties.
//
///////////////////////////////////////////////////////////////////////////////

#include "FileAttrib.h"

using namespace PKIsensee;

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileAttrib::FileAttrib( const WIN32_FIND_DATAA& findData )
:
    mFlags( findData.dwFileAttributes ),
    mSize( findData.nFileSizeLow + ( uint64_t( findData.nFileSizeHigh ) << 32 ) )
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileAttrib::FileAttrib( const WIN32_FILE_ATTRIBUTE_DATA& fileData )
:
    mFlags( fileData.dwFileAttributes ),
    mSize( fileData.nFileSizeLow + ( uint64_t( fileData.nFileSizeHigh ) << 32 ) )
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileAttrib::FileAttrib( const FileSpec& spec )
{
    Assign( spec );
}

///////////////////////////////////////////////////////////////////////////////
//
// Assign from FIND_DATA

void FileAttrib::Assign( const WIN32_FIND_DATAA& findData )
{
    mFlags = findData.dwFileAttributes;
    mSize = findData.nFileSizeLow + ( uint64_t( findData.nFileSizeHigh ) << 32 );
}

///////////////////////////////////////////////////////////////////////////////
//
// Assign from ATTRIBUTE_DATA

void FileAttrib::Assign( const WIN32_FILE_ATTRIBUTE_DATA& fileData )
{
    mFlags = fileData.dwFileAttributes;
    mSize = fileData.nFileSizeLow + ( uint64_t( fileData.nFileSizeHigh ) << 32 );
}

///////////////////////////////////////////////////////////////////////////////
//
// Assign to new spec

void FileAttrib::Assign( const FileSpec& spec )
{
    // Ignore errors; if the file doesn't exist the attributes will be empty
    WIN32_FILE_ATTRIBUTE_DATA fa = { 0 };
    GetFileAttributesExA( spec.c_str(), GetFileExInfoStandard, &fa );
    Assign( fa );                                 
}

///////////////////////////////////////////////////////////////////////////////
//
// True if file is a folder

bool FileAttrib::IsFolder() const
{
    return( mFlags & FILE_ATTRIBUTE_DIRECTORY ) != 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Size of the file in bytes

uint64_t FileAttrib::GetSize() const
{
    return mSize;
}

///////////////////////////////////////////////////////////////////////////////
