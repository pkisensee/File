///////////////////////////////////////////////////////////////////////////////
//
//  FileAttrib.h
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
#include <cstdint>

// Forward declarations
struct _WIN32_FIND_DATAA;
struct _WIN32_FILE_ATTRIBUTE_DATA;

namespace PKIsensee
{

// Forward declarations
class FileSpec;

class FileAttrib
{
public:

    FileAttrib() = default;
    explicit FileAttrib( const _WIN32_FIND_DATAA& );
    explicit FileAttrib( const _WIN32_FILE_ATTRIBUTE_DATA& );
    explicit FileAttrib( const FileSpec& );

    void Assign( const _WIN32_FIND_DATAA& );
    void Assign( const _WIN32_FILE_ATTRIBUTE_DATA& );
    void Assign( const FileSpec& );
    
    bool IsFolder() const;
    uint64_t GetSize() const;

private:

    uint32_t mFlags=0;
    uint64_t mSize=0;

}; // class FileAttrib

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
