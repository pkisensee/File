///////////////////////////////////////////////////////////////////////////////
//
//  File.h
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
#include <filesystem>

namespace PKIsensee
{

///////////////////////////////////////////////////////////////////////////////
//
// Helper classes

enum class FileFlags : uint32_t
{
    Write          = 1 << 0,
    Read           = 1 << 1,
    ShareRead      = 1 << 2,
    ShareWrite     = 1 << 3,
    ShareDelete    = 1 << 4,
    SequentialScan = 1 << 5,
    RandomAccess   = 1 << 6,
};

constexpr FileFlags operator | ( FileFlags lhs, FileFlags rhs )
{
    using T = typename std::underlying_type<FileFlags>::type;
    return static_cast<FileFlags>( static_cast<T>( lhs ) | static_cast<T>( rhs ) );
}

constexpr bool operator & ( FileFlags lhs, FileFlags rhs )
{
    using T = typename std::underlying_type<FileFlags>::type;
    return static_cast<bool>( static_cast<T>( lhs ) & static_cast<T>( rhs ) );
}

///////////////////////////////////////////////////////////////////////////////

class File
{
public:

    struct Times
    {
        // 100-nanosec intervals since 1601/01/01Z
        uint64_t creationTime;
        uint64_t lastAccessTime;
        uint64_t lastWriteTime;
    };

public:

    File();
    explicit File( const std::filesystem::path& );
    ~File();

    // Disable copy/move
    File( const File& ) = delete;
    File& operator=( const File& ) = delete;
    File( File&& ) = delete;
    File& operator=( File&& ) = delete;

    bool Create( FileFlags fileFlags );
    bool Open( FileFlags fileFlags );
    void Close();
    bool IsOpen() const;
    
    size_t GetLength() const;
    bool GetFileTimes( File::Times& ) const;
    bool Read( void*, size_t ) const;
    bool Read( void*, size_t, size_t& ) const;
    bool SetPos( size_t ) const;
    bool Write( const void*, size_t );
    void Flush();
    
    bool Delete( bool bRecycle = true ) const;

    void SetFile( const std::filesystem::path& path ) {
        Close();
        mPath = path;
    }
    std::filesystem::path GetPath() const {
        return mPath;
    }

    ///////////////////////////////////////////////////////////////////////////////
    //
    // Open, read entire file into memory, and close. T must support resize() and
    // data() functions, e.g. string, vector, etc.

    template <class T>
    static bool ReadEntireFile( const std::filesystem::path& path, T& result )
    {
        // Open with least restrictions
        File f( path );
        if( !f.Open( FileFlags::Read | FileFlags::ShareRead | FileFlags::SequentialScan ) )
            return false;

        // Create buffer the size of the file
        auto len = f.GetLength();
        result.resize( len );

        // Read file into memory
        if( !f.Read( result.data(), len ) )
        {
            result.resize( 0 );
            return false;
        }
        return true;
    }

private:

    std::filesystem::path mPath;
    void*                 mFile;

}; // class File

} // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
