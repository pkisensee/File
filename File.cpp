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
//  *** Microsoft Windows implementation ***
//
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <limits>
#include "File.h"
#include "Log.h"
#include "StrUtil.h"

// Windows headers
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

    if( flags & FileFlags::Read )       access |= GENERIC_READ;
    if( flags & FileFlags::Write )      access |= GENERIC_WRITE;

    if( flags & FileFlags::SharedRead )    share |= FILE_SHARE_READ;
    if( flags & FileFlags::SharedWrite )   share |= FILE_SHARE_WRITE;
    if( flags & FileFlags::SharedDelete )  share |= FILE_SHARE_DELETE;

    if( flags & FileFlags::SequentialScan ) attribs |= FILE_FLAG_SEQUENTIAL_SCAN;
    if( flags & FileFlags::RandomAccess )   attribs |= FILE_FLAG_RANDOM_ACCESS;
  }
};

File::Time FiletimeToStdTime( const FILETIME& ft )
{
  // complex code likely reduces to single instruction
  using FileTime = std::filesystem::file_time_type;
  ULARGE_INTEGER large = { { ft.dwLowDateTime, ft.dwHighDateTime } };
  auto duration = FileTime::duration{ large.QuadPart };
  FileTime fileTime{ duration };
  File::Time result = fileTime.time_since_epoch().count();
  return result;
}

} // end anonymous namespace

///////////////////////////////////////////////////////////////////////////////
//
// Ctors

File::File() :
  path_(),
  file_( INVALID_HANDLE_VALUE )
{
}

File::File( const fs::path& path ) :
  path_( path ),
  file_( INVALID_HANDLE_VALUE )
{
  path_.make_preferred(); // ensure Windows separators
}

///////////////////////////////////////////////////////////////////////////////
//
// Dtor

File::~File()
{
  if( file_ != INVALID_HANDLE_VALUE )
    ::CloseHandle( file_ );
}

///////////////////////////////////////////////////////////////////////////////
//
// Create the file or directory, including any intermediate directories.
// Closes any currently open file or folder.

bool File::Create( FileFlags flags )
{
  CreateFileParams cfp( CREATE_ALWAYS, flags );
  assert( path_.has_filename() || path_.has_parent_path() );
  Close();
  if( path_.has_parent_path() )
  {
    fs::create_directories( path_.parent_path() );
    if( !path_.has_filename() )
    {
      // If directory specified, adjust flags
      cfp.attribs |= FILE_FLAG_BACKUP_SEMANTICS;
      cfp.create = OPEN_EXISTING;
    }
  }

  file_ = ::CreateFileW( path_.c_str(), cfp.access, cfp.share, NULL, cfp.create, cfp.attribs, NULL );
  if( file_ == INVALID_HANDLE_VALUE )
    PKLOG_WARN( "CreateFileW failed to create file %S with error %d\n", 
                path_.c_str(), ::GetLastError() );
  return IsOpen();
}

///////////////////////////////////////////////////////////////////////////////
//
// Open the file or directory. Closes any currently open file.

bool File::Open( FileFlags flags )
{
  CreateFileParams cfp( OPEN_EXISTING, flags );
  assert( path_.has_filename() || path_.has_parent_path() );

  // Required to open a directory
  if( !path_.has_filename() )
    cfp.attribs |= FILE_FLAG_BACKUP_SEMANTICS;

  Close();
  file_ = ::CreateFileW( path_.c_str(), cfp.access, cfp.share, NULL, cfp.create, cfp.attribs, NULL );
  if( file_ == INVALID_HANDLE_VALUE )
    PKLOG_WARN( "CreateFileW failed to open file %S with error %d\n", 
                path_.c_str(), ::GetLastError() );
  return IsOpen();
}

void File::Close()
{
  if( file_ != INVALID_HANDLE_VALUE )
  {
    ::CloseHandle( file_ );
    file_ = INVALID_HANDLE_VALUE;
  }    
}

bool File::IsOpen() const
{
  return( file_ != INVALID_HANDLE_VALUE );
}

///////////////////////////////////////////////////////////////////////////////
//
// Size of file/folder in bytes

uint64_t File::GetLength() const
{
  if( !IsOpen() )
  {
    WIN32_FILE_ATTRIBUTE_DATA fa = { 0 };
    ::GetFileAttributesExW( path_.c_str(), GetFileExInfoStandard, &fa );
    uint64_t lo = fa.nFileSizeLow;
    uint64_t hi = fa.nFileSizeHigh;
    return lo + ( hi << 32 );
  }
  FILE_STANDARD_INFO FileInfo = { { { 0 } } };
  [[maybe_unused]] auto success = ::GetFileInformationByHandleEx( file_, 
                                      FileStandardInfo, &FileInfo, sizeof(FileInfo) );
  assert( success );
  assert( FileInfo.EndOfFile.QuadPart >= 0 );
  uint64_t len = static_cast<uint64_t>( FileInfo.EndOfFile.QuadPart );
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
    auto hFile = ::FindFirstFileW( path_.c_str(), &fd );
    assert( hFile != INVALID_HANDLE_VALUE );
    if( hFile == INVALID_HANDLE_VALUE )
      return false;

    ::FindClose( hFile );
    fileTimes.creationTime   = FiletimeToStdTime( fd.ftCreationTime );
    fileTimes.lastAccessTime = FiletimeToStdTime( fd.ftLastAccessTime );
    fileTimes.lastWriteTime  = FiletimeToStdTime( fd.ftLastWriteTime );
    return true;
  }

  FILETIME creationTime, lastAccessTime, lastWriteTime;
  [[maybe_unused]] auto success = ::GetFileTime( file_, 
                                     &creationTime, &lastAccessTime, &lastWriteTime );
  assert( success );
  fileTimes.creationTime   = FiletimeToStdTime( creationTime );
  fileTimes.lastAccessTime = FiletimeToStdTime( lastAccessTime );
  fileTimes.lastWriteTime  = FiletimeToStdTime( lastWriteTime );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the next position for reading or writing. Position is always from the
// beginning of the file.

bool File::SetPos( uint64_t pos ) const
{
  assert( IsOpen() );

  // With FILE_BEGIN, largePos is interpreted as unsigned, so cast is safe
  LARGE_INTEGER largePos = { .QuadPart = static_cast<LONGLONG>(pos) };
  auto success = ::SetFilePointerEx( file_, largePos, NULL, FILE_BEGIN );
  return success == TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Read a buffer from the current file position

bool File::Read( void* pBuffer, uint32_t bytes ) const
{
  uint32_t bytesRead = 0;
  auto success = Read( pBuffer, bytes, bytesRead );
  return ( success && ( bytes == bytesRead ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// Read a buffer from the current file position; returns bytesRead

bool File::Read( void* pBuffer, uint32_t bytes, uint32_t& bytesRead ) const
{
  assert( path_.has_filename() );
  assert( IsOpen() );
  assert( pBuffer != nullptr );
  assert( bytes > 0 );

  // If this fails, adjust the function to be a loop
  assert( bytes <= std::numeric_limits<DWORD>::max() );
  DWORD bytes32 = static_cast<DWORD>( bytes );

  DWORD br = 0u;
  auto success = ::ReadFile( file_, pBuffer, bytes32, &br, NULL );
  bytesRead = br;
  return( success == TRUE );
}

///////////////////////////////////////////////////////////////////////////////
//
// Write a buffer to the current file position

bool File::Write( const void* pBuffer, uint32_t bytes )
{
  assert( path_.has_filename() );
  assert( IsOpen() );
  assert( pBuffer != nullptr );
  assert( bytes <= std::numeric_limits<DWORD>::max() );
  DWORD bytes32 = static_cast<DWORD>( bytes );
  DWORD bytesWritten = 0;
  auto success = ::WriteFile( file_, pBuffer, bytes32, &bytesWritten, NULL );
  return( success && ( bytes == bytesWritten ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// Flush file to storage medium. Does nothing for directories or read-only files

void File::Flush() const
{
  assert( path_.has_filename() );
  assert( IsOpen() );
  [[maybe_unused]] BOOL success = ::FlushFileBuffers( file_ );
  assert( success );
}

///////////////////////////////////////////////////////////////////////////////
//
// Delete the file or directory. Requires that the file is closed and not
// read-only. Allows the file to be moved to the recycle bin, if one exists.

bool File::Delete( bool recycle ) const
{
  assert( !IsOpen() );
  if( IsOpen() )
    return false;

  SHFILEOPSTRUCTW FileOp = { 0 };
  FileOp.wFunc = FO_DELETE;
  FileOp.fFlags = static_cast<FILEOP_FLAGS>( recycle ? FOF_ALLOWUNDO : 0x0 );
  FileOp.fFlags |= FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

  // Path must be fully qualfied, hence fs::absolute.
  // Directories must have any trailing slashes removed.
  // String is required to end in two null characters (!)
  std::wstring fullPath = fs::absolute(path_).wstring();
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
