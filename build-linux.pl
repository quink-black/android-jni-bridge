#!/usr/bin/env perl -w

use PrepareAndroidSDK;
use File::Path;
use strict;
use warnings;

my $api = "jdk";

my @classes = (
    '::java::lang::System',
    '::java::lang::UnsupportedOperationException'
);

sub BuildLinux
{
    my $class_names = join(' ', @classes);

    system("make clean") && die("Clean failed");
    system("make MAKEFLAGS=-j4 PLATFORM=linux APICLASSES=\"$class_names\"") && die("Failed to make linux library");
}

sub ZipIt
{
    system("mkdir -p build/temp/include") && die("Failed to create temp directory.");

    # write build info
    my $git_info = qx(git symbolic-ref -q HEAD && git rev-parse HEAD);
    open(BUILD_INFO_FILE, '>', "build/temp/build.txt") or die("Unable to write build information to build/temp/build.txt");
    print BUILD_INFO_FILE "$git_info";
    close(BUILD_INFO_FILE);

    # create zip
    system("cp build/$api/source/*.h build/temp/include") && die("Failed to copy headers.");
    system("cd build && jar cf temp/jnibridge.jar bitter") && die("Failed to create java class archive.");
    system("cd build/$api && zip ../builds.zip -r linux/*.a") && die("Failed to package libraries into zip file.");
    system("cd build/temp && zip ../builds.zip -r jnibridge.jar build.txt include") && die("Failed to package headers into zip file.");
    system("rm -r build/temp") && die("Unable to remove temp directory.");
    system("cd test; unzip -o ../build/builds.zip; touch Test.cpp") && die("Unable to prepare for tests");
}

BuildLinux();
ZipIt();
