///////////////////////////////////////////////////////////////////////////////
//
//  FileItr.h
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

#pragma once
#include <memory>
#include <string>
#define NOMINMAX 1
#include <Windows.h>
#include "FileSpec.h"
#include "FileAttrib.h"

namespace PKIsensee
{

// Forward declarations
class FileSpec;

class FileItr
{
public:

    explicit FileItr( const FileSpec& );
    ~FileItr();
    
    bool Exists() const;
    bool IsNetworkAvailable() const;
    
    FileItr& operator ++();
    const FileSpec& operator *() const;
    const FileSpec* operator ->() const;
    FileAttrib GetAttrib() const;
    
private:

    FileItr();
    FileItr( const FileItr& );
    FileItr& operator=( const FileItr& );
    
    bool Init( const FileSpec& );
    void Close();
    void SkipSpecialFolders();
    
private:

    std::unique_ptr< WIN32_FIND_DATAA > mpFindData;
    HANDLE      mFindHandle;
    std::string mVol;
    std::string mDir;
    FileSpec    mSpec;
    bool        mNetworkAvail;

};

}

///////////////////////////////////////////////////////////////////////////////
