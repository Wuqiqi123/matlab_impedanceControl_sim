# Copyright 2015 The MathWorks, Inc.
#
# File    : mkvc_rsp.pl   
#
# Abstract:
#      Script to write a long list of filenames to a single file. Useful 
#      in Windows when the length of the string is greater than the maximum 
#      length of the string that you can use at the command prompt (8191 
#      characters in Windows XP and later)
#
#    Usage:
#      perl mkvc_rsp.pl cmdfile one.c two.c c:\mysrc\three.c four.c 
#

$filename = shift @ARGV;
open(filehndl,">$filename") || die "Error creating file $filename: $!\n";
grep(do { print filehndl "$_\n"; }, @ARGV);
close(filehndl);
exit(0);