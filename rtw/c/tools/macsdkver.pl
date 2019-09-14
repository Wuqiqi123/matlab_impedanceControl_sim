# Copyright 2014-2017 The MathWorks, Inc.
#
# File    : macsdkver.pl
# Abstract:
#       Determine the SDK version to use for the MAC OS
#

# Use the lowest macosx version 'xcodebuild -showsdks' returns.

my @sdks = `xcodebuild -showsdks`;
my $sdkver = undef;
my @verlist = ();
foreach my $line (@sdks){
    if($line =~ /\-sdk macosx([\d\.]+)/){
        my @aver = split('\.',$1);
        push(@verlist,$1);
    }
}

if(scalar(@verlist)>0){
    @verlist = sort(@verlist);
    print $verlist[0];
}else{
    die "Please download a supported version of Xcode";
}

#[eof] macsdkver.pl
