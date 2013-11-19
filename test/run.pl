#!/usr/bin/perl
use warnings;

my %opts = (
	valgrind => 0
);

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

	die "invalid test $test" unless $valid == 2;

	to_file ">$file", @f_begin;

	my $errf = "$tdir/err";
	my $rc = run_tim($file, $errf, join('', @cmds) . ":wq");

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

	return $rc;
}

sub runshtest
{
	my $f = shift;
	my $r = system("sh $f >/dev/null 2>&1");
	printf "%s: %s\n", $r ? "failure" : "success", $f;
	return $r;
}

sub run_tim
{
	my $pid = fork();

	if($pid == 0){
		my $inp = "$tdir/inp";
		my($file, $errf, $cmds) = @_;
		to_file ">$inp", $cmds;

		open STDIN, '<', $inp or die;

		my @args = ($tim, $file);

		if($opts{valgrind}){
			unshift @args, 'valgrind';
			open STDERR, '>', $errf or die;
		}

		exec @args;
		die;
	}

	my $dead = wait;
	my $rc = $?;

	die "bad reaped proc $dead != $pid"
		unless $dead == $pid;

	return $rc;
}

if(@ARGV and $ARGV[0] eq '-v'){
	$opts{valgrind} = 1;
	shift @ARGV;
}

my @tests = @ARGV ? @ARGV : glob '*test';
my($n, $pass) = (0,0);

# for shtests
$ENV{tim} = $tim;
$ENV{tmp} = "$tdir/tmp";

for my $f (@tests){
	my $r;
	if($f =~ /\.test$/){
		$r = runtest $f;
	}else{
		$r = runshtest $f;
	}

	if($r){
		system('stty', qw(isig onlcr icrnl ixon brkint -noflsh));
	}

	$pass += ($r == 0);
	$n++;
}

print "$n tests, $pass pass, " . ($n-$pass) . " fail\n";
exit ($pass == $n ? 0 : 1);
