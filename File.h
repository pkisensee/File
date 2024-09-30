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
//-----------------------------------------------------------------------------
// 
//  Fast file reading and writing. Typically the implementation is native code
//  for maximum speed and performance.
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
  SharedRead     = 1 << 2,
  SharedWrite    = 1 << 3,
  SharedDelete   = 1 << 4,
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

  using Time = std::chrono::system_clock::rep;
  struct Times
  {
    Time creationTime;
    Time lastAccessTime;
    Time lastWriteTime;
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
  
  uint64_t GetLength() const;
  bool GetFileTimes( File::Times& ) const;
  bool Read( void*, uint32_t ) const;
  bool Read( void*, uint32_t, uint32_t& ) const;
  bool SetPos( uint64_t ) const;
  bool Write( const void*, uint32_t );
  void Flush() const;
  
  bool Delete( bool bRecycle = true ) const;

  void SetFile( const std::filesystem::path& path ) {
    Close();
    path_ = path;
  }

  std::filesystem::path GetPath() const {
    return path_;
  }

  ///////////////////////////////////////////////////////////////////////////////
  //
  // Open, read entire file into memory, and close. T must support resize() and
  // data() functions, e.g. string, vector, etc. Returns false if file can't be
  // opened or insufficient memory. TODO enable concepts

  template <class T>
  static bool ReadEntireFile( const std::filesystem::path& path, T& result )
  {
    // Open with least restrictions
    File f( path );
    if( !f.Open( FileFlags::Read | FileFlags::SharedRead | FileFlags::SequentialScan ) )
      return false;

    // Create buffer the size of the file
    auto len = f.GetLength();
    if( len > std::numeric_limits<uint32_t>::max() )
      return false;
    auto len32 = static_cast<uint32_t>( len );
    try { result.resize( len32 ); }
    catch( std::bad_alloc& )
    {
      return false;
    }

    // Read file into memory
    if( !f.Read( result.data(), len32 ) )
    {
      result.resize( 0 );
      return false;
    }
    return true;
  }

private:

  std::filesystem::path path_;
  void*                 file_;

}; // class File

} // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
