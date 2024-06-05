///////////////////////////////////////////////////////////////////////////////
//
//  File.cpp
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

// Includes
#include <cassert>
#include <limits>
#include "File.h"
#include "Log.h"
#include "StrUtil.h"

#define NOMINMAX 1
#include "Windows.h"
#include "ShellAPI.h"
#include "ShlObj.h"

using namespace PKIsensee;
namespace fs = std::filesystem;

namespace { // anonymous namespace

struct CreateFileParams
{
    uint32_t access = 0u;
    uint32_t share = 0u;
    uint32_t create = 0u;
    uint32_t attribs = FILE_ATTRIBUTE_NORMAL;

    CreateFileParams( uint32_t howToOpen, FileFlags flags )
      : create( howToOpen )
    {
        // Can't specify both sequential and random
        assert( !( ( flags & FileFlags::SequentialScan ) && ( flags & FileFlags::RandomAccess ) ) );

        // Need to open for either reading and/or writing
        assert( ( flags & FileFlags::Read ) || ( flags & FileFlags::Write ) );

        if( flags & FileFlags::Read )           access |= GENERIC_READ;
        if( flags & FileFlags::Write )          access |= GENERIC_WRITE;

        if( flags & FileFlags::ShareRead )      share |= FILE_SHARE_READ;
        if( flags & FileFlags::ShareWrite )     share |= FILE_SHARE_WRITE;
        if( flags & FileFlags::ShareDelete )    share |= FILE_SHARE_DELETE;

        if( flags & FileFlags::SequentialScan ) attribs |= FILE_FLAG_SEQUENTIAL_SCAN;
        if( flags & FileFlags::RandomAccess )   attribs |= FILE_FLAG_RANDOM_ACCESS;
    }
};

uint64_t FiletimeToU64( const FILETIME& ft )
{
    ULARGE_INTEGER large = { ft.dwLowDateTime, ft.dwHighDateTime };
    return large.QuadPart;
}

} // end anonymous namespace

///////////////////////////////////////////////////////////////////////////////
//
// Ctors

File::File() :
    mPath(),
    mFile( INVALID_HANDLE_VALUE )
{
}

File::File( const fs::path& path ) :
    mPath( path ),
    mFile( INVALID_HANDLE_VALUE )
{
    mPath.make_preferred(); // ensure Windows separators
}

///////////////////////////////////////////////////////////////////////////////
//
// Dtor

File::~File()
{
    if( mFile != INVALID_HANDLE_VALUE )
        ::CloseHandle( mFile );
}

///////////////////////////////////////////////////////////////////////////////
//
// Create the file or directory, including any intermediate directories.
// Closes any currently open file or folder

bool File::Create( FileFlags flags )
{
    CreateFileParams cfp( CREATE_ALWAYS, flags );
    assert( mPath.has_filename() || mPath.has_parent_path() );
    Close();
    if( mPath.has_parent_path() )
    {
        fs::create_directories( mPath.parent_path() );
        if( !mPath.has_filename() )
        {
            // If a directory was specified, we need to adjust some flags
            cfp.attribs |= FILE_FLAG_BACKUP_SEMANTICS;
            cfp.create = OPEN_EXISTING;
        }
    }

    mFile = ::CreateFileW( mPath.c_str(), cfp.access, cfp.share, NULL, cfp.create, cfp.attribs, NULL );
    if( mFile == INVALID_HANDLE_VALUE )
        PKLOG_WARN( "CreateFileW failed to create file %S with error %d\n", mPath.c_str(), ::GetLastError() );
    return IsOpen();
}

///////////////////////////////////////////////////////////////////////////////
//
// Open the file or directory
// Closes any currently open file

bool File::Open( FileFlags flags )
{
    CreateFileParams cfp( OPEN_EXISTING, flags );
    assert( mPath.has_filename() || mPath.has_parent_path() );

    // Required to open a directory
    if( !mPath.has_filename() )
        cfp.attribs |= FILE_FLAG_BACKUP_SEMANTICS;

    Close();
    mFile = ::CreateFileW( mPath.c_str(), cfp.access, cfp.share, NULL, cfp.create, cfp.attribs, NULL );
    if( mFile == INVALID_HANDLE_VALUE )
        PKLOG_WARN( "CreateFileW failed to open file %S with error %d\n", mPath.c_str(), ::GetLastError() );
    return IsOpen();
}

///////////////////////////////////////////////////////////////////////////////
//
// Close the file or directory

void File::Close()
{
    if( mFile != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( mFile );
        mFile = INVALID_HANDLE_VALUE;
    }        
}

///////////////////////////////////////////////////////////////////////////////
//
// Report if file is open

bool File::IsOpen() const
{
    return( mFile != INVALID_HANDLE_VALUE );
}

///////////////////////////////////////////////////////////////////////////////
//
// The size of the file or folder in bytes

size_t File::GetLength() const
{
    if( !IsOpen() )
    {
        WIN32_FILE_ATTRIBUTE_DATA fa = { 0 };
        ::GetFileAttributesExW( mPath.c_str(), GetFileExInfoStandard, &fa );
        return fa.nFileSizeLow + ( uint64_t( fa.nFileSizeHigh ) << 32 );
    }
    FILE_STANDARD_INFO FileInfo = { 0 };
    BOOL bSuccess = ::GetFileInformationByHandleEx( mFile, FileStandardInfo, 
                                                    &FileInfo, sizeof(FileInfo) );
    assert( bSuccess );
    assert( FileInfo.EndOfFile.QuadPart >= 0 );
    size_t len = static_cast<size_t>( FileInfo.EndOfFile.QuadPart );
    assert( len <= std::numeric_limits<size_t>::max() );
    return len;
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract time that file/dir was created, accessed, modified

bool File::GetFileTimes( File::Times& fileTimes ) const
{
    if( !IsOpen() )
    {
        WIN32_FIND_DATAW fd = { 0 };
        HANDLE hFile = ::FindFirstFileW( mPath.c_str(), &fd );
        assert( hFile != INVALID_HANDLE_VALUE );
        if( hFile == INVALID_HANDLE_VALUE )
            return false;
        fileTimes.creationTime   = FiletimeToU64( fd.ftCreationTime );
        fileTimes.lastAccessTime = FiletimeToU64( fd.ftLastAccessTime );
        fileTimes.lastWriteTime  = FiletimeToU64( fd.ftLastWriteTime );
        FindClose( hFile );
        return true;
    }

    FILETIME creationTime, lastAccessTime, lastWriteTime;
    BOOL bSuccess = ::GetFileTime( mFile, &creationTime, &lastAccessTime, &lastWriteTime );
    assert( bSuccess );
    fileTimes.creationTime   = FiletimeToU64( creationTime );
    fileTimes.lastAccessTime = FiletimeToU64( lastAccessTime );
    fileTimes.lastWriteTime  = FiletimeToU64( lastWriteTime );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the next position for reading or writing

bool File::SetPos( size_t pos ) const
{
    assert( IsOpen() );
    assert( pos <= static_cast<size_t>(std::numeric_limits<long long>::max()) );
    LARGE_INTEGER largePos = { .QuadPart = static_cast<long long>(pos) };

    auto success = ::SetFilePointerEx( mFile, largePos, NULL, FILE_BEGIN );
    return success;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read a buffer from the current file position

bool File::Read( void* pBuffer, size_t bytes ) const
{
    size_t bytesRead;
    bool success = Read( pBuffer, bytes, bytesRead );
    return ( success && ( bytes == bytesRead ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// Read a buffer from the current file position; return bytes read

bool File::Read( void* pBuffer, size_t bytes, size_t& bytesRead ) const
{
    assert( mPath.has_filename() );
    assert( IsOpen() );
    assert( pBuffer != nullptr );
    assert( bytes > 0 );

    // If this fails, we need to adjust the function to be a loop
    assert( bytes <= std::numeric_limits<DWORD>::max() );
    DWORD bytes32 = static_cast<DWORD>( bytes );

    DWORD br = 0u;
    BOOL bSuccess = ::ReadFile( mFile, pBuffer, bytes32, &br, NULL );
    bytesRead = br;
    return( bSuccess == TRUE );
}

///////////////////////////////////////////////////////////////////////////////
//
// Write a buffer to the current file position

bool File::Write( const void* pBuffer, size_t bytes )
{
    assert( mPath.has_filename() );
    assert( IsOpen() );
    assert( pBuffer != nullptr );
    assert( bytes > 0 );
    assert( bytes <= std::numeric_limits<DWORD>::max() );
    DWORD bytes32 = static_cast<DWORD>( bytes );
    DWORD bytesWritten = 0;
    BOOL bSuccess = ::WriteFile( mFile, pBuffer, bytes32, &bytesWritten, NULL );
    return( ( bSuccess == TRUE ) && ( bytes == bytesWritten ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// Flush file to disk. Does nothing for directories or files opened only for reading

void File::Flush()
{
    assert( mPath.has_filename() );
    assert( IsOpen() );
    BOOL bSuccess = ::FlushFileBuffers( mFile );
    assert( bSuccess );
    (void)bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
//
// Delete the file or directory. Requires that the file is closed and not
// read-only. Allows the file to be moved to the recycle bin, if one exists.

bool File::Delete( bool bRecycle ) const
{
    assert( !IsOpen() );
    if( IsOpen() )
        return false;

    SHFILEOPSTRUCTW FileOp = { 0 };
    FileOp.wFunc = FO_DELETE;
    FileOp.fFlags = ( bRecycle ? FOF_ALLOWUNDO : 0 ) | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    // Path must be fully qualfied, hence fs::absolute
    // Directories must have any trailing slashes removed
    // String is required to end in two null characters
    std::wstring fullPath = fs::absolute(mPath).wstring();
    StrUtilT<wchar_t>::ToTrimmedTrailing( fullPath, L"\\/" );
    fullPath += L'\0';
    FileOp.pFrom = fullPath.c_str();
        
    auto result = ::SHFileOperationW( &FileOp );
    return( result == 0 );
}

///////////////////////////////////////////////////////////////////////////////
//
// Rename the file or directory
// Prefer fs::rename

///////////////////////////////////////////////////////////////////////////////
//
// Determine if file or directory exists
// Prefer fs::exists

///////////////////////////////////////////////////////////////////////////////
