#!/usr/bin/perl
use warnings;

$tim = '../tim';
$tdir = '/tmp/tim.test';
mkdir $tdir;

END {
	my $ec = $?;
	system('rm', '-rf', $tdir);
	$? = $ec;
}

sub to_file
{
	my($fnam, @l) = @_;
	open F, $fnam or die "open $fnam: $!\n";
	print F "$_\n" for @l;
	close F;
}

sub runtest
{
	my $test = shift;
	my $file = "$tdir/file";
	my $expected = "$tdir/expected";

	my(@f_begin, @f_end, @cmds);

	my $target = \@cmds;

	open F, '<', $test or die "open $test: $!\n";
	while(<F>){
		chomp;

		if(/^__IN__$/){
			$target = \@f_begin;
		}elsif(/^__OUT__$/){
			$target = \@f_end;
		}else{
			push @{$target}, $_;
		}
	}

	to_file ">$file", @f_begin;

	my $rc = run_tim($file, join('', @cmds) . ":wq");

	if($rc != 0){
		print "failure: (exit code) $test\n";
	}else{
		to_file ">$expected", @f_end;

		my @diff = map { chomp; $_ } `diff -u '$expected' '$file'`;
		$rc = $?;
		printf "%s: %s\n", $rc ? "failure" : "success", $test;

		to_file '| sed -n l', @diff[2 .. $#diff];
	}

	return $rc;
}

sub run_tim
{
	my $pid = fork();

	if($pid == 0){
		my $inp = "$tdir/inp";
		my($file, $cmds) = @_;
		to_file ">$inp", $cmds;

		open STDIN, '<', $inp or die;
		exec $tim, $file;
		die;
	}

	my $dead = wait;
	my $rc = $?;

	die "bad reaped proc $dead != $pid"
		unless $dead == $pid;

	return $rc;
}

my @tests = @ARGV ? @ARGV : glob '*.test';
my($n, $pass) = (0,0);

for my $f (@tests){
	my $r = runtest $f;
	$pass += ($r == 0);
	$n++;
}

print "$n tests, $pass pass, " . ($n-$pass) . " fail\n";
exit ($pass == $n ? 0 : 1);
