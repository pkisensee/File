///////////////////////////////////////////////////////////////////////////////
//
//  FileSpec.h
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
#include <string>

namespace PKIsensee
{

class FileSpec
{
public:

    typedef std::string::const_pointer  const_pointer;
    typedef std::string::const_iterator const_iterator;
    typedef std::string::size_type      size_type;

public:

    FileSpec() = default;
    explicit FileSpec( const std::string& sFullPath );
    FileSpec( const std::string& vol, const std::string& dir, 
              const std::string& file );
    FileSpec( const std::string& vol, const std::string& dir, 
              const std::string& file, const std::string& ext );

    void Assign( const std::string& );
    void Assign( const FileSpec& );
    void AssignPath( const std::filesystem::path& );
    const_pointer c_str() const;
    const_iterator begin() const;
    const_iterator end() const;
    void Split( std::string& vol, std::string& dir, std::string& file ) const;
    void Split( std::string& vol, std::string& dir, std::string& file, std::string& ext ) const;
    std::string GetVol() const;
    std::string GetDir() const;
    std::string GetFile() const;
    std::string GetFileNoExtension() const;
    std::string GetExtension() const;
    std::string GetFullPath() const;
    std::filesystem::path GetPath() const;
    bool IsFolder() const;
    bool IsFile() const;
    bool Exists() const;
    bool IsNetDrive() const;
    bool IsConnectedNetDrive() const;
	bool IsPrintable() const;
    bool IsExtendedAscii() const;
    
private:

    void Validate();
    void ParseSpec( const_iterator&, const_iterator&, const_iterator&, const_iterator&, 
                    const_iterator&, const_iterator&, const_iterator&, const_iterator& ) const;
    
private:

    std::string mFileSpec; // &&& eventually eliminate this

}; // class FileSpec

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
