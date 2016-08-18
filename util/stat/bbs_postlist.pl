#!/usr/bin/perl
use strict;
use warnings;

my $line;
my @array; #用于分解日值或做临时数组
my @brd_array; #所有要统计的版面列表(按区排序)
my %junk_boards; #所有要统计版面文章数的水版的散列
my %all_brds; #所有要统计版面的散列
my %water_king; #水王统计
my $board ; #读入的版面
my $name; #读入的发贴着
my %stat; #各版发贴数统计

sub process_brds(){
	my %brd_area = (
	        "0" => "0",
	        "u" => "1",
	        "z" => "2",
	        "c" => "3",
	        "r" => "4",
	        "a" => "5",
	        "s" => "6",
	        "t" => "7",
	        "b" => "8",
	        "p" => "9",
	        "*" => "10",
	        "\$" => "11"
	);
	
	open BRD_FILE ,"</var/bbs/betterman/.BOARDS" or die("can not open .BOARDS.\n");
	binmode(BRD_FILE);
	my $buffer;
	my $read_size;
	my %board_hash;
	while(1)
	{
	        my $filename;
	        my $title;
	        my $BM;
	        my $flag;
	        my $level;

	        $read_size = read(BRD_FILE,$buffer,128);
	        if($read_size < 128){last;}
		($filename,$title,$BM,$flag,$level,,,,) = unpack("a20a40a40b32b32IIIC*",$buffer);
                if(not defined($filename) ){next;}
                elsif($filename =~ /([a-zA-Z0-9_]*)/){$filename = $1;}
                if($filename eq "Diary"){next;}
                $title = sprintf "%s",$title;
                if($title =~ /^(\w{1}).*/ ){$title = $brd_area{$1} ;}
                if(not defined($title) ){$title = "13";}
                if($title eq "0"){next;}
                if($level != 0){next;}
                if($flag =~ /^[01]{9}1/) {next;}
                if($flag =~ /^[01]{12}1/) {next;}
                if($flag =~ /^[01]{6}1/ || $flag =~ /^[01]{7}1/){$junk_boards{$filename} = 1;}
		$all_brds{$filename} = 1;
                $board_hash{$title}{$filename} = 1;
	}
	my $array_var = 0;
        foreach my $elem1 (sort keys %board_hash){
        	foreach my $elem2 (sort keys %{$board_hash{$elem1}})
        	{
                	if($elem2){$brd_array[$array_var++] = $elem2;}
        	}
	}
	return;
				
}

process_brds();

#扫reclog/trace
#package main;
open TRACE_FILE ,"</var/bbs/reclog/trace" or die("Coule not open trace file reclog/trace .\n");
my @time = split / /,scalar localtime( time() - 24 * 3600 );
seek(TRACE_FILE,-1000,2);  #seek in the tail of the file
while(1)
{
	if(tell(TRACE_FILE) == 0){last;}
	$line = <TRACE_FILE>;
    	$line = <TRACE_FILE>;
    	if(not defined($line)){exit 0;}
    	if($line =~ m/ $time[0] $time[1] $time[2] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] $time[4] /){last;}
    	else{seek(TRACE_FILE,-1000,1);}
}

@time = split / /,scalar localtime( time() );
while(1){
	$line = <TRACE_FILE>;
	if(not defined($line)){ last; }
	chomp($line);
	if($line =~ m/.* $time[0] $time[1] $time[2] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] $time[4] posted \'.*\' on \'.*\'/ )
	{
		@array = split /\'/,$line;
		$board = $array[$#array];
		@array = split / /,$line;
		$name = $array[0];
		$stat{$board}{$name}++;
		if(defined($all_brds{$board}) && not defined($junk_boards{$board})){$water_king{$name}++;}
	}		
}
close(TRACE_FILE);

#扫wwwlog/trace
open TRACE_FILE ,"</var/bbs/wwwlog/trace" or die("Coule not open trace file reclog/trace .\n");
@time = split / /,scalar localtime( time() - 24 * 3600 );
seek(TRACE_FILE,-1000,2);  #seek in the tail of the file
while(1)
{
	if(tell(TRACE_FILE) == 0){last;}
        $line = <TRACE_FILE>;
        $line = <TRACE_FILE>;
        if(not defined($line)){exit 0;}
        if($line =~ m/^$time[1] $time[2] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] $time[4] /){last;}
	else{seek(TRACE_FILE,-1000,1);}
}
@time = split / /,scalar localtime( time() );                                                  while(1){ 
	$line = <TRACE_FILE>;  
	if(not defined($line)){ last; }
	chomp($line);
	if($line =~ m/^$time[1] $time[2] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] $time[4]\s+?(\S+)\s+? post .* on\s+?(\S+)$/ ) 
	{
		$board = $2; 
		$name = $1;
		$stat{$board}{$name}++;   
		if(defined($all_brds{$board}) && not defined($junk_boards{$board})){$water_king{$name}++;} 
	} 
} 
close(TRACE_FILE); 
		    

my %mgb;
my $today = time() - time() % (24 * 3600);
foreach $board (@brd_array)
{
	open DIR_FILE,"</var/bbs/boards/$board/.DIR" or die("Can't open .DIR file $board/.DIR\n");
	binmode(DIR_FILE);

	my $m = 0;
	my $g = 0;
	my $b = 0;
	my $buffer;
	my $read_size;
	seek(DIR_FILE,-1024,2);  #seek in the tail of the file
	while(1)
	{
		$read_size = read(DIR_FILE,$buffer,128);
	        if($read_size != 128){goto skip;}
		if(tell(DIR_FILE) == 0){last;}
		my ($filename,$owner,$realowner,$title,$flag,$size,$id,$filetime,) = unpack("a16a14a14a56b32IIIB*",$buffer);
		if($filetime<$today){last;}
		else{seek(DIR_FILE,-1024,1);}
	}	
	
	while(1)
	{
		$read_size = read(DIR_FILE,$buffer,128);
        	if($read_size != 128){last;}
        	my ($filename,$owner,$realowner,$title,$flag,$size,$id,$filetime,) = unpack("a16a14a14a56b32IIIB*",$buffer);
        	if($filetime<$today){next;}
		if($flag =~ /^[01]{3}1/ && $flag =~ /^[01]{4}1/){$b++;}
        	if($flag =~ /^[01]{4}1/){$g++;}
        	if($flag =~ /^[01]{3}1/){$m++;}
        }
	$mgb{$board} = [$m,$g,$b];
	close(DIR_FILE);
}

skip:
my $crlf ;
open POSTLIST_FILE,">/var/bbs/etc/postlist" or die("cannot open the file \"postlist\"");
foreach $board (@brd_array)
{
	if(not defined($board) || !$board || $board eq "" || $board =~ /^\s$/){print "\n\n" ;next;}
	print POSTLIST_FILE "版面: $board";
	$crlf = 0;
	my $count = 0;
	foreach $name (sort keys %{$stat{$board}}){
		if($crlf++ == 0){print POSTLIST_FILE "\n";}
		$count += $stat{$board}{$name};
		printf POSTLIST_FILE "%-12s = %-3s  " ,$name , $stat{$board}{$name};
		if($crlf == 3){$crlf = 0;}
	}
	print POSTLIST_FILE "\n";
	print POSTLIST_FILE "共发帖 : $count\n";
	print POSTLIST_FILE "其中 m文:$mgb{$board}[0] ,g文:$mgb{$board}[1] ,b文:$mgb{$board}[2]\n";
	print POSTLIST_FILE "\n\n";
}

my @top_ten = sort { $water_king{$b} <=> $water_king{$a} or $a cmp $b } keys %water_king; #按发帖数排序
my $top_ten_var = 0; 
my $last_count = -1;
print POSTLIST_FILE "今日发帖数十大使用者(不统计水版及限制版):\n";
foreach $name (@top_ten)
{
	if($water_king{$name} != $last_count){$last_count = $water_king{$name};$top_ten_var++;}
	if($top_ten_var >= 11){last;}
	printf POSTLIST_FILE "%-12s = %-3s  \n" , $name , $water_king{$name};
}

close (TRACE_FILE);
close (POSTLIST_FILE);
exit 0;

