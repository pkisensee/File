///////////////////////////////////////////////////////////////////////////////
//
//  FileSpec.cpp
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
#include "FileSpec.h"
#include "CharUtil.h"
#include "StrUtil.h"
#include <ShlObj.h>
#include <cassert>

using namespace PKIsensee;

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileSpec::FileSpec( const std::string& sFullPath )
: 
    mFileSpec( sFullPath )
{
    Validate();
}

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileSpec::FileSpec( const std::string& vol, const std::string& dir, 
                    const std::string& file )
:
    mFileSpec()
{
    mFileSpec.reserve( vol.length() + dir.length() + file.length() + 1 );
    mFileSpec = vol;
    mFileSpec += dir;
    
    // Append a directory separator if needed
    if( !dir.empty() && *dir.rbegin() != '\\' && *dir.rbegin() != '/' )
        mFileSpec += '\\';
    
    mFileSpec += file;
    Validate();                    
}                    

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileSpec::FileSpec( const std::string& vol, const std::string& dir, 
                    const std::string& file, const std::string& ext )
:
    mFileSpec()
{
    mFileSpec.reserve( vol.length() + dir.length() + file.length() + ext.length() + 2 );
    mFileSpec = vol;
    mFileSpec += dir;
    
    // Append a directory separator if needed
    if( !dir.empty() && *dir.rbegin() != '\\' && *dir.rbegin() != '/' )
        mFileSpec += '\\';
    
    mFileSpec += file;
    
    // Append a extension separator if needed
    if( !ext.empty() && *ext.rbegin() != '.' )
        mFileSpec += '.';

    mFileSpec += ext;
    
    Validate();
}

///////////////////////////////////////////////////////////////////////////////
//
// Assign to new spec

void FileSpec::Assign( const std::string& sFullPath )
{
    mFileSpec.assign( sFullPath );
    Validate();
}

void FileSpec::Assign( const FileSpec& spec )
{
    Assign( spec.mFileSpec );
}

void FileSpec::AssignPath( const std::filesystem::path& path )
{
    Assign( path.string() );
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract string pointer

std::string::const_pointer FileSpec::c_str() const
{
    return mFileSpec.c_str();
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract iterators

FileSpec::const_iterator FileSpec::begin() const
{
    return mFileSpec.begin();
}

FileSpec::const_iterator FileSpec::end() const
{
    return mFileSpec.end();
}

///////////////////////////////////////////////////////////////////////////////
//
// Split into component parts. File includes extension

void FileSpec::Split( std::string& vol, std::string& dir, 
                      std::string& file ) const
{
    // root_path == root_name / root_directory
    // path == root_name / root_directory / relative_path
    // path == root_path / relative_path
    // path == parent_path / filename
    // filename == stem + extension

    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    vol.assign( volBegin, volEnd );
    dir.assign( dirBegin, dirEnd );
    file.assign( fileBegin, extEnd );
}                      

///////////////////////////////////////////////////////////////////////////////
//
// Split into component parts including extension

void FileSpec::Split( std::string& vol, std::string& dir, 
                      std::string& file, std::string& ext ) const
{
    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    vol.assign( volBegin, volEnd );
    dir.assign( dirBegin, dirEnd );
    file.assign( fileBegin, fileEnd );
    ext.assign( extBegin, extEnd );
}                      

///////////////////////////////////////////////////////////////////////////////
//
// Extract volume component

std::string FileSpec::GetVol() const
{
    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    return std::string( volBegin, volEnd );
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract directory component

std::string FileSpec::GetDir() const
{
    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    return std::string( dirBegin, dirEnd );
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract file plus extension component

std::string FileSpec::GetFile() const
{
    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    return std::string( fileBegin, extEnd );
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract file not including extension component

std::string FileSpec::GetFileNoExtension() const
{
    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    return std::string( fileBegin, fileEnd );
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract extension component. Does not include separator character.

std::string FileSpec::GetExtension() const
{
    // Locate parse points
    const_iterator volBegin, volEnd;
    const_iterator dirBegin, dirEnd;
    const_iterator fileBegin, fileEnd;
    const_iterator extBegin, extEnd;
    ParseSpec( volBegin, volEnd, dirBegin, dirEnd, fileBegin, fileEnd, extBegin, extEnd );
    
    // Build results
    return std::string( extBegin, extEnd );
}

///////////////////////////////////////////////////////////////////////////////
//
// Return full path

std::string FileSpec::GetFullPath() const
{
    return mFileSpec;
}

std::filesystem::path FileSpec::GetPath() const
{
    return mFileSpec;
}

///////////////////////////////////////////////////////////////////////////////
//
// True if spec represents a folder

bool FileSpec::IsFolder() const
{
    std::string vol;
    std::string dir;
    std::string file;
    Split( vol, dir, file );

    // True if no file portion, but either a valid volume or valid dir
    if( !file.empty() )
        return false;
    return( !vol.empty() || !dir.empty() );
}

///////////////////////////////////////////////////////////////////////////////
//
// True if spec represents a file

bool FileSpec::IsFile() const
{
    std::string vol;
    std::string dir;
    std::string file;
    Split( vol, dir, file );

    // True if a file portion
    return( !file.empty() );
}

///////////////////////////////////////////////////////////////////////////////
//
// True if the file/folder exists

bool FileSpec::Exists() const
{
    // FindFirstFile doesn't handle trailing backslashes. Nuke them.
    FileSpec fs( mFileSpec );
    std::string path = fs.GetFullPath();
    if( !path.empty() && ( path.find_last_of( '\\' ) == path.length() - 1 ) )
        path.resize( path.length() - 1 );

    WIN32_FIND_DATAA findData;
    HANDLE findHandle = ::FindFirstFileExA( path.c_str(), FindExInfoBasic, &findData,
                                            FindExSearchNameMatch, NULL, 0 );
    if( findHandle == INVALID_HANDLE_VALUE )
        return false;
    ::FindClose( findHandle );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// True if the volume specifies a networked drive

bool FileSpec::IsNetDrive() const
{
    std::string vol( GetVol() );
    if( vol.empty() )
        return false;
        
    char drivePath[4] = { vol[0], ':', '\\', 0 };        
    return( GetDriveType( drivePath ) == DRIVE_REMOTE );
}

///////////////////////////////////////////////////////////////////////////////
//
// True if the volume specifies a networked drive that is also online

bool FileSpec::IsConnectedNetDrive() const
{
    // A three-step process is required:
    //
    // 1) Does the spec represent a network drive
    // 2) Does the drive have a device mapping?
    // 3) Is the mapped network device available?

    // Determine if the spec represents a network drive
    if( !IsNetDrive() )
        return false;
    
    // Determine if there is a mapping to a device name
    #pragma comment( lib, "mpr" )
    std::string vol( GetVol() );
    const DWORD kDevChars = 1024;
    char devName[ kDevChars ];
    DWORD devChars = kDevChars;
    uint32_t result = WNetGetConnectionA( vol.c_str(), devName, &devChars );
    switch( result )
    {
        case ERROR_NO_NETWORK: // network unavailable
        case ERROR_NOT_CONNECTED: // not a redirected device
        case ERROR_CONNECTION_UNAVAIL: // connection not found
            return false; 

        // unexpected errors
        case ERROR_BAD_DEVICE:
        case ERROR_MORE_DATA:
        case ERROR_EXTENDED_ERROR:
        case ERROR_NO_NET_OR_BAD_PATH:
        default: 
            assert( false ); 
            return false;
            
        case NO_ERROR: 
            break;
    }
    
    // Determine if the device is truly available
    FileSpec wildVol( vol, "\\", "*.*" );
    WIN32_FIND_DATAA findData;
    HANDLE findHandle = ::FindFirstFileExA( wildVol.c_str(), FindExInfoBasic, &findData,
                                            FindExSearchNameMatch, NULL, 0 );
    if( findHandle == INVALID_HANDLE_VALUE && ::GetLastError() == ERROR_BAD_NETPATH )
        return false;
    ::FindClose( findHandle );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// True if the full path contains all valid printable ASCII characters
// Some programs will not access file names that don't have printable chars

bool FileSpec::IsPrintable() const
{
    return StrUtilT< std::string::value_type >::IsPrintable( mFileSpec );
}

///////////////////////////////////////////////////////////////////////////////
//
// True if the full path contains any extended ASCII characters
// Some programs will not access file names that don't have standard ASCII chars

bool FileSpec::IsExtendedAscii() const
{
    return StrUtilT< std::string::value_type >::IsExtendedAscii( mFileSpec );
}

///////////////////////////////////////////////////////////////////////////////
//
// Validate file string to make sure it meets system requirements

void FileSpec::Validate() // private
{
    std::string vol;
    std::string dir;
    std::string file;
    Split( vol, dir, file );
    
    // Check volume name
    // Should be empty or single alpha character followed by ':'
    if( !vol.empty() )
    {
        assert( vol.length() == 2 );
        assert( CharUtilT< char >::IsAlpha( vol[0] ) );
        assert( vol[1] == ':' );
    }
    
    // Check directory
    // Should not have any invalid characters
    assert( StrUtilT< char >::IsGoodFileName( dir, StrUtilT< char >::AllowWildcards::Yes ) );
    
    // Check file
    // Should not have any invalid characters
    assert( StrUtilT< char >::IsGoodFileName( file, StrUtilT< char >::AllowWildcards::Yes ) );
    
    // Rebuild the spec
    std::string mCompare( vol );
    mCompare += dir;
    mCompare += file;
    
    // Compare the two; should be an exact match
    assert( mCompare == mFileSpec );
}

///////////////////////////////////////////////////////////////////////////////
//
// Parse for key breakpoints in spec
//
// c:\dir\file.ext
// ^ ^    ^   ^^  ^
// | |    |   ||  |
// | |    |   ||  end of ext
// | |    |   |begin of ext
// | |    |   end of file
// | |    begin of file
// | |    end of dir
// | begin of dir
// | end of vol 
// begin of vol
//
// Examples:
//
// mSpec                 vol   dir         file    ext
// --------------------------------------------------------
// "a:"                  a:
// "a:file"              a:                file
// "a:\file"             a:    \           file
// "\dir\"                     \dir\
// "\file"                     \           file
// "..\..\dir\"                ..\..\dir\
// "..\..\file"                ..\..\      file
// "a:\dir\file.ext"     a:    \dir\       file    ext
// "\dir\file.ext"             \dir\       file    ext
// "dir\file.ext"              dir\        file    ext
// "dir.ext\file.ext"          dir.ext\    file    ext
// "dir.ext\file"              dir.ext\    file
// "file.ex"                               file    ex
// "file.ex.longext"                       file.ex longext
// "..\"                       ..\
// ""

void FileSpec::ParseSpec( const_iterator& volBegin, const_iterator& volEnd, 
                          const_iterator& dirBegin, const_iterator& dirEnd,
                          const_iterator& fileBegin, const_iterator& fileEnd,
                          const_iterator& extBegin, const_iterator& extEnd ) const // private
{
    // Find the key items:
    // * First volume separator (colon)
    // * Last directory separator (backslash)
    // * Last file separator (period)
    size_type firstVolSep = ( mFileSpec.length() > 1 && mFileSpec[1] == ':' ) ?
                            1 : std::string::npos;
    size_type lastDirSep  = mFileSpec.rfind( '\\' );
    size_type lastFileSep = mFileSpec.rfind( '.' );
    
    // Determine if we have separators or not
    bool bHaveVolSep = firstVolSep != std::string::npos;
    bool bHaveDirSep = lastDirSep != std::string::npos;
    bool bHaveFileSep = ( lastFileSep != std::string::npos ) &&
                        ( bHaveDirSep ? ( lastFileSep > lastDirSep ) : true );
                       
    // The volume is everything up to and including the first colon
    volBegin = mFileSpec.begin();
    volEnd = mFileSpec.begin() + ( bHaveVolSep ? firstVolSep+1 : 0 );
    
    // The dir is everything between the first colon and last backslash
    dirBegin = volEnd;
    dirEnd = bHaveDirSep ? mFileSpec.begin()+lastDirSep+1 : volEnd;
    
    // The file is everything between the last backslash and the file
    // separator (period), not including the file separator
    fileBegin = dirEnd;
    fileEnd = bHaveFileSep ? mFileSpec.begin()+lastFileSep : mFileSpec.end();
    
    // The extension is everything between the last file separator and the end
    extBegin = bHaveFileSep ? mFileSpec.begin()+lastFileSep+1 : mFileSpec.end();
    extEnd = mFileSpec.end();
}

///////////////////////////////////////////////////////////////////////////////
