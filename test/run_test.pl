#!/usr/bin/perl
use warnings;

my $test = shift;
my $file = "$tdir/file";
my $expected = "$tdir/expected";

my(@f_begin, @f_end, @cmds);

my $target = \@cmds;
my $valid = 0;

open F, '<', $test or die "open $test: $!\n";
while(<F>){
	chomp;

	if(/^__IN__$/){
		$target = \@f_begin;
		$valid++;
	}elsif(/^__OUT__$/){
		$target = \@f_end;
		$valid++;
	}else{
		push @{$target}, $_;
	}
}

die "invalid test $test (too few/many __IN/OUT__)" unless $valid == 2;

to_file ">$file", @f_begin;

my $errf = "$tdir/err";
my $rc = run_tim($file, $errf, join('', @cmds) . "\033:wq");

if($opts{valgrind}){
	open VG, '<', $errf or die;
	my @errs = map { chomp; $_ } <VG>;
	close VG;

	if(grep /uninit|invalid/i, @errs){
		print "valgrind problems:\n";
		for(@errs){
			s/^(==|--)[0-9]+(==|--) //;
			print "\t$_\n";
		}
		$rc = 1;
	}
}

if($rc != 0){
	print "failure: (exit code) $test\n";
}else{
	to_file ">$expected", @f_end;

	my @diff = map { chomp; $_ } `diff -u '$expected' '$file'`;
	$rc = $?;
	printf "%s: %s\n", $rc ? "failure" : "success", $test;

	to_file '| sed -n "s/^/    /; l"', @diff[2 .. $#diff];
}
