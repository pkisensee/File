///////////////////////////////////////////////////////////////////////////////
//
//  DirTree.h
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
#include <string>
#include "FileSpec.h"
#include "FileItr.h"

namespace PKIsensee
{

namespace DirTree
{

    enum class Mode
    {
        NoSubFolders,
        YesSubFolders
    };
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Process each file specified by the spec. Examples:
    //
    // ForEach( ..., "*.cpp", NoSubFolders ) // each CPP file, not deep
    // ForEach( ..., "*", YesSubFolders )    // all files, deep search
    //
    // unaryFn is of the form:
    //
    //    void unaryFn( const FileInfo& ) {};
    //    ForEach( unaryFn, ... );
    //
    // or:
    //
    //    struct unaryFn { void operator()( const FileInfo& ) const {} };
    //    ForEach( unaryFn(), ... );

    template< typename unaryFn >
    static void ForEach( const FileSpec& fs, unaryFn fnForEach, Mode mode )
    {
        // &&& This could be sped up by doing a single pass
    
        // First find all matching files and folders at this level
        for( FileItr i( fs ); i.Exists(); ++i )
            fnForEach( *i );

        if( mode == Mode::NoSubFolders )
            return;
            
        // If we're going deep, dive into folders, too
        std::string vol, dir, file;
        fs.Split( vol, dir, file );
        FileSpec localSpec( FileSpec( vol, dir, "*" ) );
        for( FileItr i( localSpec ); i.Exists(); ++i )
        {
            if( i.GetAttrib().IsFolder() )
            {
                // Recurse into subfolder if requested
                FileSpec subFolder( vol, dir + i->GetFile(), file );
                ForEach( subFolder, fnForEach, mode );
            }
        }            
    }

}; // end namespace DirTree

} // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
