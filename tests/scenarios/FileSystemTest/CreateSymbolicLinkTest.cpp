﻿//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <functional>

#include <test_config.h>

#include "common_paths.h"

static int DoCreateSymbolicLinkTest(
    const std::filesystem::path& linkPath,
    const std::filesystem::path& targetPath,
    const char* expectedInitialContents)
{
    trace_messages(L"Creating a symbolic-link from: ", info_color, linkPath.native(), new_line);
    trace_messages(L"                           to: ", info_color, targetPath.native(), new_line);

    // NOTE: Call CreateSymbolicLink _before_ reading file contents as reading the file will copy it to the redirected
    //       path, if it is a file that exists in the package
    if (!::CreateSymbolicLinkW(linkPath.c_str(), targetPath.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        return trace_last_error(L"Failed to create the symbolic-link");
    }

    // Contents of the link file should match that of the target
    auto contents = read_entire_file(targetPath.c_str());
    trace_messages(L"Initial target file contents: ", info_color, contents.c_str(), new_line);
    if (contents != expectedInitialContents)
    {
        trace_message(L"ERROR: Initial file contents does not match the expected value!\n", error_color);
        trace_messages(error_color, L"ERROR: Expected contents: ", error_info_color, expectedInitialContents, new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    if (auto linkContents = read_entire_file(linkPath.c_str()); linkContents != contents)
    {
        trace_message(L"ERROR: Contents of the symbolic-link do not match that of its target!\n", error_color);
        trace_messages(error_color, L"ERROR: Contents of the symbolic-link: ", error_info_color, linkContents.c_str(), new_line);
        return ERROR_ASSERTION_FAILURE;
    }

    // Writing to the symbolic-link should replace the contents of the target file
    const char* newFileContents = "You are reading the contents written to the symbolic-link";
    trace_messages(L"Writing to the symbolic-link: ", info_color, newFileContents, new_line);
    if (!write_entire_file(linkPath.c_str(), newFileContents))
    {
        return trace_last_error(L"Failed to write file contents");
    }

    contents = read_entire_file(targetPath.c_str());
    trace_messages(L"Current target file contents: ", info_color, contents.c_str(), new_line);
    if (contents != newFileContents)
    {
        trace_message(L"ERROR: File contents do not match!\n", error_color);
        return ERROR_ASSERTION_FAILURE;
    }

    return ERROR_SUCCESS;
}

int CreateSymbolicLinkTests()
{
    int result = ERROR_SUCCESS;

    auto otherFilePath = g_packageRootPath / L"§¥₥βôℓïç£ïñƙ.txt";
    auto packageFilePath = g_packageRootPath / g_packageFileName;

    // Creating a link to a file in the package path should test/validate that we copy the package file to the
    // redirected location and create a link to that file (e.g. so that we can write to it)
    test_begin("CreateSymbolicLink to Package File Test");
    clean_redirection_path();
    auto testResult = DoCreateSymbolicLinkTest(otherFilePath, packageFilePath, g_packageFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    // Replace the contents of the package file to ensure that we copy-on-read only if the file hasn't previously been
    // copied to the redirected path
    test_begin("CreateSymbolicLink to Redirected File Test");
    clean_redirection_path();
    const char replacedFileContents[] = "You are reading from the package file in its redirected location";
    write_entire_file(packageFilePath.c_str(), replacedFileContents);
    testResult = DoCreateSymbolicLinkTest(otherFilePath, packageFilePath, replacedFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    // NOTE: Ideally we'd expect failure if we try and use the path to a package file as the link path since
    //       CreateSymbolicLink is documented to fail if the file already exists. However, due to the limitations we
    //       currently have surrounding deleting files, we intentionally don't handle this case at the moment, and
    //       therefore expect it to work here
    test_begin("CreateSymbolicLink Replace Package File Test");
    clean_redirection_path();
    const char otherFileContents[] = "You are reading from the generated file";
    write_entire_file(otherFilePath.c_str(), otherFileContents);
    testResult = DoCreateSymbolicLinkTest(packageFilePath, otherFilePath, otherFileContents);
    result = result ? result : testResult;
    test_end(testResult);

    return result;
}
