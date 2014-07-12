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

sub runshtest
{
	my $f = shift;
	my $r = system("sh $f >/dev/null");
	printf "%s: %s\n", $r ? "failure" : "success", $f;
	return $r;
}

sub rundisp
{
	my $f = shift;


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

my @failures;

for my $f (@tests){
	my $r;
	if($f =~ /\.test$/){
		$r = runtest $f;
	}elsif($f =~ /\.disp$/){
		$r = rundisp $f;
	}else{
		$r = runshtest $f;
	}

	if($r){
		system('stty', qw(isig onlcr icrnl ixon brkint -noflsh));
		push @failures, $f;
	}

	$pass += ($r == 0);
	$n++;
}

print "failure summary: @failures\n" if @failures;
print "$n tests, $pass pass, " . ($n-$pass) . " fail\n";
exit ($pass == $n ? 0 : 1);
