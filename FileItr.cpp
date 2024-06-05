///////////////////////////////////////////////////////////////////////////////
//
//  FileItr.cpp
//
//  Iterate over files and folders in a given folder.
//
//  Example
//
//      // Iterate over all CPP files in current folder
//      for( FileItr i( FileSpec( "*.cpp" ) ); i.Exists(); ++i )
//          printf( "%s", i->Spec.c_str() );
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

#include "FileItr.h"
#include "FileSpec.h"
#include <cassert>

#ifdef _DEBUG
#define verify(e)   assert(e)
#else
#define verify(e)   static_cast<void>(e)
#endif

using namespace PKIsensee;

///////////////////////////////////////////////////////////////////////////////
//
// Ctor

FileItr::FileItr( const FileSpec& spec )
:
    mpFindData(),
    mFindHandle( INVALID_HANDLE_VALUE ),
    mVol(),
    mDir(),
    mSpec(),
    mNetworkAvail( true )
{
    Init( spec );
}

///////////////////////////////////////////////////////////////////////////////
//
// Dtor

FileItr::~FileItr()
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
//
// True if iterator points to a valid file

bool FileItr::Exists() const
{
    return( mFindHandle != INVALID_HANDLE_VALUE );
}

///////////////////////////////////////////////////////////////////////////////
//
// True if iterator determined that network was available. Useful to determine
// whether a particular FileSpec was on a share that was not available.

bool FileItr::IsNetworkAvailable() const
{
    return mNetworkAvail;
}

///////////////////////////////////////////////////////////////////////////////
//
// Increment to next file in the search.

FileItr& FileItr::operator ++()
{
    assert( Exists() );
    assert( mpFindData.get() != NULL );
    
    // Iterate to the next file. If there are no more files, close the handle
    if( !::FindNextFileA( mFindHandle, mpFindData.get() ) )
    {
        Close();
        return *this;
    }
        
    SkipSpecialFolders();

    // Store the current mpFindData into the FileInfo object
    mSpec.Assign( FileSpec( mVol, mDir, std::string( mpFindData->cFileName ) ) );
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
//
// Return the information about the current file

const FileSpec& FileItr::operator *() const
{
    assert( Exists() );
    assert( mpFindData.get() );
    return mSpec;
}

///////////////////////////////////////////////////////////////////////////////
//
// Return the information about the current file

const FileSpec* FileItr::operator ->() const
{
    assert( Exists() );
    assert( mpFindData.get() );
    return &mSpec;
}

///////////////////////////////////////////////////////////////////////////////
//
// Extract current file attributes.
// This is a faster than using FileAttrib fa( *i )

FileAttrib FileItr::GetAttrib() const
{
    assert( Exists() );
    assert( mpFindData.get() != NULL );
    return FileAttrib( *mpFindData.get() );
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialize from a wildcard or directory specification

bool FileItr::Init( const FileSpec& spec ) // private
{
    // Reinitialize if necessary
    mpFindData.reset( new WIN32_FIND_DATAA );
    Close();
    
    // FindFirstFile doesn't handle trailing backslashes. Nuke them.
    FileSpec fs( spec );
    std::string path = fs.GetFullPath();
    if( !path.empty() && ( path.find_last_of( '\\' ) == path.length() - 1 ) )
    {
        path.resize( path.length() - 1 );
        fs.Assign( path );
    }
    
    // Prepare for iteration
    mFindHandle = ::FindFirstFileExA( path.c_str(), FindExInfoBasic, mpFindData.get(), 
                                      FindExSearchNameMatch, NULL, 0 );
    if( mFindHandle == INVALID_HANDLE_VALUE )
    {
        ZeroMemory( mpFindData.get(), sizeof( WIN32_FIND_DATAA ) );
        if( GetLastError() == ERROR_BAD_NETPATH )
            mNetworkAvail = false;
        return false;
    }        
    SkipSpecialFolders();
        
    // Save the path info so we can use it later
    std::string sFile;
    fs.Split( mVol, mDir, sFile );

    // Store the current mpFindData into the FileInfo object
    mSpec.Assign( FileSpec( mVol, mDir, std::string( mpFindData->cFileName ) ) );
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// End the iteration

void FileItr::Close() // private
{
    if( mFindHandle != INVALID_HANDLE_VALUE )
    {
        verify( FindClose( mFindHandle ) != FALSE );
        mFindHandle = INVALID_HANDLE_VALUE;
        mNetworkAvail = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Silently skip . and ..

void FileItr::SkipSpecialFolders() // private
{
    assert( Exists() );
    assert( mpFindData.get() );
    
    if( !GetAttrib().IsFolder() )
        return;
        
    if( mpFindData->cFileName[0] == '.' )
    {
        if( mpFindData->cFileName[1] == '\0' )
            operator ++();
        else if( mpFindData->cFileName[1] == '.' &&
                    mpFindData->cFileName[2] == '\0' )
            operator ++();
    }
}

///////////////////////////////////////////////////////////////////////////////
